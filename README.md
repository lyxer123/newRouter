# 双W5500网卡4G路由器

## 概述

本项目实现的是4G模块和双W5500以太网模块的组合，支持4G、WiFi和双有线网络的共存。
具体实现4G->WiFi+双有线网络共存，或者4G+双有线网络->WiFi。双有线网络可以分别作为上行（WAN）和下行（LAN）接口使用。

本项目支持双W5500网卡通过共享SPI总线连接到ESP32-S3，实现商用路由器功能，其中：
- WiFi热点使用独立的IP子网192.168.5.x，网关为192.168.5.1
- 双W5500网卡共享另一个IP子网192.168.4.x，网关为192.168.4.1
- 4G模块作为备用WAN接口

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/4g_nic_en.png" style="zoom:80%;" />

### 开发环境

本项目基于ESP-IDF v5.0.6,V5.1.6,V5.2.5,V5.4.1,V5.5.1开发，请确保您的开发环境中安装了相应版本的ESP-IDF。

#### 如何使用示例

#### 硬件要求

**必需**
- 4G模块，EC600N模块
- ESP32S3模块或ESP32S3系列开发板
- 双W5500模块，通过共享SPI总线与ESP32S3连接
- Micro-USB线用于供电和编程

**可选**
- 一根以太网线
- 一根USB通信线
- 一些杜邦线用于连接MCU的SPI接口

**双W5500硬件连接**：
- **共享SPI总线**：
  - SCK = GPIO9
  - MOSI = GPIO3
  - MISO = GPIO46
- **卡1（W5500 Card 1）**：
  - CS0 = GPIO10
  - INT = GPIO12
  - RESET = GPIO38
- **卡2（W5500 Card 2）**：
  - CS1 = GPIO11
  - INT = GPIO13
  - RESET = GPIO34

请按照为此示例提供的详细说明进行操作。

#### 4G Cat.1 EC600N模块配置

EC600N是一款支持4G Cat.1的通信模块，通过USB接口连接到ESP32-S3。

**硬件连接:**
- 将EC600N模块通过USB线连接到ESP32-S3开发板

**menuconfig中的配置参数:**
```
Component config → Bridge Configuration → Modem Configuration
[*] Use 4G modem interface to connect to the external network
()  Modem reset control GPIO, set 0 if not use (NEW) = 13
(esp32) Set MODEM APN (NEW) = internet
(espressif) Set username for authentication (NEW) = espressif
(esp32) Set password for authentication (NEW) = esp32
[ ] SIM PIN needed (NEW)
(1234) Set SIM PIN (NEW) = 1234

Choose the interface of the modem (UART or USB) → USB
USB Configuration
(0x2C7C) USB Vendor ID (NEW) = 0x2C7C
(0x6002) USB Product ID (NEW) = 0x6002
(3) USB interface number (NEW) = 3
```

#### 双W5500以太网模块配置

双W5500是以太网控制器，通过共享SPI总线连接到ESP32-S3。

**menuconfig中的配置参数:**
```
Component config → Bridge Configuration → ETH Configuration
[*] Enable Ethernet interface
[*] SPI Ethernet

共享SPI总线配置：
Bridge ETH SPI Host Number (2) → 2
Bridge ETH SPI SCLK GPIO number (9) → 9
Bridge ETH SPI MOSI GPIO number (3) → 3
Bridge ETH SPI MISO GPIO number (46) → 46
Bridge ETH SPI clock speed (MHz) (16) → 16

卡1配置：
Bridge ETH SPI CS0 GPIO number for SPI Ethernet module #1 (10) → 10
Bridge ETH SPI Interrupt GPIO number SPI Ethernet module #1 (12) → 12
Bridge ETH SPI PHY Reset GPIO number of SPI Ethernet Module #1 (38) → 38
Bridge ETH SPI PHY Address of SPI Ethernet Module #1 (0) → 0

卡2配置：
Bridge ETH SPI CS1 GPIO number for SPI Ethernet module #2 (11) → 11
Bridge ETH SPI Interrupt GPIO number SPI Ethernet module #2 (13) → 13
Bridge ETH SPI PHY Reset GPIO number of SPI Ethernet Module #2 (34) → 34
Bridge ETH SPI PHY Address of SPI Ethernet Module #2 (1) → 1

网络接口配置：
[*] Ethernet acts as WAN or LAN automatically
[ ] Use Wi-Fi station interface to connect to the external network
[*] Use ethernet interface to provide network data forwarding for other devices
```

#### 网络接口配置

本项目支持多种网络接口配置，实现商用路由器功能：

**双W5500网卡配置**：
- **卡1**：可WAN可LAN（自动切换）
  - 连接上级路由器时自动作为WAN
  - 连接下位设备时自动作为LAN
  - 启用"Ethernet acts as WAN or LAN automatically"功能
- **卡2**：固定为LAN（连接下位计算机）
  - 始终作为数据转发接口
  - 始终启用DHCP服务器
  - 不参与自动切换

**IP地址分配策略**：
- **WiFi热点**：使用192.168.5.x子网，网关为192.168.5.1
- **双W5500网卡**：共享192.168.4.x子网，网关为192.168.4.1
- **4G模块**：作为备用WAN接口

**DHCP地址池**：
- WiFi热点：192.168.5.100-192.168.5.200
- 双W5500网卡：192.168.4.100-192.168.4.200

#### 选择用于为其他设备提供网络数据转发的接口

您可以在`menuconfig`的`Component config → Bridge Configuration → The interface used to provide network data forwarding for other devices`中选择接口（ETH/SPI/SDIO）连接到PC/MCU。

#### 构建和烧录
运行 `idf.py flash monitor` 来构建、烧录和监控项目。

完成完整的烧录过程后，您可以使用 `idf.py app-flash monitor` 来减少烧录时间。

如下是串口信息
```c
[0;32mI (1023) w5500.mac: version=0
[0;32mI (1036) esp_eth.netif.netif_glue: 02:00:00:12:34:56
[0;32mI (1036) esp_eth.netif.netif_glue: ethernet attached to netif
[0;32mI (1048) bridge_eth: Ethernet Started
[0;32mI (1049) bridge_eth: [ETH_LAN     ] Card0
[0;32mI (1050) bridge_eth: ETH Card0 IP Address:192.168.4.1
[0;32mI (1055) bridge_common: netif list add success
[0;32mI (1060) w5500.mac: version=0
[0;32mI (1065) esp_eth.netif.netif_glue: 02:00:00:12:34:57
[0;32mI (1070) esp_eth.netif.netif_glue: ethernet attached to netif
[0;32mI (1075) bridge_eth: Ethernet Started
[0;32mI (1080) bridge_eth: [ETH_LAN2    ] Card1
[0;32mI (1085) bridge_eth: ETH Card1 IP Address:192.168.4.1
[0;32mI (1090) bridge_common: netif list add success
[0;33mW (1100) bridge_modem: Force reset 4g board
[0;32mI (1105) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
[0;32mI (3049) bridge_eth: Ethernet Link Up
[0;32mI (3049) bridge_eth: Ethernet HW Addr 02:00:00:12:34:56
[0;32mI (6574) bridge_modem: Initializing esp_modem for the EC600N module...
[0;32mI (6575) bridge_modem: Waiting for USB device connection...
[0;32mI (8009) bridge_modem: Signal quality: rssi=20, ber=99
[0;32mI (8017) bridge_modem: Waiting for IP address
[0;32mI (8027) bridge_modem: Modem Connect to PPP Server
[0;32mI (8027) bridge_modem: ~~~~~~~~~~~~~~
[0;32mI (8028) bridge_modem: IP          : 10.44.120.176
[0;32mI (8032) bridge_modem: Netmask     : 255.255.255.255
[0;32mI (8037) bridge_modem: Gateway     : 10.64.64.64
[0;32mI (8043) bridge_modem: Name Server1: 61.128.128.68
[0;32mI (8048) bridge_modem: Name Server2: 61.128.192.68
[0;32mI (8053) bridge_modem: ~~~~~~~~~~~~~~
[0;32mI (8058) esp-netif_lwip-ppp: Connected
[0;32mI (8062) bridge_common: [WIFI_AP_DEF ]Name Server1: 61.128.128.68
[0;32mI (8069) bridge_common: [ETH_LAN     ]Name Server1: 61.128.128.68
[0;32mI (8076) bridge_common: [ETH_LAN2    ]Name Server1: 61.128.128.68
[0;32mI (8080) bridge_modem: GOT ip event!!!
[0;32mI (8085) bridge_modem: PPP state changed event 0
[0;32mI (8090) main_task: Returned from app_main()
```

#### 双W5500网卡实现详情

本项目实现了双W5500网卡支持，采用共享SPI总线方案：

1. **硬件连接**：
   - 两个W5500模块共享SPI总线（SPI2）
   - 每个模块有独立的CS、INT、RST信号
   - GPIO资源需求：10个（共享总线3个 + 独立信号7个）

2. **软件实现**：
   - 卡1支持自动WAN/LAN切换
   - 卡2固定为LAN模式
   - WiFi热点使用独立子网192.168.5.x
   - 双W5500网卡共享子网192.168.4.x

3. **IP地址分配**：
   - WiFi热点：192.168.5.1（网关）
   - 卡1和卡2：192.168.4.1（网关）
   - WiFi热点DHCP地址池：192.168.5.100-200
   - 双W5500网卡DHCP地址池：192.168.4.100-200

4. **路由优先级**：
   - 卡1作为WAN时：卡1 > 4G > 卡2
   - 卡1作为LAN时：4G > 卡1 > 卡2
