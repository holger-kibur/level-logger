#include "access_point.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/portmacro.h"
#include "nvs_flash.h"
#include "setup.h"
#include "station.h"
#include "util.h"
#include <stdio.h>

static const char *TAG = "level_logger_main";

void do_setup(void) {
    // Start the server
    setup_ap_server_t *setup_server = setup_ap_start_server();

    // Loop until setup succeeds
    while (true) {
        // Block until the user has submitted network and target information
        // through the setup website.
        wait_for_netinfo_filled(setup_server);
        ESP_LOGD(TAG, "netinfo filling unblocked, continuing on main thread");

        // Attempt to connect to the network with given ssid and password
        ESP_LOGI(TAG, "Connecting to network %s...", setup_server->info.ssid);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        // We succeeded in connecting, break out of the loop.
        tried_connecting(setup_server, se_None);
        break;
    }

    // Give the user 5s to see the successful connection
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Stop the setup access point and server
    setup_ap_stop_server(setup_server);
    setup_server = NULL;


}

void app_main(void) {
    // Init logging
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    // Init NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Init network interface and event loop
    ESP_EC(esp_netif_init());
    ESP_EC(esp_event_loop_create_default());

    // Configure network interface for the access point
    ll_access_point_init();

    // Configure network interface for the station
    ll_station_init();

    // Set WIFI mode to APSTA for both setup and client modes
    ESP_EC(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Initialize WIFI driver.
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    // Do main thread setup logic
    do_setup();
}
