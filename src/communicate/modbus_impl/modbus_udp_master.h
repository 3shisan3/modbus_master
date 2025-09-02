#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "modbus_master.h"

namespace modbus
{

// 前向声明
class ModbusUdpResponseHandler;

/**
 * @class ModbusUdpMaster
 * @brief 增强版Modbus UDP主站实现
 */
class ModbusUdpMaster : public ModbusMaster
{
public:
    /**
     * @struct CommunicationStatus
     * @brief 通信状态统计结构
     */
    struct CommunicationStatus {
        uint32_t total_queries = 0;      ///< 总查询次数
        uint32_t failed_queries = 0;     ///< 失败查询次数
        uint32_t total_controls = 0;     ///< 总控制命令次数
        uint32_t failed_controls = 0;    ///< 失败控制命令次数
        std::chrono::milliseconds avg_response_time{0}; ///< 平均响应时间(毫秒)
    };

    /**
     * @brief 构造函数，创建ModbusUdpMaster实例
     * @param ip 目标设备IP地址
     * @param port 目标设备端口号
     * @throw std::runtime_error 如果订阅响应消息失败
     */
    explicit ModbusUdpMaster(const std::string& ip, uint16_t port);

    /**
     * @brief 析构函数
     */
    ~ModbusUdpMaster() override;

    /**
     * @brief 发送查询请求并等待响应
     * @param request Modbus请求对象
     * @param timeout 超时时间(毫秒)，默认为1000毫秒
     * @return Modbus响应对象
     * @throw std::runtime_error 如果发送请求失败或响应超时
     * @note 此方法会自动控制查询频率，避免超过设定的轮询间隔
     */
    ModbusResponse query(const ModbusRequest& request,
                       std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    /**
     * @brief 异步发送控制命令，不等待响应
     * @param request Modbus请求对象
     * @note 此方法不会阻塞调用线程，适合不需要确认响应的控制场景
     */
    void control_async(const ModbusRequest& request);

    /**
     * @brief 批量发送控制命令
     * @param requests Modbus请求对象列表
     * @note 此方法会依次发送所有请求，但不保证所有请求都成功执行
     */
    void control_batch(const std::vector<ModbusRequest>& requests);

    /**
     * @brief 设置最小轮询间隔
     * @param interval 最小轮询间隔时间(毫秒)
     * @note 用于控制查询频率，防止对设备造成过大负载
     */
    void set_polling_interval(std::chrono::milliseconds interval);

    /**
     * @brief 获取当前通信状态统计信息
     * @return CommunicationStatus 包含各种统计信息的结构体
     */
    CommunicationStatus get_status() const;

    /**
     * @brief 发送Modbus请求并接收响应
     * @param request Modbus请求对象
     * @param timeout 超时时间(毫秒)
     * @return Modbus响应对象
     */
    ModbusResponse send_request(const ModbusRequest& request,
                              std::chrono::milliseconds timeout) override;

private:
    class Impl;
    friend class ModbusUdpResponseHandler; // 声明响应处理器为友元
    std::unique_ptr<Impl> impl_;  ///< PIMPL实现指针
};

} // namespace modbus