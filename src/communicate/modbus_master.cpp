/**
 * @file modbus_master.cpp
 * @brief Modbus主站抽象基类实现
 */

#include "modbus_master.h"
#include <stdexcept>

namespace modbus
{

std::vector<uint16_t> SsModbusMaster::read_holding_registers(uint8_t slave_address,
                                                             uint16_t start_address,
                                                             uint16_t register_count,
                                                             std::chrono::milliseconds timeout)
{
    ModbusRequest request;
    request.slave_address = slave_address;
    request.function_code = FunctionCode::READ_HOLDING_REGISTERS;
    request.start_address = start_address;
    request.register_count = register_count;

    ModbusResponse response = send_request(request, timeout);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }

    if (response.data.size() != register_count * 2)
    {
        throw std::runtime_error("Invalid response data size");
    }

    std::vector<uint16_t> result;
    for (size_t i = 0; i < response.data.size(); i += 2)
    {
        uint16_t value = (response.data[i] << 8) | response.data[i + 1];
        result.push_back(value);
    }

    return result;
}

void SsModbusMaster::write_single_register(uint8_t slave_address,
                                           uint16_t address,
                                           uint16_t value,
                                           std::chrono::milliseconds timeout)
{
    ModbusRequest request;
    request.slave_address = slave_address;
    request.function_code = FunctionCode::WRITE_SINGLE_REGISTER;
    request.start_address = address;
    request.values = {value};

    ModbusResponse response = send_request(request, timeout);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }
}

void SsModbusMaster::write_multiple_registers(uint8_t slave_address,
                                              uint16_t start_address,
                                              const std::vector<uint16_t> &values,
                                              std::chrono::milliseconds timeout)
{
    ModbusRequest request;
    request.slave_address = slave_address;
    request.function_code = FunctionCode::WRITE_MULTIPLE_REGISTERS;
    request.start_address = start_address;
    request.register_count = values.size();
    request.values = values;

    ModbusResponse response = send_request(request, timeout);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }
}

uint16_t SsModbusMaster::calculate_crc(const uint8_t *data, size_t length)
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

bool SsModbusMaster::verify_crc(const uint8_t *data, size_t length)
{
    if (length < 2)
        return false;

    uint16_t received_crc = (data[length - 1] << 8) | data[length - 2];
    return calculate_crc(data, length - 2) == received_crc;
}

// 添加16位数据到大端字节流
void append_uint16(std::vector<uint8_t> &frame, uint16_t value)
{
    frame.push_back((value >> 8) & 0xFF);
    frame.push_back(value & 0xFF);
}

std::vector<uint8_t> SsModbusMaster::build_request_frame(const ModbusRequest &request)
{
    std::vector<uint8_t> frame;

    // 地址域
    frame.push_back(request.slave_address);

    // 功能码
    frame.push_back(static_cast<uint8_t>(request.function_code));

    // 数据域 (根据功能码不同)
    switch (request.function_code)
    {
    case FunctionCode::READ_HOLDING_REGISTERS:
    case FunctionCode::READ_INPUT_REGISTERS:
        // 寄存器地址(2字节) + 数量(2字节)
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.register_count);
        break;

    case FunctionCode::WRITE_SINGLE_REGISTER:
        // 寄存器地址(2字节) + 值(2字节)
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.values[0]);
        break;

    case FunctionCode::WRITE_MULTIPLE_REGISTERS:
        // 寄存器地址(2字节) + 数量(2字节) + 字节数(1字节) + 值(n字节)
        append_uint16(frame, request.start_address);
        append_uint16(frame, request.register_count);
        frame.push_back(request.values.size() * 2);
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

size_t SsModbusMaster::get_actual_message_length(const uint8_t *data)
{
    switch (data[1])
    {
    case 0x03: // 读取保持寄存器
        return 3 + data[2] + 2;
    case 0x06: // 写单个寄存器
    case 0x10: // 写多个寄存器
        return 8;
    default:
        return 0;
    }
}

} // namespace modbus