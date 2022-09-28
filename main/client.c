#include "esp_flash.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "util.h"
#include "station.h"
#include "client.h"

connect_result_t try_connect_to_network(char *ssid, char *pass) {
    ll_station_set_network_params(ssid, pass);
    conn_attemp
}

