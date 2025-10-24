# 使用CH390以太网模块

本文档说明了如何在本项目中使用CH390以太网模块替换原有的W5500模块。

## 硬件连接

CH390模块通过SPI接口连接到ESP32-S3，连接方式如下：

| CH390引脚 | ESP32-S3引脚 | 功能说明 |
|----------|-------------|---------|
| VCC      | 3.3V        | 电源正极 |
| GND      | GND         | 电源地 |
| SCLK     | GPIO6       | SPI时钟 |
| MOSI     | GPIO7       | SPI主出从入 |
| MISO     | GPIO15      | SPI主入从出 |
| CS       | GPIO5       | 片选信号 |
| INT      | GPIO4       | 中断信号 |
| RST      | GPIO16      | 复位信号 |

## 软件配置

### 1. 添加CH390组件依赖

项目已通过 `idf_component.yml` 文件添加了CH390组件依赖：

```yaml
dependencies:
  idf: '>=5.0'
  espressif/ch390: '0.3.0'
```

### 2. 配置sdkconfig

项目已自动配置sdkconfig以使用CH390模块：

```
CONFIG_ETH_SPI_ETHERNET_CH390=y
# CONFIG_ETH_SPI_ETHERNET_W5500 is not set
```

### 3. 桥接配置

ETH Configuration已设置为：

```
CONFIG_BRIDGE_USE_SPI_ETHERNET=y
CONFIG_BRIDGE_ETH_SPI_HOST=2
CONFIG_BRIDGE_ETH_SPI_SCLK_GPIO=6
CONFIG_BRIDGE_ETH_SPI_MOSI_GPIO=7
CONFIG_BRIDGE_ETH_SPI_MISO_GPIO=15
CONFIG_BRIDGE_ETH_SPI_CLOCK_MHZ=25
CONFIG_BRIDGE_ETH_SPI_CS0_GPIO=5
CONFIG_BRIDGE_ETH_SPI_INT0_GPIO=4
CONFIG_BRIDGE_ETH_SPI_PHY_RST0_GPIO=16
CONFIG_BRIDGE_ETH_SPI_PHY_ADDR0=1
```

## 编译和烧录

1. 确保ESP-IDF环境已正确设置
2. 连接ESP32-S3开发板到电脑
3. 在项目根目录下运行以下命令：

```bash
idf.py build
idf.py flash monitor
```

## 验证连接

烧录完成后，可以通过以下方式验证CH390模块是否正常工作：

1. 查看串口输出，应能看到以太网连接成功的日志
2. 使用电脑通过网线连接到CH390模块
3. 设备应能获取到IP地址并通过以太网访问互联网

## 注意事项

1. CH390模块需要3.3V供电，请确保电源稳定
2. SPI时钟频率设置为25MHz，在某些情况下可以适当调整
3. 如果遇到连接问题，请检查硬件连接是否正确
4. 确保使用的GPIO引脚没有与其他功能冲突