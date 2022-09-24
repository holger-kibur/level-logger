#include "try_connect.h"
#include "esp_netif_types.h"
#include "esp_wifi_default.h"
#include "util.h"

void try_connect_config_netif() {
    esp_netif_t *try_connect_netif = esp_netif_create_default_wifi_sta();
    NPC(try_connct_netif);
}

void try_connect_init() {}

void try_connect_deinit() {}
