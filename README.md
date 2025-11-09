# ESP-IoT-Bridge Dual W5500 Implementation

## 项目概述

本项目基于ESP-IDF v5.0.6,V5.1.6,V5.2.5,V5.4.1,V5.5.1开发，实现双W5500网卡功能，同时保留原有的4G转WiFi功能。项目支持多种网络接入方式的智能切换，可作为商用路由器使用。

### 网络接口配置

1. **WAN接口**（连接上级网络）：
   - W5500卡1（自动WAN/LAN切换）
   - WiFi Station（连接上级WiFi路由器）
   - 4G模块（EC600N，备用连接）

2. **LAN接口**：卡1作为LAN、卡2和WiFi热点共享同一个子网（192.168.4.x）

**IP地址分配**：
- 卡1作为WAN时：从上级路由器通过DHCP获取IP（如192.168.1.x网段）
- WiFi Station作为WAN时：从上级路由器通过DHCP获取IP（如192.168.1.x网段）
- 4G模块作为WAN时：从运营商通过DHCP获取IP（如10.x.x.x网段）
- 卡1作为LAN时：使用192.168.4.1（与卡2和WiFi热点共享子网）
- 卡2：固定使用192.168.4.1（与卡1作为LAN时和WiFi热点共享子网）
- WiFi热点：使用192.168.4.1（与卡1作为LAN时和卡2共享子网）

**DHCP服务**：
- 卡1作为LAN、卡2和WiFi热点共享DHCP地址池（192.168.4.100-192.168.4.200）
- 下位设备连接到任一接口时，都能获取到同一子网内的IP地址

### 开发环境

本项目基于ESP-IDF v5.0.6,V5.1.6,V5.2.5,V5.4.1,V5.5.1开发，请确保您的开发环境中安装了相应版本的ESP-IDF。

#### 如何使用示例

#### 硬件要求

**必需**
- 4G模块，EC600N模块
- ESP32S3模块或ESP32S3系列开发板
- W5500模块，通过SPI与ESP32S3连接
- Micro-USB线用于供电和编程

**可选**
- 一根以太网线
- 一根USB通信线
- 一些杜邦线用于连接MCU的SPI或SDIO接口

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

#### W5500以太网模块配置

W5500是以太网控制器，通过SPI接口连接到ESP32。

**menuconfig中的配置参数:**
```
Component config → Bridge Configuration → ETH Configuration
[*] Enable Ethernet interface (NEW)
[*] SPI Ethernet (NEW)

Bridge ETH SPI Host Number (1) → 1
Bridge ETH SPI SCLK GPIO number (9) → 9
Bridge ETH SPI MOSI GPIO number (3) → 3
Bridge ETH SPI MISO GPIO number (46) → 46
Bridge ETH SPI clock speed (MHz) (23) → 23   (最大值60MHz)
Bridge ETH SPI CS0 GPIO number for SPI Ethernet module #1 (10) → 10
Bridge ETH SPI Interrupt GPIO number SPI Ethernet module #1 (12) → 12
Bridge ETH SPI PHY Reset GPIO number of SPI Ethernet Module #1 (38) → 38
Bridge ETH SPI PHY Address of SPI Ethernet Module #1 (1) → 1

# 卡2配置
Bridge ETH SPI CS1 GPIO number for SPI Ethernet module #2 (11) → 11
Bridge ETH SPI Interrupt GPIO number SPI Ethernet module #2 (13) → 13
Bridge ETH SPI PHY Reset GPIO number of SPI Ethernet Module #2 (34) → 34
Bridge ETH SPI PHY Address of SPI Ethernet Module #2 (1) → 1
```

#### 修改bridge_modem.c文件（espressif__iot_bridge 0.11.9 版本）

为了支持EC600N模块，需要修改bridge_modem.c文件中的设备类型：

文件路径: `managed_components/espressif__iot_bridge/src/bridge_modem.c`

修改内容:
1. 将设备类型从ESP_MODEM_DCE_BG96改为ESP_MODEM_DCE_GENERIC
2. 更新日志信息以反映使用的是EC600N模块

修改前:
```c
ESP_LOGI(TAG, "Initializing esp_modem for the BG96 module...");
esp_modem_dce_t *dce = esp_modem_new_dev_usb(ESP_MODEM_DCE_BG96, &dte_usb_config, &dce_config, esp_netif);
```

修改后:
```c
ESP_LOGI(TAG, "Initializing esp_modem for the EC600N module...");
esp_modem_dce_t *dce = esp_modem_new_dev_usb(ESP_MODEM_DCE_GENERIC, &dte_usb_config, &dce_config, esp_netif);
```

#### 选择用于为其他设备提供网络数据转发的接口

您可以在`menuconfig`的`Component config → Bridge Configuration → The interface used to provide network data forwarding for other devices`中选择接口（ETH/SPI/SDIO）连接到PC/MCU。

#### 构建和烧录
运行 `idf.py flash monitor` 来构建、烧录和监控项目。

完成完整的烧录过程后，您可以使用 `idf.py app-flash monitor` 来减少烧录时间。

如下是串口信息
```c
[0;31mE (999) ksz8851snl-mac: emac_ksz8851_init(260): verify chip id failed
[0;32mI (1006) gpio: GPIO[11]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
[0;31mE (1016) esp_eth: esp_eth_driver_install(228): init mac failed
[0;32mI (1023) w5500.mac: version=0
[0;32mI (1036) esp_eth.netif.netif_glue: 02:00:00:12:34:56
[0;32mI (1036) esp_eth.netif.netif_glue: ethernet attached to netif
[0;32mI (1048) bridge_eth: Ethernet Started
[0;32mI (1049) bridge_eth: [ETH_LAN     ]
Add netif eth with b45c94b(commit id)

[0;32mI (1050) bridge_common: netif list add success
[0;32mI (1055) bridge_eth: ETH IP Address:192.168.4.1
[0;33mW (1060) bridge_modem: Force reset 4g board
[0;32mI (1065) gpio: GPIO[13]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
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
[0;32mI (8043) bridge_modem: Name Server1: 61.128.12