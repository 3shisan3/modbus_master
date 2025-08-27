/**
 * @file serial_linux.h
 * @brief Linux平台串口通信实现
 */

#pragma once
#include <termios.h>
#include <string>
#include <vector>

class LinuxSerialPort
{
public:
    LinuxSerialPort();
    ~LinuxSerialPort();

    /**
     * @brief 打开串口
     * @param port 串口设备路径，如"/dev/ttyUSB0"
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
    int fd_;             ///< 文件描述符
    struct termios tty_; ///< 终端设置
    std::string port_;   ///< 串口设备路径
};