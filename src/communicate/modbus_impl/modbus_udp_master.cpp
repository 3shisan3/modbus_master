/**
 * @file modbus_udp_master.cpp
 * @brief 简化版Modbus UDP主站实现
 * @note 采用轮询方式检查响应，避免锁竞争问题
 */

#include "modbus_udp_master.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include "udp-tcp-communicate/communicate_api.h"

namespace modbus
{

/**
 * @brief ModbusUdpMaster的实现类
 * @details 采用轮询方式检查响应，完全避免锁竞争
 */
class ModbusUdpMaster::Impl
{
public:
    Impl(const std::string &ip, uint16_t port);
    ~Impl();

    ModbusResponse send_request(const ModbusRequest &request,
                                std::chrono::milliseconds timeout);

private:
    /**
     * @brief 响应消息处理器
     */
    class ResponseHandler : public communicate::SubscribebBase
    {
    public:
        explicit ResponseHandler(Impl *owner) : owner_(owner) {}

        int handleMsg(std::shared_ptr<void> msg) override
        {
            return owner_ ? owner_->handle_response(msg) : -1;
        }

    private:
        Impl *owner_;
    };

    /**
     * @brief 响应消息结构
     */
    struct ResponseMessage
    {
        std::vector<uint8_t> data;
        std::chrono::steady_clock::time_point receive_time;
    };

    /**
     * @brief 请求上下文信息
     */
    struct RequestContext
    {
        uint16_t transaction_id;                         // 事务ID
        std::chrono::steady_clock::time_point send_time; // 发送时间
        std::chrono::milliseconds timeout;               // 超时时间
        ModbusResponse response;                         // 响应数据
        bool response_received = false;                  // 是否收到响应
        bool processed = false;                          // 是否已处理
    };

    // 响应处理函数
    int handle_response(std::shared_ptr<void> msg);

    // 处理接收到的响应数据
    bool process_response_data(const uint8_t *data, size_t size, ModbusResponse &response);

    std::string targetIp_;                     // 目标设备IP
    uint16_t targetPort_;                      // 目标设备端口
    std::unique_ptr<ResponseHandler> handler_; // 响应处理器

    // 线程同步
    std::mutex mutex_;                 // 保护共享数据的互斥锁
    std::atomic<bool> running_{false}; // 运行标志

    std::atomic<uint16_t> transaction_id_{0}; // 事务ID计数器

    // 响应消息队列
    std::queue<ResponseMessage> response_queue_;

    // 待处理的请求映射表：事务ID -> 请求上下文
    std::unordered_map<uint16_t, std::shared_ptr<RequestContext>> pending_requests_;
};

/**
 * @brief 构造函数
 * @param ip 目标设备IP
 * @param port 目标设备端口
 */
ModbusUdpMaster::Impl::Impl(const std::string &ip, uint16_t port)
    : targetIp_(ip)
    , targetPort_(port)
    , handler_(std::make_unique<ResponseHandler>(this))
{
    // 订阅响应消息
    if (::communicate::SubscribeLocal("", port, handler_.get()) != 0)
    {
        throw std::runtime_error("Failed to subscribe response messages");
    }

    running_ = true;
}

/**
 * @brief 析构函数
 */
ModbusUdpMaster::Impl::~Impl()
{
    running_ = false;

    // 清理所有未完成请求
    std::lock_guard<std::mutex> lock(mutex_);
    pending_requests_.clear();
    while (!response_queue_.empty())
    {
        response_queue_.pop();
    }
}

/**
 * @brief 发送Modbus请求并等待响应
 * @param request Modbus请求
 * @param timeout 超时时间
 * @return Modbus响应
 */
ModbusResponse ModbusUdpMaster::Impl::send_request(const ModbusRequest &request,
                                                   std::chrono::milliseconds timeout)
{
    // 生成唯一的事务ID
    uint16_t tid = transaction_id_.fetch_add(1);

    // 创建请求上下文
    auto context = std::make_shared<RequestContext>();
    context->transaction_id = tid;
    context->send_time = std::chrono::steady_clock::now();
    context->timeout = timeout;

    // 注册到待处理请求表
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_[tid] = context;
    }

    // 构建请求帧
    std::vector<uint8_t> frame = SsModbusMaster::build_request_frame(request);

    // 发送请求
    if (::communicate::SendGeneralMessage(targetIp_.c_str(), targetPort_,
                                          frame.data(), frame.size()) != 0)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_.erase(tid);
        throw std::runtime_error("Failed to send Modbus request");
    }

    // 轮询等待响应或超时
    auto end_time = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < end_time)
    {
        // 处理响应队列
        std::vector<ResponseMessage> messages_to_process;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            while (!response_queue_.empty())
            {
                messages_to_process.push_back(std::move(response_queue_.front()));
                response_queue_.pop();
            }
        }

        // 处理所有响应消息
        for (auto &msg : messages_to_process)
        {
            ModbusResponse response;
            if (process_response_data(msg.data.data(), msg.data.size(), response))
            {
                // 查找匹配的请求
                std::lock_guard<std::mutex> lock(mutex_);
                for (auto &pair : pending_requests_)
                {
                    auto &ctx = pair.second;
                    if (ctx->transaction_id == tid && !ctx->response_received)
                    {
                        ctx->response = response;
                        ctx->response_received = true;
                        ctx->processed = true;
                        return response;
                    }
                }
            }
        }

        // 检查当前请求是否已经完成
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pending_requests_.find(tid);
            if (it != pending_requests_.end() && it->second->response_received)
            {
                ModbusResponse response = it->second->response;
                pending_requests_.erase(it);
                return response;
            }
        }

        // 短暂休眠，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 超时处理
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_.erase(tid);
        throw std::runtime_error("Response timeout");
    }
}

/**
 * @brief 处理接收到的响应消息
 * @param msg 原始消息数据
 * @return 处理结果：0成功，-1失败
 */
int ModbusUdpMaster::Impl::handle_response(std::shared_ptr<void> msg)
{
    if (!msg)
        return -1;

    uint8_t *data = static_cast<uint8_t *>(msg.get());
    size_t size = SsModbusMaster::get_actual_message_length(data);

    // 基本校验
    if (size < 4)
        return -1;

    // 将响应消息加入队列
    ResponseMessage response_msg;
    response_msg.data.assign(data, data + size);
    response_msg.receive_time = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        response_queue_.push(std::move(response_msg));
    }

    return 0;
}

/**
 * @brief 处理响应数据
 * @param data 响应数据
 * @param size 数据大小
 * @param response 输出的响应对象
 * @return 处理结果：true成功，false失败
 */
bool ModbusUdpMaster::Impl::process_response_data(const uint8_t *data, size_t size, ModbusResponse &response)
{
    // CRC校验
    if (!SsModbusMaster::verify_crc(data, size))
        return false;

    // 赋值响应对象
    response.slave_address = data[0];
    response.function_code = static_cast<FunctionCode>(data[1]);

    // 处理异常响应
    if (static_cast<uint8_t>(response.function_code) & 0x80)
    {
        if (size != 5)
            return false;
        response.error = static_cast<ModbusError>(data[2]);
    }
    else
    {
        // 处理正常响应
        switch (response.function_code)
        {
        case FunctionCode::READ_HOLDING_REGISTERS:
        case FunctionCode::READ_INPUT_REGISTERS:
            if (size < 5 || data[2] == 0 || size != (size_t)(5 + data[2]))
                return false;
            response.data.assign(data + 3, data + size - 2);
            break;

        case FunctionCode::WRITE_SINGLE_REGISTER:
        case FunctionCode::WRITE_MULTIPLE_REGISTERS:
            if (size != 8)
                return false;
            break;

        default:
            return false;
        }
    }

    response.error = ModbusError::NO_ERROR;
    return true;
}

// ModbusUdpMaster成员函数实现
ModbusUdpMaster::ModbusUdpMaster(const std::string &ip, uint16_t port)
    : impl_(std::make_unique<Impl>(ip, port)) {}

ModbusUdpMaster::~ModbusUdpMaster() = default;

ModbusResponse ModbusUdpMaster::send_request(const ModbusRequest &request,
                                             std::chrono::milliseconds timeout)
{
    return impl_->send_request(request, timeout);
}

} // namespace modbus