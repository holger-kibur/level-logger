#include "access_point.h"
#include "const.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "setup_ap.h"
#include "util.h"

static const char *TAG = "ll_access_point";
static const wifi_config_t AP_CONFIG = {
    .ap =
        {
            .ssid = AP_SSID,
            .ssid_len = 0,    // Treat SSID as null terminated string.
            .ssid_hidden = 0, // Broadcast the SSID.
            .password = "",   // No password because it's open.
            .channel = 1,     // First channel, usable in all countries.
            .authmode = WIFI_AUTH_OPEN, // No security.
            .max_connection = 1, // Only one device should be able to connect at
                                 // a time.
            .beacon_interval = 100, // Standard 802.11 beacon interval.
            .pairwise_cipher = WIFI_CIPHER_TYPE_NONE, // Open connection, we
                                                      // don't need a cipher.
            .ftm_responder = false,                   // ??
            .pmf_cfg =
                {
                    // We don't need Protected Management Frame capability.
                    .capable = false,
                    .required = false,
                },
        },
};

void ll_access_point_init() {
    esp_netif_ip_info_t ap_ip_info;
    esp_netif_set_ip4_addr(&ap_ip_info.ip, 192, 168, 1, 1);
    esp_netif_set_ip4_addr(&ap_ip_info.gw, 0, 0, 0, 0);
    esp_netif_set_ip4_addr(&ap_ip_info.netmask, 255, 255, 255, 0);

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    NPC(ap_netif);
    ESP_EC(esp_netif_dhcps_stop(ap_netif));
    ESP_EC(esp_netif_set_ip_info(ap_netif, &ap_ip_info));
    ESP_EC(esp_netif_dhcps_start(ap_netif));

    ESP_EC(esp_wifi_set_config(WIFI_IF_AP, (wifi_config_t *)&AP_CONFIG));
}
