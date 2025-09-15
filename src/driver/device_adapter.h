/***************************************************************
Copyright (c) 2022-2030, shisan233@sszc.live.
SPDX-License-Identifier: MIT 
File:        device_adapter.h
Version:     1.0
Author:      cjx
start date: 
Description: 设备适配器基类
    主要声明必要实现用于通讯过程中数据的处理的方法
Version history

[序号]    |   [修改日期]  |   [修改者]   |   [修改内容]

*****************************************************************/

#ifndef SSDEVICE_ADAPTER_H
#define SSDEVICE_ADAPTER_H

#include <cstdint>

#include "modbus_master.h"

namespace modbus
{

class SsDeviceAdapter
{
public:
    explicit SsDeviceAdapter(SsModbusMaster &master, uint8_t slave_address = 1, int time_out = 1000);

    ~SsDeviceAdapter();

    void changeTimeOut(int time_out);

protected:
    SsModbusMaster &m_master_;
    uint8_t m_slaveAddr_;
    std::chrono::milliseconds m_timeOut_;

    /********* 基础方法封装 *********/

    /**
     * @brief 读取单个或多个保持寄存器 (Modbus功能码03)
     * @param address 寄存器起始地址
     * @param count 要读取的寄存器数量(1-125)
     * @return 读取的数据(16位或32位)
     * @note 对于16位数据返回低16位有效，对于32位数据返回完整32位
     * @throw std::invalid_argument 当count不在1-125范围内
     * @throw ModbusException 当Modbus通信出错时
     */
    uint32_t read_holding_registers(uint16_t address, uint8_t count = 1);

    /**
     * @brief 写入单个保持寄存器 (Modbus功能码06)
     * @param address 寄存器地址
     * @param value 要写入的16位值
     * @throw ModbusException 当Modbus通信出错时
     */
    void write_single_register(uint16_t address, uint16_t value);

    /**
     * @brief 写入多个保持寄存器 (Modbus功能码16)
     * @param address 寄存器起始地址
     * @param values 要写入的数据数组
     * @throw std::invalid_argument 当values为空或太大
     * @throw ModbusException 当Modbus通信出错时
     */
    void write_multiple_registers(uint16_t address, const std::vector<uint16_t> &values);

    /**
     * @brief 读取32位无符号整数(两个连续寄存器)
     * @param address 寄存器起始地址
     * @return 读取的32位无符号整数
     * @throw ModbusException 当Modbus通信出错时
     */
    uint32_t read_uint32(uint16_t address)
    {
        return read_holding_registers(address, 2);
    }

    /**
     * @brief 写入32位无符号整数(两个连续寄存器)
     * @param address 寄存器起始地址
     * @param value 要写入的32位无符号整数
     * @throw ModbusException 当Modbus通信出错时
     */
    void write_uint32(uint16_t address, uint32_t value)
    {
        write_multiple_registers(address, {
            static_cast<uint16_t>((value >> 16) & 0xFFFF), // 高16位
            static_cast<uint16_t>(value & 0xFFFF)          // 低16位
        });
    }
};

} // namespace modbus

#endif  // SSDEVICE_ADAPTER_H