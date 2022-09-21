#include <stdio.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/portmacro.h"
#include "nvs_flash.h"
#include "setup_ap.h"

static const char* TAG = "MAIN";

void do_setup(void) {
    // Initialize ESP stuff for the access point
    setup_ap_init();

    // Start the server
    setup_ap_server_t *setup_server = setup_ap_start_server();
    
    // Loop until setup succeeds
    while (true) {
        // Block until the user has submitted network and target information
        // through the setup website.
        wait_for_netinfo_filled(setup_server);
        
        // Attempt to connect to the network with given ssid and password
        ESP_LOGI(TAG, "Connecting to network %s...", setup_server->info.ssid);

        // We succeeded in connecting, break out of the loop.
        tried_connecting(setup_server, se_None);
        break;
    }
    
    // Give the user 5s to see the successful connection
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Stop the setup access point and server
    setup_ap_stop_server(setup_server);
    setup_server = NULL;

    // Deinitialize the ESP stuff involved with setup
    setup_ap_deinit();
}

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
