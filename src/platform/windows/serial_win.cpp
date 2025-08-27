/**
 * @file serial_win.cpp
 * @brief Windows平台串口通信实现
 */

#include "serial_win.h"

#include <stdexcept>

WinSerialPort::WinSerialPort() : hSerial_(INVALID_HANDLE_VALUE) {}

WinSerialPort::~WinSerialPort()
{
    close();
}

bool WinSerialPort::open(const std::string &port, uint32_t baudrate)
{
    if (isOpen())
    {
        close();
    }

    // 打开串口 (同步模式)
    hSerial_ = CreateFileA(
        ("\\\\.\\" + port).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hSerial_ == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // 获取当前串口参数
    if (!GetCommState(hSerial_, &dcbSerialParams_))
    {
        close();
        return false;
    }

    // 配置串口参数
    dcbSerialParams_.BaudRate = baudrate;
    dcbSerialParams_.ByteSize = 8;
    dcbSerialParams_.StopBits = ONESTOPBIT;
    dcbSerialParams_.Parity = NOPARITY;
    dcbSerialParams_.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(hSerial_, &dcbSerialParams_))
    {
        close();
        return false;
    }

    // 设置超时 (单位: 毫秒)
    timeouts_.ReadIntervalTimeout = 50;
    timeouts_.ReadTotalTimeoutConstant = 50;
    timeouts_.ReadTotalTimeoutMultiplier = 10;
    timeouts_.WriteTotalTimeoutConstant = 50;
    timeouts_.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial_, &timeouts_))
    {
        close();
        return false;
    }

    port_ = port;
    return true;
}

void WinSerialPort::close()
{
    if (isOpen())
    {
        CloseHandle(hSerial_);
        hSerial_ = INVALID_HANDLE_VALUE;
    }
}

size_t WinSerialPort::write(const uint8_t *data, size_t length)
{
    if (!isOpen())
        return 0;

    DWORD bytesWritten;
    if (!WriteFile(hSerial_, data, static_cast<DWORD>(length), &bytesWritten, NULL))
    {
        return 0;
    }
    return static_cast<size_t>(bytesWritten);
}

size_t WinSerialPort::read(uint8_t *buffer, size_t length)
{
    if (!isOpen())
        return 0;

    DWORD bytesRead;
    if (!ReadFile(hSerial_, buffer, static_cast<DWORD>(length), &bytesRead, NULL))
    {
        return 0;
    }
    return static_cast<size_t>(bytesRead);
}

bool WinSerialPort::isOpen() const
{
    return hSerial_ != INVALID_HANDLE_VALUE;
}