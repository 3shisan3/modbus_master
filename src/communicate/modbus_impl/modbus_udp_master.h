/**
 * @file modbus_udp_master.h
 * @brief 简化版Modbus UDP主站实现
 */

#pragma once

#include <string>
#include <memory>
#include <chrono>
#include "modbus_master.h"

namespace modbus
{

class ModbusUdpMaster : public ModbusMaster
{
public:
    /**
     * @brief 构造函数
     * @param ip 目标设备IP地址
     * @param port 目标设备端口号
     */
    explicit ModbusUdpMaster(const std::string& ip, uint16_t port);
    ~ModbusUdpMaster() override;

    /**
     * @brief 发送Modbus请求并等待响应
     * @param request Modbus请求对象
     * @param timeout 超时时间(毫秒)
     * @return Modbus响应对象
     * @throw std::runtime_error 如果通信失败或超时
     */
    ModbusResponse send_request(const ModbusRequest& request,
                              std::chrono::milliseconds timeout) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace modbus