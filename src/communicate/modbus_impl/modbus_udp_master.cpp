#include "modbus_udp_master.h"
#include <array>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

#include "udp-tcp-communicate/communicate_api.h"

namespace modbus
{

// 先定义Impl类
class ModbusUdpMaster::Impl
{
public:
    Impl(const std::string &ip, uint16_t port);
    ~Impl() = default;

    ModbusResponse query(const ModbusRequest &request, std::chrono::milliseconds timeout);

    void control_async(const ModbusRequest &request);

    void control_batch(const std::vector<ModbusRequest> &requests);

    void set_polling_interval(std::chrono::milliseconds interval);

    ModbusUdpMaster::CommunicationStatus get_status() const;

    ModbusResponse send_request(const ModbusRequest &request, std::chrono::milliseconds timeout);

    int handleResponse(std::shared_ptr<void> msg);

private:
    std::vector<uint8_t> build_request_frame(const ModbusRequest &request);
    
    void append_uint16(std::vector<uint8_t> &frame, uint16_t value);

    uint16_t calculate_crc(const uint8_t *data, size_t length);
    
    bool verify_crc(const uint8_t *data, size_t length);

    std::string targetIp_;
    uint16_t targetPort_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<ModbusResponse> responseQueue_;

    struct Status
    {
        std::atomic<uint32_t> total_queries{0};
        std::atomic<uint32_t> failed_queries{0};
        std::atomic<uint32_t> total_controls{0};
        std::atomic<uint32_t> failed_controls{0};
        std::chrono::milliseconds avg_response_time{0};
    } status_;

    std::chrono::milliseconds minPollingInterval_;
    std::chrono::steady_clock::time_point lastPollTime_;
    friend class ModbusUdpResponseHandler; // 声明为友元类
};

// 然后定义响应处理器
class ModbusUdpResponseHandler : public communicate::SubscribebBase
{
public:
    explicit ModbusUdpResponseHandler(ModbusUdpMaster::Impl *owner)
        : owner_(owner) {}

    int handleMsg(std::shared_ptr<void> msg) override
    {
        if (owner_)
        {
            return owner_->handleResponse(msg);
        }
        return -1;
    }

private:
    ModbusUdpMaster::Impl *owner_;
};

// Impl成员函数实现
ModbusUdpMaster::Impl::Impl(const std::string &ip, uint16_t port)
    : targetIp_(ip), targetPort_(port),
      minPollingInterval_(std::chrono::milliseconds(100)),
      lastPollTime_(std::chrono::steady_clock::now())
{
    if (::communicate::SubscribeLocal("", port, new ModbusUdpResponseHandler(this)) != 0)
    {
        throw std::runtime_error("Failed to subscribe response messages");
    }
}

int ModbusUdpMaster::Impl::handleResponse(std::shared_ptr<void> msg)
{
    if (!msg)
        return -1;

    auto *rawData = static_cast<uint8_t *>(msg.get());
    size_t size = *static_cast<size_t *>(msg.get());
    if (size < 4)
        return -1;

    ModbusResponse response;
    response.slave_address = rawData[0];
    response.function_code = static_cast<FunctionCode>(rawData[1]);

    if (static_cast<uint8_t>(response.function_code) & 0x80)
    {
        if (size != 5)
            return -1;
        response.error = static_cast<ModbusError>(rawData[2]);
        if (!verify_crc(rawData, size))
            return -1;
    }
    else
    {
        switch (response.function_code)
        {
        case FunctionCode::READ_HOLDING_REGISTERS:
        case FunctionCode::READ_INPUT_REGISTERS:
        {
            if (size < 5 || rawData[2] == 0 || size != (size_t)(5 + rawData[2]))
            {
                return -1;
            }
            if (!verify_crc(rawData, size))
                return -1;
            response.data.assign(rawData + 2, rawData + size - 2);
            break;
        }

        case FunctionCode::WRITE_SINGLE_REGISTER:
        {
            if (size != 8)
                return -1;
            if (!verify_crc(rawData, size))
                return -1;
            break;
        }

        case FunctionCode::WRITE_MULTIPLE_REGISTERS:
        {
            if (size != 8)
                return -1;
            if (!verify_crc(rawData, size))
                return -1;
            break;
        }

        default:
            return -1;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (responseQueue_.size() >= 10)
        {
            responseQueue_.pop();
        }
        responseQueue_.push(response);
    }

    cv_.notify_one();
    return 0;
}

ModbusResponse ModbusUdpMaster::Impl::query(const ModbusRequest &request,
                                            std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);

    // 频率控制检查
    auto now = std::chrono::steady_clock::now();
    if (now - lastPollTime_ < minPollingInterval_)
    {
        std::this_thread::sleep_for(minPollingInterval_ - (now - lastPollTime_));
    }
    lastPollTime_ = std::chrono::steady_clock::now();

    // 记录查询开始时间
    auto start_time = std::chrono::steady_clock::now();
    status_.total_queries++;

    // 发送请求
    std::vector<uint8_t> frame = build_request_frame(request);
    responseQueue_ = std::queue<ModbusResponse>();

    if (::communicate::SendGeneralMessage(targetIp_.c_str(), targetPort_,
                                          frame.data(), frame.size()) != 0)
    {
        status_.failed_queries++;
        throw std::runtime_error("Failed to send Modbus request");
    }

    // 等待响应
    if (cv_.wait_for(lock, timeout, [this]()
                     { return !responseQueue_.empty(); }))
    {
        ModbusResponse response = responseQueue_.front();
        responseQueue_.pop();

        // 更新平均响应时间
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        auto total_queries = status_.total_queries.load();
        status_.avg_response_time = std::chrono::milliseconds(
            (status_.avg_response_time.count() * (total_queries - 1) + duration.count()) / total_queries);

        return response;
    }

    status_.failed_queries++;
    throw std::runtime_error("Response timeout");
}

void ModbusUdpMaster::Impl::control_async(const ModbusRequest &request)
{
    std::vector<uint8_t> frame = build_request_frame(request);
    status_.total_controls++;

    if (::communicate::SendGeneralMessage(targetIp_.c_str(), targetPort_,
                                          frame.data(), frame.size()) != 0)
    {
        status_.failed_controls++;
    }
}

void ModbusUdpMaster::Impl::control_batch(const std::vector<ModbusRequest> &requests)
{
    for (const auto &request : requests)
    {
        control_async(request);
    }
}

void ModbusUdpMaster::Impl::set_polling_interval(std::chrono::milliseconds interval)
{
    std::lock_guard<std::mutex> lock(mutex_);
    minPollingInterval_ = interval;
}

ModbusUdpMaster::CommunicationStatus ModbusUdpMaster::Impl::get_status() const
{
    CommunicationStatus result;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        result.total_queries = status_.total_queries.load();
        result.failed_queries = status_.failed_queries.load();
        result.total_controls = status_.total_controls.load();
        result.failed_controls = status_.failed_controls.load();
        result.avg_response_time = status_.avg_response_time;
    }
    return result;
}

ModbusResponse ModbusUdpMaster::Impl::send_request(const ModbusRequest &request,
                                                   std::chrono::milliseconds timeout)
{
    return query(request, timeout);
}

std::vector<uint8_t> ModbusUdpMaster::Impl::build_request_frame(const ModbusRequest &request)
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

    uint16_t crc = calculate_crc(frame.data(), frame.size());
    frame.push_back(crc & 0xFF);
    frame.push_back((crc >> 8) & 0xFF);

    return frame;
}

void ModbusUdpMaster::Impl::append_uint16(std::vector<uint8_t> &frame, uint16_t value)
{
    frame.push_back((value >> 8) & 0xFF);
    frame.push_back(value & 0xFF);
}

uint16_t ModbusUdpMaster::Impl::calculate_crc(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool ModbusUdpMaster::Impl::verify_crc(const uint8_t *data, size_t length)
{
    if (length < 2)
        return false;
    uint16_t received_crc = (data[length - 1] << 8) | data[length - 2];
    uint16_t calculated_crc = calculate_crc(data, length - 2);
    return received_crc == calculated_crc;
}

// ModbusUdpMaster成员函数实现
ModbusUdpMaster::ModbusUdpMaster(const std::string &ip, uint16_t port)
    : impl_(std::make_unique<Impl>(ip, port)) {}

ModbusUdpMaster::~ModbusUdpMaster() = default;

ModbusResponse ModbusUdpMaster::query(const ModbusRequest &request,
                                      std::chrono::milliseconds timeout)
{
    return impl_->query(request, timeout);
}

void ModbusUdpMaster::control_async(const ModbusRequest &request)
{
    impl_->control_async(request);
}

void ModbusUdpMaster::control_batch(const std::vector<ModbusRequest> &requests)
{
    impl_->control_batch(requests);
}

void ModbusUdpMaster::set_polling_interval(std::chrono::milliseconds interval)
{
    impl_->set_polling_interval(interval);
}

ModbusUdpMaster::CommunicationStatus ModbusUdpMaster::get_status() const
{
    return impl_->get_status();
}

ModbusResponse ModbusUdpMaster::send_request(const ModbusRequest &request,
                                             std::chrono::milliseconds timeout)
{
    return impl_->query(request, timeout);
}

} // namespace modbus