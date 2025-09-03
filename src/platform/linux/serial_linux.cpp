/**
 * @file serial_linux.cpp
 * @brief Linux平台串口通信实现
 */

#include "serial_linux.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

LinuxSerialPort::LinuxSerialPort() : fd_(-1) {}

LinuxSerialPort::~LinuxSerialPort()
{
    close();
}

bool LinuxSerialPort::open(const std::string &port, uint32_t baudrate)
{
    if (isOpen())
    {
        close();
    }

    // 打开串口设备 (非阻塞模式)
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0)
    {
        return false;
    }

    // 获取当前串口设置
    if (tcgetattr(fd_, &tty_) != 0)
    {
        close();
        return false;
    }

    // 设置输入输出波特率
    speed_t speed;
    switch (baudrate)
    {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        break;
    }
    cfsetispeed(&tty_, speed);
    cfsetospeed(&tty_, speed);

    // 设置常规配置
    tty_.c_cflag &= ~PARENB;        // 无奇偶校验
    tty_.c_cflag &= ~CSTOPB;        // 1位停止位
    tty_.c_cflag &= ~CSIZE;         // 清除数据位掩码
    tty_.c_cflag |= CS8;            // 8位数据位
    tty_.c_cflag &= ~CRTSCTS;       // 无硬件流控
    tty_.c_cflag |= CREAD | CLOCAL; // 启用接收，忽略控制线

    // 输入模式设置
    tty_.c_iflag &= ~(IXON | IXOFF | IXANY); // 关闭软件流控
    tty_.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // 输出模式设置
    tty_.c_oflag &= ~OPOST; // 原始输出
    tty_.c_oflag &= ~ONLCR;

    // 本地模式设置
    tty_.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // 超时设置 (单位: 0.1秒)
    tty_.c_cc[VTIME] = 1; // 1秒超时
    tty_.c_cc[VMIN] = 0;  // 最小读取0字节

    // 应用设置
    if (tcsetattr(fd_, TCSANOW, &tty_) != 0)
    {
        close();
        return false;
    }

    port_ = port;
    return true;
}

void LinuxSerialPort::close()
{
    if (isOpen())
    {
        ::close(fd_);
        fd_ = -1;
    }
}

size_t LinuxSerialPort::write(const uint8_t *data, size_t length)
{
    if (!isOpen())
        return 0;

    ssize_t written = ::write(fd_, data, length);
    if (written < 0)
    {
        return 0;
    }
    return static_cast<size_t>(written);
}

size_t LinuxSerialPort::read(uint8_t *buffer, size_t length)
{
    if (!isOpen())
        return 0;

    ssize_t bytesRead = ::read(fd_, buffer, length);
    if (bytesRead < 0)
    {
        return 0;
    }
    return static_cast<size_t>(bytesRead);
}

bool LinuxSerialPort::isOpen() const
{
    return fd_ >= 0;
}