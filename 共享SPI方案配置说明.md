# 共享SPI方案配置说明

## 一、方案概述

本项目采用**共享SPI方案**实现双W5500网卡功能。两个W5500设备共享SPI总线（SCK、MOSI、MISO），使用独立的CS、INT、RST信号进行设备区分和控制。

## 二、硬件连接配置

### 2.1 GPIO引脚分配

**共享SPI总线**（两个W5500共用）：
- **SCK** = GPIO9
- **MOSI** = GPIO3
- **MISO** = GPIO46

**W5500卡1（Card 1）**：
- **CS0** = GPIO10
- **INT** = GPIO12
- **RESET** = GPIO45

**W5500卡2（Card 2）**：
- **CS0** = GPIO11
- **INT** = GPIO13
- **RESET** = GPIO34

### 2.2 硬件连接示意图

```
ESP32S3                    W5500卡1              W5500卡2
--------                   -------              -------
GPIO9  (SCK)  ─────┬────── SCK                  SCK
GPIO3  (MOSI) ─────┼────── MOSI                 MOSI
GPIO46 (MISO) ─────┼────── MISO                 MISO
                   │
GPIO10 (CS0)  ─────┴────── CS                   (不连接)
GPIO12 (INT)  ──────────── INT                  (不连接)
GPIO45 (RST)  ──────────── RESET                (不连接)

GPIO11 (CS1)  ────────────────────────────────── CS
GPIO13 (INT1) ────────────────────────────────── INT
GPIO34 (RST1) ────────────────────────────────── RESET
```

## 三、Kconfig配置项

### 3.1 共享SPI总线配置（卡1和卡2共用）

在`menuconfig`中配置：
```
Component config → Bridge Configuration → ETH Configuration
```

**SPI总线配置**：
- `SPI Host`：选择SPI主机（例如SPI2）
- `SPI SCLK GPIO`：9
- `SPI MOSI GPIO`：3
- `SPI MISO GPIO`：46

### 3.2 卡1配置

**控制信号配置**：
- `SPI CS0 GPIO number for SPI Ethernet module #1`：10
- `Interrupt GPIO number SPI Ethernet module #1`：12
- `PHY Reset GPIO number of SPI Ethernet Module #1`：45
- `PHY Address of SPI Ethernet Module #1`：0（默认）

### 3.3 卡2配置

**控制信号配置**：
- `SPI CS1 GPIO number for SPI Ethernet module #2`：11
- `Interrupt GPIO number SPI Ethernet module #2`：13
- `PHY Reset GPIO number of SPI Ethernet Module #2`：34
- `PHY Address of SPI Ethernet Module #2`：1（默认）

## 四、配置验证

### 4.1 使用menuconfig验证

```bash
idf.py menuconfig
```

导航到：`Component config → Bridge Configuration → ETH Configuration`

验证以下配置：
- ✅ SPI总线配置：SCK=GPIO9, MOSI=GPIO3, MISO=GPIO46
- ✅ 卡1配置：CS=GPIO10, INT=GPIO12, RST=GPIO45
- ✅ 卡2配置：CS=GPIO11, INT=GPIO13, RST=GPIO34

### 4.2 检查sdkconfig文件

```bash
grep -E "BRIDGE_ETH_SPI_(HOST|SCLK|MOSI|MISO|CS[01]|INT[01]|PHY_RST[01]|PHY_ADDR[01])" sdkconfig
```

预期输出：
```
CONFIG_BRIDGE_ETH_SPI_HOST=2
CONFIG_BRIDGE_ETH_SPI_SCLK_GPIO=9
CONFIG_BRIDGE_ETH_SPI_MOSI_GPIO=3
CONFIG_BRIDGE_ETH_SPI_MISO_GPIO=46
CONFIG_BRIDGE_ETH_SPI_CS0_GPIO=10
CONFIG_BRIDGE_ETH_SPI_INT0_GPIO=12
CONFIG_BRIDGE_ETH_SPI_PHY_RST0_GPIO=45
CONFIG_BRIDGE_ETH_SPI_PHY_ADDR0=0
CONFIG_BRIDGE_ETH_SPI_CS1_GPIO=11
CONFIG_BRIDGE_ETH_SPI_INT1_GPIO=13
CONFIG_BRIDGE_ETH_SPI_PHY_RST1_GPIO=34
CONFIG_BRIDGE_ETH_SPI_PHY_ADDR1=1
```

## 五、工作原理

### 5.1 SPI总线共享机制

1. **SPI总线初始化**：
   - SPI总线只初始化一次（在初始化第一个W5500时）
   - 使用共享的SCK、MOSI、MISO引脚

2. **设备区分**：
   - 使用独立的CS（片选）信号区分设备
   - CS0（GPIO10）用于卡1
   - CS1（GPIO11）用于卡2

3. **总线仲裁**：
   - ESP-IDF SPI驱动自动处理总线仲裁
   - 通过CS信号切换，确保同一时间只有一个设备访问总线
   - 驱动层自动管理CS信号，无需手动控制

### 5.2 设备初始化顺序

1. **初始化SPI总线**（仅一次）
2. **初始化W5500卡1**：
   - 添加SPI设备（使用CS0=GPIO10）
   - 配置INT和RST引脚
   - 启动以太网驱动
3. **初始化W5500卡2**：
   - 添加SPI设备（使用CS1=GPIO11）
   - 配置INT和RST引脚
   - 启动以太网驱动

### 5.3 数据通信

- **串行通信**：两个W5500设备通过共享SPI总线串行通信
- **CS切换**：ESP-IDF SPI驱动自动切换CS信号，确保同一时间只有一个设备访问总线
- **性能限制**：总带宽受SPI总线速度限制（30-50Mbps）

## 六、性能特点

### 6.1 优势

- ✅ **GPIO资源节省**：只需9个GPIO（共享总线3个 + 独立控制6个）
- ✅ **实现简单**：ESP-IDF SPI驱动自动处理总线仲裁
- ✅ **硬件成本低**：无需额外的SPI接口资源

### 6.2 限制

- ⚠️ **性能受限**：串行通信，总带宽受SPI总线速度限制
- ⚠️ **延迟较高**：CS切换和总线仲裁会增加延迟
- ⚠️ **并发能力受限**：两个设备无法真正并行传输

### 6.3 性能预期

- **总带宽**：30-50Mbps
- **延迟**：10-50ms（取决于负载）
- **适用场景**：中等负载应用（5-10台设备）

## 七、故障排查

### 7.1 常见问题

**问题1：两个W5500无法同时初始化**
- **原因**：CS信号配置错误或冲突
- **解决**：检查CS0和CS1的GPIO配置是否正确

**问题2：数据通信异常**
- **原因**：SPI总线速度过高或CS切换时序问题
- **解决**：降低SPI时钟速度，检查CS信号切换时序

**问题3：GPIO冲突**
- **原因**：GPIO引脚被其他功能占用
- **解决**：检查GPIO配置，确保没有冲突

### 7.2 调试方法

1. **使用示波器检查**：
   - 检查共享SPI总线信号（SCK、MOSI、MISO）
   - 检查CS0和CS1信号切换
   - 检查INT0和INT1信号

2. **查看串口日志**：
   - 检查SPI总线初始化信息
   - 检查W5500初始化信息
   - 检查GPIO配置错误

3. **功能测试**：
   - 单独测试卡1和卡2
   - 测试同时工作场景
   - 测试数据转发功能

## 八、总结

共享SPI方案是一个实用的解决方案，适合GPIO资源受限的场景。虽然性能受到一定限制，但实现简单，成本低，能够满足中等负载应用的需求。

关键要点：
- ✅ 两个W5500共享SPI总线（SCK、MOSI、MISO）
- ✅ 使用独立的CS、INT、RST信号区分设备
- ✅ ESP-IDF SPI驱动自动处理总线仲裁
- ✅ 性能受SPI总线速度限制（30-50Mbps）

