#include <stdio.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "setup_ap.h"

static const char* TAG = "MAIN";

void app_main(void)
{
    // Init NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize network interface for the setup AP.
    setup_ap_config_netif();

    // Initialize WIFI driver.
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    // Initialize AP driver
    setup_ap_init();

    // Start the HTTP server
    setup_ap_start_server();
}
