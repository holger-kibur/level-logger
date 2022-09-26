#include "station.h"
#include "const.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "util.h"
#include <string.h>

static const char *TAG = "ll_station";
static wifi_config_t glob_sta_config = {
    .sta = {
        // SSID and password fields are set at runtime
        .scan_method = WIFI_ALL_CHANNEL_SCAN,
        .bssid_set = false,
        .channel = 0,
        .listen_interval = 0,
        .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
        .threshold =
            {
                .rssi = MINIMUM_NETWORK_RSSI,
                .authmode = WIFI_AUTH_OPEN,
            },
        .pmf_cfg =
            {
                .capable = false,
                .required = false,
            },
        .rm_enabled = false,
        .btm_enabled = false,
        .mbo_enabled = false,
        .ft_enabled = false,
        .owe_enabled = false,
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    }};

void ll_station_init() {
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    NPC(sta_netif);
}

void ll_station_set_network_params(char *ssid, char *pass) {
    strncpy((char *)&glob_sta_config.sta.ssid, ssid, MAX_SSID_LEN);
    strncpy((char *)&glob_sta_config.sta.password, pass, MAX_PASSPHRASE_LEN);
    ESP_EC(esp_wifi_set_config(WIFI_IF_STA, &glob_sta_config));
}
