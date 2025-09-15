/**
 * @file modbus_rtu_master.h
 * @brief Modbus-RTU主站类
 */

#pragma once

#include <string>
#include <memory>
#include "modbus_master.h"

namespace modbus
{

class ModbusRtuMaster : public SsModbusMaster
{
public:
    explicit ModbusRtuMaster(const std::string &port, uint32_t baudrate = 9600, Parity parity = Parity::NONE);
    ~ModbusRtuMaster() override;

    ModbusResponse send_request(const ModbusRequest &request,
                                std::chrono::milliseconds timeout) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace modbus