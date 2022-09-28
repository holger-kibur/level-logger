#include "station.h"
#include "const.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "util.h"
#include <pthread.h>
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

static void handle_sta_connect(void *conn_attempt, esp_event_base_t base,
                               int32_t id, void *event_data) {}

static void handle_sta_disconnect(void *conn_attempt, esp_event_base_t base,
                                  int32_t id, void *event_data) {}

static void handle_sta_got_ip(void *conn_attempt, esp_event_base_t base,
                              int32_t id, void *event_data) {}

void ll_station_init() {
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    NPC(sta_netif);
}

void ll_station_set_network_params(char *ssid, char *pass) {
    strncpy((char *)&glob_sta_config.sta.ssid, ssid, MAX_SSID_LEN);
    strncpy((char *)&glob_sta_config.sta.password, pass, MAX_PASSPHRASE_LEN);
    ESP_EC(esp_wifi_set_config(WIFI_IF_STA, &glob_sta_config));
}

conn_attempt_t *ll_station_create_conn_attempt() {
    conn_attempt_t *ret = malloc(sizeof(conn_attempt_t));
    NPC(ret);
    ret->state = cas_Initial;
    POSIX_EC(pthread_mutex_init(&ret->mutex, NULL));
    POSIX_EC(pthread_cond_init(&ret->state_changed, NULL));
    return ret;
}

void ll_station_destroy_conn_attempt(conn_attempt_t *conn_attempt) {
    NPC(conn_attempt);
    POSIX_EC(pthread_mutex_destroy(&conn_attempt->mutex));
    POSIX_EC(pthread_cond_destroy(&conn_attempt->state_changed));
    free(conn_attempt);
}

void ll_station_start_conn_fsm(conn_attempt_t *conn_attempt) {
    NPC(conn_attempt);

    // Register WIFI event handlers
    ESP_EC(esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, handle_sta_connect, conn_attempt,
        NULL));
    ESP_EC(esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, handle_sta_disconnect,
        conn_attempt, NULL));
    ESP_EC(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, handle_sta_got_ip, conn_attempt, NULL));

    // Start the connection process
    ESP_EC(esp_wifi_connect());
}

void ll_station_wait_for_change(conn_attempt_t *conn_attempt) {
    NPC(conn_attempt);
    POSIX_EC(pthread_mutex_lock(&conn_attempt->mutex));
    conn_attempt_state_t prev_state = conn_attempt->state;
    while (conn_attempt->state == prev_state) {
        POSIX_EC(pthread_cond_wait(&conn_attempt->state_changed,
                                   &conn_attempt->mutex));
    }
    POSIX_EC(pthread_mutex_unlock(&conn_attempt->mutex));
}

void ll_station_stop_conn_fsm(conn_attempt_t *conn_attempt) {
    NPC(conn_attempt);

    // Unregister WIFI event handlers
    ESP_EC(esp_event_handler_instance_unregister(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, NULL));
    ESP_EC(esp_event_handler_instance_unregister(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL));
    ESP_EC(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                 NULL));
}
