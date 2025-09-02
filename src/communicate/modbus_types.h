/**
 * @file modbus_types.h
 * @brief Modbus协议相关类型定义
 */

#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace modbus
{

/**
 * @brief Modbus功能码枚举
 */
enum class FunctionCode : uint8_t
{
    READ_COILS = 0x01,              ///< 读线圈寄存器
    READ_DISCRETE_INPUTS = 0x02,    ///< 读离散输入寄存器
    READ_HOLDING_REGISTERS = 0x03,  ///< 读保持寄存器
    READ_INPUT_REGISTERS = 0x04,    ///< 读输入寄存器
    WRITE_SINGLE_COIL = 0x05,       ///< 写单个线圈
    WRITE_SINGLE_REGISTER = 0x06,   ///< 写单个寄存器
    WRITE_MULTIPLE_COILS = 0x0F,    ///< 写多个线圈
    WRITE_MULTIPLE_REGISTERS = 0x10 ///< 写多个寄存器
};

/**
 * @brief Modbus错误码枚举
 */
enum class ModbusError : uint8_t
{
    NO_ERROR = 0x00,                    ///< 无错误
    ILLEGAL_FUNCTION = 0x01,            ///< 非法功能码
    ILLEGAL_DATA_ADDRESS = 0x02,        ///< 非法数据地址
    ILLEGAL_DATA_VALUE = 0x03,          ///< 非法数据值
    SERVER_DEVICE_FAILURE = 0x04,       ///< 从站设备故障
    ACKNOWLEDGE = 0x05,                 ///< 确认
    SERVER_DEVICE_BUSY = 0x06,          ///< 从站设备忙
    MEMORY_PARITY_ERROR = 0x08,         ///< 内存奇偶错误
    GATEWAY_PATH_UNAVAILABLE = 0x0A,    ///< 网关路径不可用
    GATEWAY_TARGET_DEVICE_FAILED = 0x0B ///< 网关目标设备失败
};

/**
 * @brief Modbus异常类
 */
struct ModbusException : public std::runtime_error
{
    explicit ModbusException(ModbusError error)
        : std::runtime_error("Modbus error"), error_code(error) {}

    ModbusError error_code; ///< 错误码
};

/**
 * @brief Modbus请求结构体
 */
struct ModbusRequest
{
    uint8_t slave_address;        ///< 从站地址
    FunctionCode function_code;   ///< 功能码
    uint16_t start_address;       ///< 起始地址
    uint16_t register_count;      ///< 寄存器数量
    std::vector<uint16_t> values; ///< 写入值(用于写操作)
};

/**
 * @brief Modbus响应结构体
 */
struct ModbusResponse
{
    uint8_t slave_address;      ///< 从站地址
    FunctionCode function_code; ///< 功能码
    std::vector<uint8_t> data;  ///< 响应数据
    ModbusError error;          ///< 错误码
};

/**
 * @brief 校验位枚举
 */
enum class Parity
{
    NONE, ///< 无校验
    ODD,  ///< 奇校验
    EVEN  ///< 偶校验
};

} // namespace modbus