#include "device_adapter.h"

namespace modbus
{

SsDeviceAdapter::SsDeviceAdapter(SsModbusMaster &master, uint8_t slave_address, int time_out)
    : m_master_(master)
    , m_slaveAddr_(slave_address)
    , m_timeOut_(std::chrono::milliseconds(time_out))
{
    if (slave_address == 0 || slave_address > 128)
    {
        throw std::invalid_argument("Invalid slave address (1-128)");
    }
}

SsDeviceAdapter::~SsDeviceAdapter() = default;

void SsDeviceAdapter::changeTimeOut(int time_out)
{
    m_timeOut_ = std::chrono::milliseconds(time_out);
}

uint32_t SsDeviceAdapter::read_holding_registers(uint16_t address, uint8_t count)
{
    if (count == 0 || count > 125)
    {
        throw std::invalid_argument("Register count must be 1-125");
    }

    ModbusRequest request{
        .slave_address = m_slaveAddr_,
        .function_code = FunctionCode::READ_HOLDING_REGISTERS,
        .start_address = address,
        .register_count = count};

    ModbusResponse response = m_master_.send_request(request, m_timeOut_);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw ModbusException(response.error);
    }

    // 对于单寄存器返回16位数据
    if (count == 1)
    {
        return (response.data[0] << 8) | response.data[1];
    }

    // 对于双寄存器返回32位数据(大端序)
    if (count == 2)
    {
        return (response.data[0] << 24) |
               (response.data[1] << 16) |
               (response.data[2] << 8) |
               response.data[3];
    }

    // 对于多寄存器返回第一个寄存器的值(兼容旧代码)
    return (response.data[0] << 8) | response.data[1];
}

void SsDeviceAdapter::write_single_register(uint16_t address, uint16_t value)
{
    ModbusRequest request{
        .slave_address = m_slaveAddr_,
        .function_code = FunctionCode::WRITE_SINGLE_REGISTER,
        .start_address = address,
        .register_count = 1,
        .values = {value}};

    ModbusResponse response = m_master_.send_request(request, m_timeOut_);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw ModbusException(response.error);
    }
}

void SsDeviceAdapter::write_multiple_registers(uint16_t address, const std::vector<uint16_t> &values)
{
    if (values.empty() || values.size() > 123)
    {
        throw std::invalid_argument("Values count must be 1-123");
    }

    ModbusRequest request{
        .slave_address = m_slaveAddr_,
        .function_code = FunctionCode::WRITE_MULTIPLE_REGISTERS,
        .start_address = address,
        .register_count = static_cast<uint16_t>(values.size()),
        .values = values};

    ModbusResponse response = m_master_.send_request(request, m_timeOut_);

    if (response.error != ModbusError::NO_ERROR)
    {
        throw ModbusException(response.error);
    }
}

} // namespace modbus