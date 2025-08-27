/**
 * @file serial_win.h
 * @brief Windows平台串口通信实现
 */

#pragma once
#include <windows.h>

#include <cstdint>
#include <string>

class WinSerialPort
{
public:
    WinSerialPort();
    ~WinSerialPort();

    /**
     * @brief 打开串口
     * @param port 串口名称 (如"COM1")
     * @param baudrate 波特率
     * @return 成功返回true，失败返回false
     */
    bool open(const std::string &port, uint32_t baudrate);

    /**
     * @brief 关闭串口
     */
    void close();

    /**
     * @brief 写入数据
     * @param data 数据指针
     * @param length 数据长度
     * @return 实际写入的字节数
     */
    size_t write(const uint8_t *data, size_t length);

    /**
     * @brief 读取数据
     * @param buffer 接收缓冲区
     * @param length 期望读取的长度
     * @return 实际读取的字节数
     */
    size_t read(uint8_t *buffer, size_t length);

    /**
     * @brief 检查串口是否打开
     * @return 打开返回true，否则返回false
     */
    bool isOpen() const;

private:
    HANDLE hSerial_;              ///< 串口句柄
    DCB dcbSerialParams_ = {0};   ///< 串口参数
    COMMTIMEOUTS timeouts_ = {0}; ///< 超时设置
    std::string port_;            ///< 串口名称
};