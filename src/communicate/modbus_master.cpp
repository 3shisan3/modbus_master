/**
 * @file modbus_master.cpp
 * @brief Modbus主站抽象基类实现
 */

#include "modbus_master.h"
#include <stdexcept>

namespace modbus
{

std::vector<uint16_t> ModbusMaster::read_holding_registers(uint8_t slave_address,
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

    if (response.error != ModbusError::NO_ERROR) {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }

    if (response.data.size() != register_count * 2) {
        throw std::runtime_error("Invalid response data size");
    }

    std::vector<uint16_t> result;
    for (size_t i = 0; i < response.data.size(); i += 2) {
        uint16_t value = (response.data[i] << 8) | response.data[i + 1];
        result.push_back(value);
    }

    return result;
}

void ModbusMaster::write_single_register(uint8_t slave_address,
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

    if (response.error != ModbusError::NO_ERROR) {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }
}

void ModbusMaster::write_multiple_registers(uint8_t slave_address,
                                          uint16_t start_address,
                                          const std::vector<uint16_t>& values,
                                          std::chrono::milliseconds timeout)
{
    ModbusRequest request;
    request.slave_address = slave_address;
    request.function_code = FunctionCode::WRITE_MULTIPLE_REGISTERS;
    request.start_address = start_address;
    request.register_count = values.size();
    request.values = values;

    ModbusResponse response = send_request(request, timeout);

    if (response.error != ModbusError::NO_ERROR) {
        throw std::runtime_error("Modbus error: " + std::to_string(static_cast<int>(response.error)));
    }
}

} // namespace modbus