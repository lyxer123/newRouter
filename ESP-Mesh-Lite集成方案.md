# ESP-Mesh-Lite集成方案

## 1. 项目背景与目标

### 1.1 项目背景
当前项目已实现以下功能：
- 4G模块（EC600N）通过USB接口连接ESP32S3，作为主要的互联网接入方式
- 双W5500以太网接口，其中卡1支持自动WAN/LAN切换，卡2固定作为LAN
- WiFi SoftAP功能，为其他设备提供网络数据转发
- WiFi Station功能，可作为WAN接口连接上级路由器

### 1.2 集成目标
在现有项目基础上集成ESP-Mesh-Lite功能，实现：
1. 两个ESP32S3设备之间通过WiFi组成Mesh网络
2. 每个设备都通过4G连接上网
3. 当一个设备的4G断开时，其他设备可通过WiFi Mesh网络实现上网
4. 保持现有所有功能不变

## 2. ESP-Mesh-Lite技术分析

### 2.1 ESP-Mesh-Lite简介
ESP-Mesh-Lite是基于ESP-IDF的WiFi Mesh网络解决方案，具有以下特点：
- 基于SoftAP + Station模式构建
- 支持自组织、自修复网络
- 允许子设备独立访问外部网络
- 传输信息对父节点不敏感，降低应用层开发难度

### 2.2 核心功能
- 支持Mesh网络自愈功能
- 支持无路由器情况下由根节点建立Mesh网络
- 支持设备自动加入Mesh网络
- 支持节点角色配置（根节点/非根节点）
- 支持网络加入控制
- 支持设备合法性检查
- 支持节点间通信（根节点与子节点、广播通信等）
- 支持数据通信加密

### 2.3 API接口
主要API接口：
```c
esp_bridge_create_all_netif();
esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
esp_mesh_lite_init(&mesh_lite_config);
esp_mesh_lite_start();
```

## 3. 集成方案设计

### 3.1 WiFi功能保留分析

根据对现有项目代码和ESP-Mesh-Lite技术文档的分析，ESP-Mesh-Lite集成后原有WiFi功能可以完全保留：

1. **WiFi Station连接路由器上网**：设备可以继续连接到上级路由器热点
2. **WiFi SoftAP提供网络服务**：设备可以继续为下级设备（手机、电脑等）提供网络接入
3. **网络数据转发**：原有的网络桥接功能不受影响
4. **4G网络接入**：4G模块作为主要或备用网络接入方式

ESP-Mesh-Lite的实现机制与现有WiFi功能完全兼容，因为它同样基于SoftAP + Station模式构建。

#### 3.1.1 功能保留机制

ESP-Mesh-Lite与现有WiFi功能的兼容性基于以下机制：

1. **共存模式**：ESP-Mesh-Lite在Station + SoftAP模式下运行，与现有配置一致
2. **自动切换**：当检测到上级网络不可用时，自动切换到Mesh网络
3. **路由优先级**：可以设置不同网络接口的优先级（4G > 路由器 > Mesh网络）
4. **无缝切换**：网络切换过程中，下级设备的网络服务不会中断

#### 3.1.2 网络架构增强

集成ESP-Mesh-Lite后，网络架构将得到增强：

1. **多重冗余**：4G、路由器、Mesh网络提供多重网络接入保障
2. **自动故障转移**：当主网络断开时，自动切换到备用网络
3. **扩展覆盖范围**：通过Mesh网络扩展WiFi覆盖范围
4. **保持服务连续性**：即使主网络断开，仍能为下级设备提供网络服务

根据对现有项目代码和ESP-Mesh-Lite技术文档的分析，ESP-Mesh-Lite集成后原有WiFi功能可以完全保留：

1. **WiFi Station连接路由器上网**：设备可以继续连接到上级路由器热点
2. **WiFi SoftAP提供网络服务**：设备可以继续为下级设备（手机、电脑等）提供网络接入
3. **网络数据转发**：原有的网络桥接功能不受影响
4. **4G网络接入**：4G模块作为主要或备用网络接入方式

ESP-Mesh-Lite的实现机制与现有WiFi功能完全兼容，因为它同样基于SoftAP + Station模式构建。

### 3.2 WiFi热点配置（SoftAP）

#### 3.2.1 当前配置方式

在集成ESP-Mesh-Lite后，ESP32S3的WiFi热点（SoftAP）配置保持不变，仍然通过以下方式配置：

```c
#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = CONFIG_BRIDGE_SOFTAP_SSID,
            .password = CONFIG_BRIDGE_SOFTAP_PASSWORD,
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
#endif
```

这些配置值定义在`sdkconfig`文件中：
- SSID: `CONFIG_BRIDGE_SOFTAP_SSID` (默认为 "ESP_Bridge")
- 密码: `CONFIG_BRIDGE_SOFTAP_PASSWORD` (默认为 "12345678")

#### 3.2.2 配置修改方法

可以通过以下几种方式修改WiFi热点的SSID和密码：

1. **通过menuconfig配置界面**：
   ```bash
   idf.py menuconfig
   ```
   导航到：
   - Bridge Configuration → SoftAP Config
   - 修改"SoftAP SSID"和"SoftAP Password"值

2. **直接在代码中修改**：
   ```c
   #if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
       wifi_config_t wifi_cfg = {
           .ap = {
               .ssid = "您的自定义SSID",
               .password = "您的自定义密码",
           }
       };
       esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
   #endif
   ```

3. **使用esp_bridge_wifi_set API**：
   ```c
   esp_bridge_wifi_set(WIFI_MODE_AP, "您的自定义SSID", "您的自定义密码", NULL);
   ```

#### 3.2.3 MAC地址后缀配置

默认情况下，SoftAP的SSID会自动添加设备MAC地址后缀，以区分不同设备：
- 配置项：`CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC=y`
- 格式：原SSID + "_" + MAC地址后3字节（如：ESP_Bridge_a1b2c3）

### 3.3 Mesh网络与上级路由器共存

#### 3.3.1 共存机制

ESP-Mesh-Lite可以与上级路由器数据完全共存：

1. **WiFi Station连接**：设备可以通过WiFi Station接口连接到上级路由器获取互联网访问
2. **Mesh网络通信**：设备同时参与Mesh网络，与其他Mesh节点通信
3. **独立工作**：两种连接方式独立工作，互不干扰

#### 3.3.2 网络优先级设计

为确保网络连接的智能切换，系统采用以下优先级设计：
- 4G网络：优先级最高（route_prio = 50）
- Mesh网络：优先级中等（route_prio = 30）
- WiFi Station接口：优先级较低（route_prio = 20）
- 以太网接口：优先级较低（route_prio = 10）
- WiFi SoftAP接口：优先级最低（route_prio = 10）

#### 3.3.3 工作场景

1. **正常工作场景**：
   - 设备通过WiFi Station连接到上级路由器
   - 同时参与Mesh网络，为其他Mesh节点提供通信路径
   - 下级设备通过SoftAP连接到本设备

2. **路由器断开场景**：
   - WiFi Station连接断开
   - Mesh网络成为主要通信路径
   - 通过其他Mesh节点访问互联网（如果其他节点仍有连接）

3. **4G和路由器都断开场景**：
   - Mesh网络成为唯一通信路径
   - 设备间通过Mesh网络保持通信
   - 下级设备仍可通过SoftAP连接到本设备

### 3.4 网络接口配置示例

#### 3.4.1 多接口协同工作配置

```c
// WiFi SoftAP配置（本地热点）
#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = "MyDevice_AP",
            .password = "MySecurePassword",
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
#endif

// WiFi Station配置（连接上级路由器）
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    wifi_config_t sta_cfg = {
        .sta = {
            .ssid = "UpstreamRouter",
            .password = "RouterPassword",
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_STA, &sta_cfg);
    esp_wifi_connect();
#endif
```

#### 3.4.2 动态配置更新

可以通过以下方式动态更新WiFi配置：

```c
// 更新SoftAP配置
esp_bridge_wifi_set(WIFI_MODE_AP, "NewSSID", "NewPassword", NULL);

// 更新Station配置
esp_bridge_wifi_set(WIFI_MODE_STA, "NewRouterSSID", "NewRouterPassword", NULL);
```

### 3.5 网络架构设计
```
设备A (4G连接正常)           设备B (4G断开)
    |                           |
[4G模块EC600N]              [4G模块EC600N]
    |                           |
[ESP32S3主控] <--WiFi Mesh--> [ESP32S3主控]
    |                           |
[W5500网卡]                 [W5500网卡]
    |                           |
[下位设备] <--Internet-->   [下位设备]
```

#### 3.5.1 增强型网络架构

```
互联网
  |
  | (WiFi/4G)
  ↓
设备A (根节点) ←→ 设备B (子节点) ←→ 设备C (子节点)
  |                 |                 |
[SoftAP]        [SoftAP]          [SoftAP]
  |                 |                 |
手机/电脑        手机/电脑          手机/电脑
```

在这种架构下，即使某个设备的主网络连接断开，它仍然可以通过Mesh网络经由其他设备访问互联网，同时继续为下级设备提供网络服务。

### 3.6 工作原理
1. 设备A和设备B都启用ESP-Mesh-Lite功能
2. 设备A作为根节点（4G连接正常），设备B作为子节点（4G断开）
3. 设备B通过Mesh网络将数据转发给设备A
4. 设备A通过4G网络访问互联网
5. 返回数据通过Mesh网络转发给设备B

### 3.7 路由优先级设计
- 4G网络：优先级最高（route_prio = 50）
- Mesh网络：优先级中等（route_prio = 30）
- 以太网/WiFi Station：优先级较低（route_prio = 10）

## 4. 开发步骤

### 步骤1：添加ESP-Mesh-Lite组件依赖

#### 4.1.1 修改idf_component.yml文件
在[main/idf_component.yml](file:///e:/github/newRouter/main/idf_component.yml)中添加mesh_lite依赖：
```yaml
dependencies:
  espressif/iot_bridge:
    version: '*'
  espressif/mesh_lite:
    version: '*'
  idf: '>=5.0'
  # 其他现有依赖保持不变
```

#### 4.1.2 验证依赖添加
运行以下命令验证依赖是否正确添加：
```bash
idf.py reconfigure
```

### 步骤2：配置ESP-Mesh-Lite参数

#### 4.2.1 启用Mesh功能配置
在`menuconfig`中配置：
```bash
idf.py menuconfig
```

导航到以下路径并启用相应配置：
- Component config → ESP-Mesh-Lite Configuration
  - [*] Enable ESP-Mesh-Lite
  - [*] Enable root node auto-start
  - [*] Enable node type auto-detection
  - (MyMesh) Set Mesh network SSID
  - (12345678) Set Mesh network password

#### 4.2.2 配置网络接口优先级
确保4G网络优先级高于Mesh网络：
- 4G模块：route_prio = 50
- Mesh网络：route_prio = 30
- 其他接口：route_prio = 10

### 步骤3：修改主程序代码

#### 4.3.1 修改app_main.c文件
在[main/app_main.c](file:///e:/github/newRouter/main/app_main.c)中集成Mesh功能：

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_bridge.h"
#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
#include "web_server.h"
#endif
#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
#include "wifi_prov_mgr.h"
#endif

// 添加Mesh-Lite头文件
#include "esp_mesh_lite.h"

static const char *TAG = "main";

static esp_err_t esp_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建所有网络接口
    esp_bridge_create_all_netif();

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = CONFIG_BRIDGE_SOFTAP_SSID,
            .password = CONFIG_BRIDGE_SOFTAP_PASSWORD,
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    esp_wifi_connect();
#endif

    // 初始化并启动Mesh-Lite
    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    esp_mesh_lite_init(&mesh_lite_config);
    esp_mesh_lite_start();

#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
    StartWebServer();
#endif /* CONFIG_APP_BRIDGE_USE_WEB_SERVER */

#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
    esp_bridge_wifi_prov_mgr();
#endif /* CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE */
}
```

### 步骤4：实现4G状态检测与Mesh网络切换

#### 4.4.1 创建网络状态监控模块
创建新文件`main/network_monitor.c`：

```c
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mesh_lite.h"

static const char *TAG = "network_monitor";

// 网络状态结构体
typedef struct {
    bool modem_connected;
    bool mesh_connected;
    bool is_mesh_root;
} network_status_t;

static network_status_t g_network_status = {0};

// 4G模块连接状态回调
static void on_modem_ip_event(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_PPP_GOT_IP) {
        ESP_LOGI(TAG, "4G module connected to internet");
        g_network_status.modem_connected = true;
        // 如果是Mesh根节点，保持根节点状态
        if (g_network_status.is_mesh_root) {
            ESP_LOGI(TAG, "Maintaining root node status");
        }
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGI(TAG, "4G module disconnected");
        g_network_status.modem_connected = false;
        // 4G断开时，如果Mesh网络可用，保持连接
        if (g_network_status.mesh_connected) {
            ESP_LOGI(TAG, "Switching to Mesh network for internet access");
        }
    }
}

// Mesh网络状态回调
static void on_mesh_event(void *arg, esp_event_base_t event_base,
                         int32_t event_id, void *event_data)
{
    if (event_id == MESH_LITE_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Connected to Mesh network");
        g_network_status.mesh_connected = true;
        // 检查是否为根节点
        g_network_status.is_mesh_root = esp_mesh_lite_is_root();
        if (g_network_status.is_mesh_root) {
            ESP_LOGI(TAG, "This node is Mesh root");
        } else {
            ESP_LOGI(TAG, "This node is Mesh child");
        }
    } else if (event_id == MESH_LITE_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from Mesh network");
        g_network_status.mesh_connected = false;
        g_network_status.is_mesh_root = false;
    }
}

// 初始化网络监控
void network_monitor_init(void)
{
    // 注册4G模块事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_GOT_IP, &on_modem_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_LOST_IP, &on_modem_ip_event, NULL));
    
    // 注册Mesh事件处理
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_LITE_EVENT, ESP_EVENT_ANY_ID, &on_mesh_event, NULL));
    
    ESP_LOGI(TAG, "Network monitor initialized");
}

// 获取网络状态
network_status_t network_monitor_get_status(void)
{
    return g_network_status;
}
```

#### 4.4.2 创建网络状态监控头文件
创建新文件`main/network_monitor.h`：

```c
#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

// 网络状态结构体
typedef struct {
    bool modem_connected;
    bool mesh_connected;
    bool is_mesh_root;
} network_status_t;

// 初始化网络监控
void network_monitor_init(void);

// 获取网络状态
network_status_t network_monitor_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_MONITOR_H */
```

#### 4.4.3 修改主程序以包含网络监控
修改[main/app_main.c](file:///e:/github/newRouter/main/app_main.c)文件：

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_bridge.h"
#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
#include "web_server.h"
#endif
#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
#include "wifi_prov_mgr.h"
#endif

// 添加Mesh-Lite和网络监控头文件
#include "esp_mesh_lite.h"
#include "network_monitor.h"

static const char *TAG = "main";

static esp_err_t esp_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建所有网络接口
    esp_bridge_create_all_netif();

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = CONFIG_BRIDGE_SOFTAP_SSID,
            .password = CONFIG_BRIDGE_SOFTAP_PASSWORD,
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    esp_wifi_connect();
#endif

    // 初始化并启动Mesh-Lite
    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    esp_mesh_lite_init(&mesh_lite_config);
    esp_mesh_lite_start();

    // 初始化网络监控
    network_monitor_init();

#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
    StartWebServer();
#endif /* CONFIG_APP_BRIDGE_USE_WEB_SERVER */

#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
    esp_bridge_wifi_prov_mgr();
#endif /* CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE */
}
```

### 步骤5：配置路由优先级

#### 4.5.1 修改网络接口优先级
在`menuconfig`中配置网络接口优先级：
- 4G模块（PPP接口）：route_prio = 50
- Mesh网络接口：route_prio = 30
- WiFi Station接口：route_prio = 20
- 以太网接口：route_prio = 10
- WiFi SoftAP接口：route_prio = 10

#### 4.5.2 实现动态路由优先级调整
创建路由管理模块`main/route_manager.c`：

```c
#include "esp_log.h"
#include "esp_netif.h"
#include "network_monitor.h"

static const char *TAG = "route_manager";

// 更新路由优先级
static void update_route_priority(void)
{
    network_status_t status = network_monitor_get_status();
    
    // 获取各网络接口
    esp_netif_t *modem_netif = esp_netif_get_handle_from_ifkey("PPP_DEF");
    esp_netif_t *mesh_netif = esp_netif_get_handle_from_ifkey("MESH_LITE_DEF");
    
    if (status.modem_connected) {
        // 4G连接正常，设置最高优先级
        if (modem_netif) {
            esp_netif_set_route_prio(modem_netif, 50);
            ESP_LOGI(TAG, "Set 4G interface priority to 50");
        }
    } else if (status.mesh_connected) {
        // 4G断开，Mesh网络可用，设置中等优先级
        if (mesh_netif) {
            esp_netif_set_route_prio(mesh_netif, 30);
            ESP_LOGI(TAG, "Set Mesh interface priority to 30");
        }
    }
    
    // 其他接口保持默认优先级
}

// 路由管理任务
void route_manager_task(void *pvParameters)
{
    while (1) {
        // 每5秒检查一次网络状态并更新路由优先级
        vTaskDelay(pdMS_TO_TICKS(5000));
        update_route_priority();
    }
}

// 初始化路由管理器
void route_manager_init(void)
{
    // 创建路由管理任务
    xTaskCreate(route_manager_task, "route_manager", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Route manager initialized");
}
```

#### 4.5.3 创建路由管理头文件
创建`main/route_manager.h`：

```c
#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

// 初始化路由管理器
void route_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ROUTE_MANAGER_H */
```

#### 4.5.4 在主程序中集成路由管理
修改[main/app_main.c](file:///e:/github/newRouter/main/app_main.c)文件：

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_bridge.h"
#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
#include "web_server.h"
#endif
#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
#include "wifi_prov_mgr.h"
#endif

// 添加Mesh-Lite和网络监控头文件
#include "esp_mesh_lite.h"
#include "network_monitor.h"
#include "route_manager.h"

static const char *TAG = "main";

static esp_err_t esp_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_storage_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建所有网络接口
    esp_bridge_create_all_netif();

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid = CONFIG_BRIDGE_SOFTAP_SSID,
            .password = CONFIG_BRIDGE_SOFTAP_PASSWORD,
        }
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_cfg);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    esp_wifi_connect();
#endif

    // 初始化并启动Mesh-Lite
    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    esp_mesh_lite_init(&mesh_lite_config);
    esp_mesh_lite_start();

    // 初始化网络监控
    network_monitor_init();
    
    // 初始化路由管理器
    route_manager_init();

#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
    StartWebServer();
#endif /* CONFIG_APP_BRIDGE_USE_WEB_SERVER */

#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
    esp_bridge_wifi_prov_mgr();
#endif /* CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE */
}
```

## 5. 测试验证方案

### 5.1 单元测试

#### 5.1.1 Mesh网络初始化测试
验证Mesh网络是否能正常初始化：
1. 编译并烧录程序到两个ESP32S3设备
2. 观察串口日志，确认Mesh网络初始化成功
3. 检查是否能自动建立Mesh网络

#### 5.1.2 网络状态监控测试
验证网络状态监控功能：
1. 检查4G模块连接状态是否能正确检测
2. 检查Mesh网络连接状态是否能正确检测
3. 验证节点角色（根节点/子节点）是否能正确识别

#### 5.1.3 路由优先级测试
验证路由优先级设置：
1. 检查4G连接时路由优先级是否为最高
2. 检查4G断开、Mesh连接时路由优先级是否正确调整
3. 验证数据流是否按预期路径转发

### 5.2 集成测试

#### 5.2.1 正常场景测试
两个设备都连接4G网络：
1. 验证两个设备都能正常上网
2. 验证Mesh网络正常建立
3. 验证下位设备能通过WiFi/以太网正常上网

#### 5.2.2 故障切换测试
一个设备4G断开：
1. 断开设备B的4G连接
2. 验证设备B能通过Mesh网络访问互联网
3. 验证设备A仍能正常通过4G上网
4. 验证下位设备网络访问不受影响

#### 5.2.3 网络恢复测试
断开的4G网络恢复：
1. 恢复设备B的4G连接
2. 验证设备B自动切换回4G网络
3. 验证Mesh网络正常维护
4. 验证所有设备网络访问正常

### 5.3 性能测试

#### 5.3.1 网络延迟测试
测量不同场景下的网络延迟：
1. 4G直连延迟
2. Mesh网络转发延迟
3. 切换过程中的延迟变化

#### 5.3.2 吞吐量测试
测量不同网络路径的吞吐量：
1. 4G直连吞吐量
2. Mesh网络转发吞吐量
3. 多设备同时访问时的性能表现

#### 5.3.3 稳定性测试
长时间运行稳定性测试：
1. 24小时连续运行测试
2. 频繁切换测试
3. 多设备连接稳定性测试

## 6. 风险评估与应对措施

### 6.1 技术风险

#### 6.1.1 Mesh网络稳定性风险
**风险描述**：Mesh网络可能因为信号干扰、节点故障等原因不稳定
**应对措施**：
1. 实现Mesh网络自愈功能
2. 添加网络状态监控和自动重连机制
3. 设置合理的超时和重试机制

#### 6.1.2 路由优先级冲突风险
**风险描述**：多个网络接口同时可用时可能出现路由冲突
**应对措施**：
1. 明确设置各接口优先级
2. 实现动态路由优先级调整
3. 添加路由表验证机制

#### 6.1.3 内存资源不足风险
**风险描述**：同时运行多个网络协议可能导致内存不足
**应对措施**：
1. 优化内存使用
2. 合理配置缓冲区大小
3. 必要时增加PSRAM支持

### 6.2 兼容性风险

#### 6.2.1 与现有功能冲突
**风险描述**：Mesh功能可能与现有4G、以太网、WiFi功能冲突
**应对措施**：
1. 充分测试各功能协同工作
2. 实现功能隔离机制
3. 添加配置开关

#### 6.2.2 不同ESP-IDF版本兼容性
**风险描述**：不同版本的ESP-IDF可能对Mesh-Lite支持不同
**应对措施**：
1. 明确支持的ESP-IDF版本范围
2. 进行多版本兼容性测试
3. 提供版本适配指南

## 7. 时间计划

### 7.1 开发阶段
| 阶段 | 任务 | 预估时间 |
|------|------|----------|
| 第1周 | 环境搭建与依赖配置 | 2天 |
| 第2周 | 核心功能开发与集成 | 5天 |
| 第3周 | 网络监控与路由管理实现 | 5天 |
| 第4周 | 测试与调试 | 5天 |
| 第5周 | 优化与文档编写 | 3天 |

### 7.2 测试阶段
| 阶段 | 任务 | 预估时间 |
|------|------|----------|
| 第1周 | 单元测试 | 3天 |
| 第2周 | 集成测试 | 4天 |
| 第3周 | 性能测试 | 3天 |
| 第4周 | 稳定性测试 | 4天 |

## 8. 验收标准

### 8.1 功能验收标准
1. ✅ 两个ESP32S3设备能正常建立Mesh网络
2. ✅ 每个设备都能通过4G网络正常上网
3. ✅ 当一个设备4G断开时，能通过Mesh网络实现上网
4. ✅ 保持现有所有功能正常运行
5. ✅ 网络切换过程平滑，无明显中断

### 8.2 性能验收标准
1. ✅ Mesh网络转发延迟 < 50ms
2. ✅ 网络切换时间 < 5秒
3. ✅ 支持至少10个下位设备同时连接
4. ✅ 24小时连续运行稳定

### 8.3 稳定性验收标准
1. ✅ 无内存泄漏
2. ✅ 无系统崩溃
3. ✅ 自动重连成功率 > 99%
4. ✅ 故障恢复时间 < 10秒

## 9. 后续优化建议

### 9.1 功能优化
1. 实现Mesh网络负载均衡
2. 添加Web配置界面
3. 实现QoS服务质量控制
4. 添加网络流量统计功能

### 9.2 性能优化
1. 优化Mesh网络路由算法
2. 减少网络切换延迟
3. 提高并发处理能力
4. 优化内存使用效率

### 9.3 扩展功能
1. 支持更多Mesh节点类型
2. 实现Mesh网络安全管理
3. 添加OTA升级功能
4. 实现远程监控和管理