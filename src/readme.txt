架构梳理 & 使用方法
分通信和设备协议适配两个部分

外部使用，先继承 SsDeviceAdapter 适配补充操作设备的 寄存器地址，数据 等内容

整体使用先按设备的地址（串口地址 | 网络IP端口地址）构造 SsModbusMaster（对应的子类）
再构造具体的 SsDeviceAdapter（具体适配的子类），传入对应设备前一步的 SsModbusMaster

以此操作使用 SsDeviceAdapter（具体适配的子类）中接口，实现modbus命令下发，及响应处理