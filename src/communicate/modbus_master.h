/**
 * @file modbus_master.h
 * @brief Modbus主站抽象基类
 */

#pragma once

#include <chrono>
#include <vector>

#include "modbus_types.h"

namespace modbus
{

/**
 * @brief Modbus主站抽象基类
 */
class ModbusMaster
{
public:
    virtual ~ModbusMaster() = default;

    /**
     * @brief 发送Modbus请求
     * @param request 请求数据
     * @param timeout 超时时间
     * @return 响应数据
     */
    virtual ModbusResponse send_request(const ModbusRequest& request,
                                      std::chrono::milliseconds timeout) = 0;

    /**
     * @brief 读取保持寄存器
     * @param slave_address 从站地址
     * @param start_address 起始地址
     * @param register_count 寄存器数量
     * @param timeout 超时时间
     * @return 寄存器值
     */
    virtual std::vector<uint16_t> read_holding_registers(uint8_t slave_address,
                                                       uint16_t start_address,
                                                       uint16_t register_count,
                                                       std::chrono::milliseconds timeout);

    /**
     * @brief 写入单个寄存器
     * @param slave_address 从站地址
     * @param address 寄存器地址
     * @param value 寄存器值
     * @param timeout 超时时间
     */
    virtual void write_single_register(uint8_t slave_address,
                                     uint16_t address,
                                     uint16_t value,
                                     std::chrono::milliseconds timeout);

    /**
     * @brief 写入多个寄存器
     * @param slave_address 从站地址
     * @param start_address 起始地址
     * @param values 寄存器值数组
     * @param timeout 超时时间
     */
    virtual void write_multiple_registers(uint8_t slave_address,
                                        uint16_t start_address,
                                        const std::vector<uint16_t>& values,
                                        std::chrono::milliseconds timeout);
};

} // namespace modbus