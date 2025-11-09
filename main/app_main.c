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

#if defined(CONFIG_APP_BRIDGE_USE_WEB_SERVER)
    StartWebServer();
#endif /* CONFIG_APP_BRIDGE_USE_WEB_SERVER */
#if defined(CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE)
    esp_bridge_wifi_prov_mgr();
#endif /* CONFIG_APP_BRIDGE_USE_WIFI_PROVISIONING_OVER_BLE */
}