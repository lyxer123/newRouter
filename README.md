# 4G路由器

## 概述

本项目实现的是4g模块和W5500模块的组合，实现4G和WIFI+有线网络的共存。
具体为4g->wifi+有线网络共存，或者4g+有线网络->wifi。其中有线网络要么做对下接内网，要么接路由器对上

<img src="https://raw.githubusercontent.com/espressif/esp-iot-bridge/master/components/iot_bridge/docs/_static/4g_nic_en.png" style="zoom:80%;" />

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
Bridge ETH SPI Interrupt GPIO number SPI Ethernet module #1 (11) → 11
Bridge ETH SPI PHY Reset GPIO number of SPI Ethernet Module #1 (12) → 12
Bridge ETH SPI PHY Address of SPI Ethernet Module #1 (1) → 1
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
[0;32mI (8043) bridge_modem: Name Server1: 61.128.128.68
[0;32mI (8048) bridge_modem: Name Server2: 61.128.192.68
[0;32mI (8053) bridge_modem: ~~~~~~~~~~~~~~
[0;32mI (8058) esp-netif_lwip-ppp: Connected
[0;32mI (8062) bridge_common: [WIFI_AP_DEF ]Name Server1: 61.128.128.68
[0;32mI (8069) bridge_common: [ETH_LAN     ]Name Server1: 61.128.128.68
[0;32mI (8076) bridge_modem: GOT ip event!!!
[0;32mI (8080) bridge_modem: PPP state changed event 0
[0;32mI (8087) main_task: Returned from app_main()
```
