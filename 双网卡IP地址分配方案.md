# 双网卡IP地址分配方案

## 1. 目标
实现商用路由器功能，使WiFi热点、卡2和卡1（作为LAN时）共享同一个IP子网，而卡1（作为WAN时）、WiFi Station和4G模块使用不同的IP子网。

## 2. 当前问题
目前卡1和卡2使用了不同的IP子网（卡1使用192.168.5.x，卡2使用192.168.4.x），导致无法实现共享LAN子网的功能。

## 3. 新的IP地址分配方案

### 3.1 IP子网规划
- **WAN子网**：卡1作为WAN时、WiFi Station和4G模块使用，例如从上级路由器通过DHCP获取的IP（如192.168.1.x）
- **LAN子网**：WiFi热点、卡2和卡1作为LAN时共享，例如192.168.4.x

### 3.2 具体分配
- **卡1作为WAN**：
  - IP地址：从上级路由器通过DHCP获取（如192.168.1.100）
  - 网关：上级路由器地址（如192.168.1.1）
  
- **WiFi Station作为WAN**：
  - IP地址：从上级路由器通过DHCP获取（如192.168.1.101）
  - 网关：上级路由器地址（如192.168.1.1）
  
- **4G模块作为WAN**：
  - IP地址：从运营商通过DHCP获取（如10.44.120.176）
  - 网关：运营商网关地址（如10.64.64.64）
  
- **卡1作为LAN**：
  - IP地址：192.168.4.1（与WiFi热点和卡2共享子网）
  - DHCP服务器：为下位设备分配192.168.4.100-192.168.4.200
  
- **卡2（固定LAN）**：
  - IP地址：192.168.4.1（与WiFi热点和卡1作为LAN时共享子网）
  - DHCP服务器：为下位设备分配192.168.4.100-192.168.4.200
  
- **WiFi热点**：
  - IP地址：192.168.4.1（与卡1作为LAN时和卡2共享子网）
  - DHCP服务器：为下位设备分配192.168.4.100-192.168.4.200

### 3.3 网络角色切换时的IP处理
- 当卡1从WAN切换到LAN时：
  - IP地址从WAN子网（如192.168.1.100）切换到LAN子网（192.168.4.1）
  - 启动DHCP服务器功能
  - 与WiFi热点和卡2共享LAN子网
  
- 当卡1从LAN切换到WAN时：
  - 停止DHCP服务器功能
  - IP地址从LAN子网（192.168.4.1）切换到通过DHCP从上级路由器获取的地址

## 4. 实现要点

### 4.1 IP地址冲突避免
- 确保卡1作为WAN时、WiFi Station和4G模块使用不同的IP子网
- 确保卡2、WiFi热点和卡1作为LAN时使用相同的IP子网（192.168.4.1）

### 4.2 DHCP服务器配置
- 卡1作为LAN时、卡2和WiFi热点共享DHCP地址池（192.168.4.100-192.168.4.200）
- 需要确保同一时间只有一个接口启用DHCP服务器功能，避免地址分配冲突

### 4.3 路由配置
- 当卡1作为WAN时或WiFi Station连接时，路由优先级最高
- 当卡1作为LAN时，4G作为WAN，卡1和卡2作为LAN
- WiFi热点始终作为LAN

## 5. 配置文件修改

### 5.1 bridge_common.c
修改IP地址分配函数，确保不同角色使用不同的IP子网：

```c
// 为双网卡创建专门的IP地址分配函数
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

### 5.2 bridge_eth.c
修改网络接口创建函数，根据角色分配正确的IP地址：

```c
esp_netif_t* esp_bridge_create_eth_netif_dual(esp_netif_ip_info_t* ip_info, 
                                          uint8_t mac[6], 
                                          bool data_forwarding, 
                                          bool enable_dhcps,
                                          int card_index)
{
    // ... 现有代码 ...
    
    if (netif) {
        // 卡1：使用自动WAN/LAN切换
        if (card_index == 0) {
#if defined(CONFIG_BRIDGE_NETIF_ETHERNET_AUTO_WAN_OR_LAN)
            // 卡1启用自动切换，由esp_bridge_set_eth_wan_netif/esp_bridge_set_eth_lan_netif管理
            esp_bridge_set_eth_wan_netif(netif);
#endif
        } else {
            // 卡2：固定LAN
        }
        
        // ... 现有代码 ...
        
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
    }
    
    return netif;
}
```

## 6. 测试验证

### 6.1 IP地址验证
- 卡1作为WAN时获取上级路由器分配的IP（如192.168.1.100）
- WiFi Station作为WAN时获取上级路由器分配的IP（如192.168.1.101）
- 4G模块获取运营商分配的IP（如10.44.120.176）
- 卡1作为LAN时使用192.168.4.1
- 卡2使用192.168.4.1
- WiFi热点使用192.168.4.1

### 6.2 DHCP服务验证
- 下位设备连接到卡1（作为LAN）、卡2或WiFi热点时，都能获取到192.168.4.x范围内的IP地址
- 确保IP地址分配不冲突

### 6.3 网络连通性验证
- 下位设备能够通过卡1（作为WAN）、WiFi Station或4G访问互联网
- 网络角色切换时，下位设备能够保持网络连接或快速重新连接