# 双网卡IP地址分配实现方案

## 1. 目标
实现商用路由器功能，使WiFi热点、卡2和卡1（作为LAN时）共享同一个IP子网（192.168.4.x），而卡1（作为WAN时）、WiFi Station和4G模块使用不同的IP子网。

## 2. 当前问题分析
目前系统中存在多个网络接口，但IP地址分配策略不统一：
1. 卡1和卡2使用不同的子网（卡1使用192.168.5.x，卡2使用192.168.4.x）
2. WiFi热点使用192.168.5.x子网
3. 卡1作为WAN时从上级路由器获取IP
4. WiFi Station作为WAN时从上级路由器获取IP
5. 4G模块作为WAN时从运营商获取IP

这种分配方式导致：
- 下位设备在不同接口间切换时需要重新获取IP
- 网络管理复杂
- 不符合商用路由器的典型配置

## 3. 新的IP地址分配方案

### 3.1 IP子网规划
- **WAN子网**：卡1作为WAN时、WiFi Station和4G模块使用各自获取的IP地址
- **LAN子网**：WiFi热点、卡2和卡1作为LAN时共享192.168.4.x子网

### 3.2 具体分配策略

#### WAN接口（动态IP）
- **卡1作为WAN**：从上级路由器通过DHCP获取IP（如192.168.1.x网段）
- **WiFi Station作为WAN**：从上级路由器通过DHCP获取IP（如192.168.1.x网段）
- **4G模块作为WAN**：从运营商通过DHCP获取IP（如10.x.x.x网段）

#### LAN接口（固定IP）
- **卡1作为LAN**：192.168.4.1
- **卡2**：192.168.4.1
- **WiFi热点**：192.168.4.1

### 3.3 DHCP地址池
- **共享地址池**：192.168.4.100-192.168.4.200
- **分配策略**：
  - 同一时间只有一个接口启用DHCP服务器功能
  - 卡1作为LAN时启用DHCP服务器
  - 卡2始终启用DHCP服务器
  - WiFi热点始终启用DHCP服务器
  - 通过协调机制避免地址冲突

## 4. 实现方案

### 4.1 修改IP地址分配函数

#### 4.1.1 修改bridge_common.c中的IP地址分配函数

**原函数**：
```c
esp_err_t esp_bridge_netif_request_ip(esp_netif_ip_info_t *ip_info)
{
    // 原有实现
}
```

**新增双网卡专用函数**：
```c
esp_err_t esp_bridge_netif_request_ip_dual(esp_netif_ip_info_t *ip_info, int card_index, bool is_lan_mode)
{
    if (is_lan_mode) {
        // LAN模式，使用192.168.4.x子网
        if (card_index == 0 || card_index == 1) {
            // 卡1和卡2作为LAN时都使用192.168.4.1
            ip_info->ip.addr = ESP_IP4TOADDR(192, 168, 4, 1);
            ip_info->gw.addr = ESP_IP4TOADDR(192, 168, 4, 1);
            ip_info->netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
            ESP_LOGI(TAG, "Card%d LAN IP Address:" IPSTR, card_index, IP2STR(&ip_info->ip));
            return ESP_OK;
        }
    }
    
    // WAN模式或默认情况，使用原来的逻辑
    return esp_bridge_netif_request_ip(ip_info);
}
```

### 4.2 修改网络接口创建函数

#### 4.2.1 修改bridge_eth.c中的网络接口创建函数

**原函数**：
```c
esp_netif_t* esp_bridge_create_eth_netif(esp_netif_ip_info_t* ip_info, 
                                          uint8_t mac[6], 
                                          bool data_forwarding, 
                                          bool enable_dhcps)
```

**新增双网卡专用函数**：
```c
esp_netif_t* esp_bridge_create_eth_netif_dual(esp_netif_ip_info_t* ip_info, 
                                          uint8_t mac[6], 
                                          bool data_forwarding, 
                                          bool enable_dhcps,
                                          int card_index)
{
    esp_netif_t* netif = NULL;
    
    // 根据card_index创建不同的网络接口
    if (card_index == 0) {
        // 卡1：可WAN可LAN
        if (data_forwarding) {
            // LAN模式
            netif = esp_netif_new(&esp_netif_config_lan);
        } else {
            // WAN模式
            netif = esp_netif_new(&esp_netif_config_wan);
        }
    } else if (card_index == 1) {
        // 卡2：固定LAN
        netif = esp_netif_new(&esp_netif_config_lan);
    }
    
    if (netif) {
        // 卡1：使用自动WAN/LAN切换
        if (card_index == 0) {
#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
            // 卡1启用自动切换，由esp_bridge_set_eth_wan_netif/esp_bridge_set_eth_lan_netif管理
            // 这里不设置固定角色，让自动切换逻辑处理
            // 但我们需要确保网卡能够启动DHCP客户端
            esp_bridge_set_eth_wan_netif(netif);
#endif
        } else {
            // 卡2：固定LAN，不使用自动切换
            // 直接配置为LAN模式
        }
        
        // 设置MAC地址
        if (mac) {
            esp_netif_set_mac(netif, mac);
        }
        
        // 添加到网络接口列表
        esp_bridge_netif_list_add(netif, NULL);
        
        // 配置IP地址
        esp_netif_ip_info_t allocate_ip_info = {0};
        if (ip_info) {
            esp_bridge_netif_set_ip_info(netif, ip_info, true, true);
        } else {
            bool conflict_check = true;
            if (esp_bridge_load_ip_info_from_nvs(esp_netif_get_ifkey(netif), &allocate_ip_info, &conflict_check) != ESP_OK) {
                // 根据card_index和模式分配IP
                if (card_index == 0) {
                    // 卡1：根据配置决定是WAN还是LAN
#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
                    // 自动切换模式，初始不分配固定IP，等待自动切换逻辑处理
                    // 但在LAN模式下需要分配IP
                    if (data_forwarding) {
                        // LAN模式
                        esp_bridge_netif_request_ip_dual(&allocate_ip_info, card_index, true);
                    }
#else
                    // 固定模式
                    if (data_forwarding) {
                        // LAN模式
                        esp_bridge_netif_request_ip_dual(&allocate_ip_info, card_index, true);
                    } else {
                        // WAN模式，使用DHCP获取IP
                        esp_bridge_netif_request_ip(&allocate_ip_info);
                    }
#endif
                } else {
                    // 卡2：固定LAN模式
                    esp_bridge_netif_request_ip_dual(&allocate_ip_info, card_index, true);
                }
            }
            esp_bridge_netif_set_ip_info(netif, &allocate_ip_info, true, conflict_check);
        }
        
        // 配置DHCP服务器
        if (enable_dhcps) {
            // 为LAN模式配置IP地址
            esp_netif_ip_info_t lan_ip_info = {0};
            lan_ip_info.ip.addr = ESP_IP4TOADDR(192, 168, 4, 1);
            lan_ip_info.gw.addr = ESP_IP4TOADDR(192, 168, 4, 1);
            lan_ip_info.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
            esp_netif_dhcps_stop(netif);
            esp_netif_set_ip_info(netif, &lan_ip_info);
            esp_netif_dhcps_start(netif);
        }
        
        // 确保网卡1启动DHCP客户端
        if (card_index == 0) {
            esp_netif_dhcpc_start(netif);
        }
    }
    
    return netif;
}
```

#### 4.2.2 修改bridge_wifi.c中的WiFi接口创建函数

**确保WiFi Station接口正确创建**：
```c
#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
esp_netif_t* esp_bridge_create_station_netif(esp_netif_ip_info_t* ip_info, uint8_t mac[6], bool data_forwarding, bool enable_dhcps)
{
    esp_netif_t* wifi_netif = NULL;
    wifi_mode_t mode = WIFI_MODE_NULL;

    if (data_forwarding || enable_dhcps) {
        return wifi_netif;
    }

    esp_bridge_wifi_init();
    wifi_netif = esp_netif_create_default_wifi_sta();
    esp_bridge_netif_list_add(wifi_netif, NULL);

    esp_wifi_get_mode(&mode);
    if (mode != WIFI_MODE_STA && mode != WIFI_MODE_APSTA) {
        mode |= WIFI_MODE_STA;
        ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    }

    wifi_sta_config_t router_config;
    /* Get WiFi Station configuration */
    esp_wifi_get_config(WIFI_IF_STA, (wifi_config_t*)&router_config);

    /* Get Wi-Fi Station ssid success */
    if (strlen((const char*)router_config.ssid)) {
        ESP_LOGI(TAG, "Found ssid %s", (const char*)router_config.ssid);
        ESP_LOGI(TAG, "Found password %s", (const char*)router_config.password);
    }

    if (ip_info) {
        esp_bridge_netif_set_ip_info(wifi_netif, ip_info, true, true);
    } else {
        esp_netif_ip_info_t ip_info_from_nvs;
        bool conflict_check = true;
        if (esp_bridge_load_ip_info_from_nvs(esp_netif_get_ifkey(wifi_netif), &ip_info_from_nvs, &conflict_check) == ESP_OK) {
            esp_bridge_netif_set_ip_info(wifi_netif, &ip_info_from_nvs, true, conflict_check);
        }
        // WiFi Station作为WAN，使用DHCP获取IP
        // 不需要手动设置IP，由DHCP自动获取
    }

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_sta_disconnected_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));

    return wifi_netif;
}
#endif /* CONFIG_BRIDGE_EXTERNAL_NETIF_STATION */
```

### 4.3 修改创建所有网络接口的函数

#### 4.3.1 修改bridge_common.c中的esp_bridge_create_all_netif函数

```c
void esp_bridge_create_all_netif(void)
{
    ESP_LOGI(TAG, "esp-iot-bridge version: %d.%d.%d", IOT_BRIDGE_VER_MAJOR, IOT_BRIDGE_VER_MINOR, IOT_BRIDGE_VER_PATCH);
    
    // 验证配置
    esp_bridge_validate_config();

#if CONFIG_BRIDGE_ETHERNET_NETIF_ENABLE
    // 创建卡1网络接口（可WAN可LAN）
    {
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x56};
#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
        // 启用自动切换，创建为WAN接口
        esp_bridge_create_eth_netif_dual(NULL, mac, false, false, 0);
#else
        // 固定模式，根据配置创建
#if CONFIG_BRIDGE_EXTERNAL_NETIF_ETHERNET
        // 作为WAN
        esp_bridge_create_eth_netif_dual(NULL, mac, false, false, 0);
#else
        // 作为LAN
        esp_bridge_create_eth_netif_dual(NULL, mac, true, true, 0);
#endif
#endif
    }
    
    // 创建卡2网络接口（固定LAN）
    {
        uint8_t mac[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x57};
        esp_bridge_create_eth_netif_dual(NULL, mac, true, true, 1);
    }
    
    // 初始化SPI以太网（共享SPI总线）
    esp_bridge_eth_spi_init_dual(0);  // 初始化卡1
    esp_bridge_eth_spi_init_dual(1);  // 初始化卡2
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_STATION)
    // 创建WiFi Station网络接口（作为WAN）
    esp_bridge_create_station_netif(NULL, NULL, false, false);
#if defined(CONFIG_BRIDGE_WIFI_PMF_DISABLE)
    esp_wifi_disable_pmf_config(WIFI_IF_STA);
#endif
#endif

#if defined(CONFIG_BRIDGE_DATA_FORWARDING_NETIF_SOFTAP)
    // 创建WiFi SoftAP网络接口（作为LAN）
    uint8_t softap_mac[6] = {0x02, 0x00, 0x00, 0x12, 0x34, 0x58};
    esp_bridge_create_softap_netif(NULL, softap_mac, true, true);
#endif

#if defined(CONFIG_BRIDGE_EXTERNAL_NETIF_MODEM)
    // 创建4G模块网络接口（作为WAN）
    esp_bridge_create_modem_netif();
#endif
}
```

### 4.4 修改DNS更新逻辑

#### 4.4.1 修改bridge_common.c中的DNS更新函数

```c
static void esp_bridge_update_dns_info(esp_netif_t *external_netif, esp_netif_t *softap_netif)
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(external_netif, ESP_NETIF_DNS_MAIN, &dns);
    
    // 更新SoftAP的DNS信息
    if (softap_netif) {
        esp_netif_set_dns_info(softap_netif, ESP_NETIF_DNS_MAIN, &dns);
    }
    
    // 更新卡1作为LAN时的DNS信息
    esp_netif_t *eth_lan_netif = esp_netif_get_handle_from_ifkey("ETH_LAN");
    if (eth_lan_netif) {
        esp_netif_set_dns_info(eth_lan_netif, ESP_NETIF_DNS_MAIN, &dns);
    }
    
    // 更新卡2的DNS信息
    esp_netif_t *eth_lan2_netif = esp_netif_get_handle_from_ifkey("ETH_LAN2");
    if (eth_lan2_netif) {
        esp_netif_set_dns_info(eth_lan2_netif, ESP_NETIF_DNS_MAIN, &dns);
    }
}
```

### 4.5 实现DHCP服务器协调机制

#### 4.5.1 添加DHCP服务器管理函数

```c
static bool dhcp_server_running_eth_lan = false;
static bool dhcp_server_running_eth_lan2 = false;
static bool dhcp_server_running_softap = false;

static void esp_bridge_manage_dhcp_servers(void)
{
    // 获取各网络接口状态
    esp_netif_t *eth_lan_netif = esp_netif_get_handle_from_ifkey("ETH_LAN");
    esp_netif_t *eth_lan2_netif = esp_netif_get_handle_from_ifkey("ETH_LAN2");
    esp_netif_t *softap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    
    // 确保同一时间只有一个DHCP服务器运行
    // 优先级：卡1作为LAN > 卡2 > WiFi热点
    
    if (eth_lan_netif && dhcp_server_running_eth_lan) {
        // 卡1作为LAN时，停止其他DHCP服务器
        if (eth_lan2_netif && dhcp_server_running_eth_lan2) {
            esp_netif_dhcps_stop(eth_lan2_netif);
            dhcp_server_running_eth_lan2 = false;
        }
        if (softap_netif && dhcp_server_running_softap) {
            esp_netif_dhcps_stop(softap_netif);
            dhcp_server_running_softap = false;
        }
    } else if (eth_lan2_netif && dhcp_server_running_eth_lan2) {
        // 卡2运行DHCP服务器时，停止WiFi热点的DHCP服务器
        if (softap_netif && dhcp_server_running_softap) {
            esp_netif_dhcps_stop(softap_netif);
            dhcp_server_running_softap = false;
        }
    }
    // 如果都没有运行，则启动优先级最高的
    else {
        if (eth_lan_netif && !dhcp_server_running_eth_lan) {
            esp_netif_dhcps_start(eth_lan_netif);
            dhcp_server_running_eth_lan = true;
        } else if (eth_lan2_netif && !dhcp_server_running_eth_lan2) {
            esp_netif_dhcps_start(eth_lan2_netif);
            dhcp_server_running_eth_lan2 = true;
        } else if (softap_netif && !dhcp_server_running_softap) {
            esp_netif_dhcps_start(softap_netif);
            dhcp_server_running_softap = true;
        }
    }
}
```

## 5. 测试验证方案

### 5.1 IP地址验证
- 验证卡1作为WAN时获取上级路由器分配的IP（如192.168.1.100）
- 验证WiFi Station作为WAN时获取上级路由器分配的IP（如192.168.1.101）
- 验证4G模块获取运营商分配的IP（如10.44.120.176）
- 验证卡1作为LAN时使用192.168.4.1
- 验证卡2使用192.168.4.1
- 验证WiFi热点使用192.168.4.1

### 5.2 DHCP服务验证
- 下位设备连接到卡1（作为LAN）、卡2或WiFi热点时，都能获取到192.168.4.x范围内的IP地址
- 确保IP地址分配不冲突
- 验证DHCP服务器协调机制正常工作

### 5.3 网络连通性验证
- 下位设备能够通过卡1（作为WAN）、WiFi Station或4G访问互联网
- 网络角色切换时，下位设备能够保持网络连接或快速重新连接
- 验证路由优先级根据网络状态正确更新

## 6. 风险评估与缓解措施

### 6.1 技术风险
1. **IP地址冲突**：
   - 风险：多个接口同时启用DHCP服务器可能导致地址分配冲突
   - 缓解措施：实现DHCP服务器协调机制，确保同一时间只有一个接口启用DHCP服务器

2. **路由优先级管理**：
   - 风险：网络状态变化时路由优先级更新不及时
   - 缓解措施：实现事件驱动的路由优先级更新机制

3. **内存占用增加**：
   - 风险：双网卡功能可能增加内存占用
   - 缓解措施：优化内存使用，必要时调整系统配置

### 6.2 兼容性风险
1. **原有功能影响**：
   - 风险：修改可能影响4G转WiFi等原有功能
   - 缓解措施：充分测试原有功能，确保兼容性

2. **配置复杂性增加**：
   - 风险：新增配置项可能增加用户配置复杂性
   - 缓解措施：提供详细的配置说明和默认配置

## 7. 实施计划

### 7.1 第一阶段：基础功能实现
1. 修改IP地址分配函数
2. 修改网络接口创建函数
3. 实现双网卡支持
4. 验证基础功能

### 7.2 第二阶段：高级功能实现
1. 实现DHCP服务器协调机制
2. 实现路由优先级动态调整
3. 实现网络状态监控
4. 验证高级功能

### 7.3 第三阶段：测试与优化
1. 功能测试
2. 性能测试
3. 稳定性测试
4. 优化调整