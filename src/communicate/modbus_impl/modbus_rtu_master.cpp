/**
 * @file modbus_rtu_master.cpp
 * @brief Modbus RTU主站实现
 */

#include "modbus_rtu_master.h"
#ifdef _WIN32
    #include "serial_win.h"
    using SerialPort = WinSerialPort;
#else
    #include "serial_linux.h"
    using SerialPort = LinuxSerialPort;
#endif

#include <array>
#include <mutex>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

namespace modbus
{

class ModbusRtuMaster::Impl
{
public:
    Impl(const std::string &port, uint32_t baudrate, Parity parity)
        : serialPort_(), baudrate_(baudrate), parity_(parity)
    {
        if (!serialPort_.open(port, baudrate))
        {
            throw std::runtime_error("Failed to open serial port: " + port);
        }
    }

    ~Impl()
    {
        serialPort_.close();
    }

    ModbusResponse send_request(const ModbusRequest &request,
                                std::chrono::milliseconds timeout)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 构建请求帧
        std::vector<uint8_t> frame = SsModbusMaster::build_request_frame(request);

        // 清空接收缓冲区
        clear_input_buffer();

        // 发送请求
        if (serialPort_.write(frame.data(), frame.size()) != frame.size())
        {
            throw std::runtime_error("Failed to send Modbus request");
        }

        // 接收响应
        return receive_response(request, timeout);
    }

private:
    SerialPort serialPort_;
    uint32_t baudrate_;
    Parity parity_;
    std::mutex mutex_;

    // 清空输入缓冲区
    void clear_input_buffer()
    {
        uint8_t buffer[256];
        while (serialPort_.read(buffer, sizeof(buffer)) > 0)
        {
            // 持续读取直到缓冲区为空
        }
    }

    // 接收响应
    ModbusResponse receive_response(const ModbusRequest &request,
                                    std::chrono::milliseconds timeout)
    {
        ModbusResponse response;
        std::array<uint8_t, 256> buffer;

        // 读取响应头 (地址+功能码)
        if (!read_with_timeout(buffer.data(), 2, timeout))
        {
            throw std::runtime_error("Response timeout");
        }

        response.slave_address = buffer[0];
        response.function_code = static_cast<FunctionCode>(buffer[1]);

        // 检查异常响应
        if (static_cast<uint8_t>(response.function_code) & 0x80)
        {
            if (!read_with_timeout(buffer.data(), 1, timeout))
            {
                throw std::runtime_error("Incomplete exception response");
            }
            response.error = static_cast<ModbusError>(buffer[0]);
            return response;
        }

        // 根据功能码处理正常响应
        switch (response.function_code)
        {
        case FunctionCode::READ_HOLDING_REGISTERS:
        case FunctionCode::READ_INPUT_REGISTERS:
        {
            if (!read_with_timeout(buffer.data(), 1, timeout))
            {
                throw std::runtime_error("Incomplete read response");
            }
            uint8_t byte_count = buffer[0];
            if (!read_with_timeout(buffer.data() + 1, byte_count + 2, timeout))
            {
                throw std::runtime_error("Incomplete read data");
            }
            response.data.assign(buffer.begin() + 1, buffer.begin() + 1 + byte_count);
            SsModbusMaster::verify_crc(buffer.data(), byte_count + 3);
            break;
        }
        case FunctionCode::WRITE_SINGLE_REGISTER:
        {
            if (!read_with_timeout(buffer.data(), 6, timeout))
            {
                throw std::runtime_error("Incomplete write response");
            }
            SsModbusMaster::verify_crc(buffer.data(), 6);
            break;
        }
        case FunctionCode::WRITE_MULTIPLE_REGISTERS:
        {
            if (!read_with_timeout(buffer.data(), 6, timeout))
            {
                throw std::runtime_error("Incomplete write response");
            }
            SsModbusMaster::verify_crc(buffer.data(), 6);
            break;
        }
        default:
            throw std::runtime_error("Unsupported function code in response");
        }

        return response;
    }

    // 带超时的读取
    bool read_with_timeout(uint8_t *buffer, size_t length,
                           std::chrono::milliseconds timeout)
    {
        auto start = std::chrono::steady_clock::now();
        size_t total_read = 0;

        while (total_read < length)
        {
            size_t n = serialPort_.read(buffer + total_read, length - total_read);
            if (n > 0)
            {
                total_read += n;
            }
            else
            {
                if (std::chrono::steady_clock::now() - start > timeout)
                {
                    return false;
                }
                std::this_thread::sleep_for(10ms);
            }
        }

        return true;
    }
};

// ModbusRtuMaster包装实现
ModbusRtuMaster::ModbusRtuMaster(const std::string &port, uint32_t baudrate, Parity parity)
    : impl_(std::make_unique<Impl>(port, baudrate, parity)) {}

ModbusRtuMaster::~ModbusRtuMaster() = default;

ModbusResponse ModbusRtuMaster::send_request(const ModbusRequest &request,
                                             std::chrono::milliseconds timeout)
{
    return impl_->send_request(request, timeout);
}

} // namespace modbus