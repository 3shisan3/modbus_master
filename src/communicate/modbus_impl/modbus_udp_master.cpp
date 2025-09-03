/**
 * @file modbus_udp_master.cpp
 * @brief 简化版Modbus UDP主站实现
 */

#include "modbus_udp_master.h"

#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

#include "udp-tcp-communicate/communicate_api.h"

namespace modbus
{

// 先定义Impl类
class ModbusUdpMaster::Impl
{
public:
    Impl(const std::string& ip, uint16_t port);
    ~Impl();

    ModbusResponse send_request(const ModbusRequest& request,
                              std::chrono::milliseconds timeout);

private:
    // 内部响应处理器类
    class ResponseHandler : public communicate::SubscribebBase
    {
    public:
        explicit ResponseHandler(Impl* owner) : owner_(owner) {}

        int handleMsg(std::shared_ptr<void> msg) override
        {
            return owner_ ? owner_->handle_response(msg) : -1;
        }

    private:
        Impl* owner_;
    };

    int handle_response(std::shared_ptr<void> msg);
    size_t get_actual_message_length(const uint8_t* data);
    std::vector<uint8_t> build_request_frame(const ModbusRequest& request);
    void append_uint16(std::vector<uint8_t>& frame, uint16_t value);
    uint16_t calculate_crc(const uint8_t* data, size_t length);
    bool verify_crc(const uint8_t* data, size_t length);

    std::string targetIp_;
    uint16_t targetPort_;
    std::unique_ptr<ResponseHandler> handler_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<ModbusResponse> responseQueue_;
};

// Impl成员函数实现
ModbusUdpMaster::Impl::Impl(const std::string& ip, uint16_t port)
    : targetIp_(ip), targetPort_(port),
      handler_(std::make_unique<ResponseHandler>(this))
{
    // 订阅响应消息
    if (::communicate::SubscribeLocal("", port, handler_.get()) != 0)
    {
        throw std::runtime_error("Failed to subscribe response messages");
    }
}

ModbusUdpMaster::Impl::~Impl()
{
    // 取消订阅 - 如果API支持
    // 注意: 根据错误信息，UnSubscribeLocal可能不存在
    // 这里改为使用SubscribeLocal的取消方式或忽略
}

ModbusResponse ModbusUdpMaster::Impl::send_request(const ModbusRequest& request,
                                                  std::chrono::milliseconds timeout)
{
    // 构建请求帧
    std::vector<uint8_t> frame = build_request_frame(request);

    // 清空之前的响应
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!responseQueue_.empty()) responseQueue_.pop();
    }

    // 发送请求
    if (::communicate::SendGeneralMessage(targetIp_.c_str(), targetPort_,
                                        frame.data(), frame.size()) != 0)
    {
        throw std::runtime_error("Failed to send Modbus request");
    }

    // 等待响应
    std::unique_lock<std::mutex> lock(mutex_);
    if (cv_.wait_for(lock, timeout, [this] { return !responseQueue_.empty(); }))
    {
        ModbusResponse response = responseQueue_.front();
        responseQueue_.pop();
        return response;
    }

    throw std::runtime_error("Response timeout");
}

int ModbusUdpMaster::Impl::handle_response(std::shared_ptr<void> msg)
{
    if (!msg) return -1;

    uint8_t* data = static_cast<uint8_t*>(msg.get());
    size_t size = get_actual_message_length(data);

    // 基本校验
    if (size < 4) return -1;

    // 创建响应对象
    ModbusResponse response;
    response.slave_address = data[0];
    response.function_code = static_cast<FunctionCode>(data[1]);

    // 处理异常响应
    if (static_cast<uint8_t>(response.function_code) & 0x80)
    {
        if (size != 5) return -1;
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
                return -1;
            response.data.assign(data + 3, data + size - 2);
            break;

        case FunctionCode::WRITE_SINGLE_REGISTER:
        case FunctionCode::WRITE_MULTIPLE_REGISTERS:
            if (size != 8) return -1;
            break;

        default:
            return -1;
        }
    }

    // CRC校验
    if (!verify_crc(data, size)) return -1;

    // 存储响应并通知等待线程
    {
        std::lock_guard<std::mutex> lock(mutex_);
        responseQueue_.push(response);
    }
    cv_.notify_one();

    return 0;
}

size_t ModbusUdpMaster::Impl::get_actual_message_length(const uint8_t* data)
{
    // 根据功能码判断消息长度
    if (data[1] & 0x80) return 5; // 异常响应
    
    switch(data[1]) {
        case 0x03: // 读取保持寄存器
        case 0x04: // 读取输入寄存器
            return 5 + data[2];
        case 0x06: // 写单个寄存器
        case 0x10: // 写多个寄存器
            return 8;
        default:
            return 0;
    }
}

std::vector<uint8_t> ModbusUdpMaster::Impl::build_request_frame(const ModbusRequest& request)
{
    std::vector<uint8_t> frame;
    frame.push_back(request.slave_address);
    frame.push_back(static_cast<uint8_t>(request.function_code));

    switch (request.function_code)
    {
    case FunctionCode::READ_HOLDING_REGISTERS:
    case FunctionCode::READ_INPUT_REGISTERS:
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.register_count);
        break;

    case FunctionCode::WRITE_SINGLE_REGISTER:
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.values[0]);
        break;

    case FunctionCode::WRITE_MULTIPLE_REGISTERS:
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.register_count);
        frame.push_back(static_cast<uint8_t>(request.values.size() * 2));
        for (uint16_t value : request.values)
        {
            append_uint16(frame, value);
        }
        break;

    default:
        throw std::runtime_error("Unsupported function code");
    }

    // 添加CRC校验
    uint16_t crc = calculate_crc(frame.data(), frame.size());
    frame.push_back(crc & 0xFF);
    frame.push_back((crc >> 8) & 0xFF);

    return frame;
}

void ModbusUdpMaster::Impl::append_uint16(std::vector<uint8_t>& frame, uint16_t value)
{
    frame.push_back((value >> 8) & 0xFF);
    frame.push_back(value & 0xFF);
}

uint16_t ModbusUdpMaster::Impl::calculate_crc(const uint8_t* data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

bool ModbusUdpMaster::Impl::verify_crc(const uint8_t* data, size_t length)
{
    if (length < 2) return false;
    uint16_t received_crc = (data[length - 1] << 8) | data[length - 2];
    return calculate_crc(data, length - 2) == received_crc;
}

// ModbusUdpMaster成员函数实现
ModbusUdpMaster::ModbusUdpMaster(const std::string& ip, uint16_t port)
    : impl_(std::make_unique<Impl>(ip, port)) {}

ModbusUdpMaster::~ModbusUdpMaster() = default;

ModbusResponse ModbusUdpMaster::send_request(const ModbusRequest& request,
                                           std::chrono::milliseconds timeout)
{
    return impl_->send_request(request, timeout);
}

} // namespace modbus