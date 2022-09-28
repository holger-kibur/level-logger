#include "client.h"
#include "esp_flash.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types.h"
#include "setup.h"
#include "station.h"
#include "util.h"

static const char *TAG = "client";

#define REFLECT_REASON(X)                                                      \
    {                                                                          \
    case WIFI_REASON_##X:                                                      \
        return "WIFI_REASON_" #X;                                              \
    }

const char *esp_wifi_reflect_reason(uint8_t reason) {
    switch (reason) {
        REFLECT_REASON(UNSPECIFIED);
        REFLECT_REASON(AUTH_EXPIRE);
        REFLECT_REASON(AUTH_LEAVE);
        REFLECT_REASON(ASSOC_EXPIRE);
        REFLECT_REASON(ASSOC_TOOMANY);
        REFLECT_REASON(NOT_AUTHED);
        REFLECT_REASON(NOT_ASSOCED);
        REFLECT_REASON(ASSOC_LEAVE);
        REFLECT_REASON(ASSOC_NOT_AUTHED);
        REFLECT_REASON(DISASSOC_PWRCAP_BAD);
        REFLECT_REASON(DISASSOC_SUPCHAN_BAD);
        REFLECT_REASON(IE_INVALID);
        REFLECT_REASON(MIC_FAILURE);
        REFLECT_REASON(4WAY_HANDSHAKE_TIMEOUT);
        REFLECT_REASON(GROUP_KEY_UPDATE_TIMEOUT);
        REFLECT_REASON(IE_IN_4WAY_DIFFERS);
        REFLECT_REASON(GROUP_CIPHER_INVALID);
        REFLECT_REASON(PAIRWISE_CIPHER_INVALID);
        REFLECT_REASON(AKMP_INVALID);
        REFLECT_REASON(UNSUPP_RSN_IE_VERSION);
        REFLECT_REASON(INVALID_RSN_IE_CAP);
        REFLECT_REASON(802_1X_AUTH_FAILED);
        REFLECT_REASON(CIPHER_SUITE_REJECTED);
        REFLECT_REASON(BEACON_TIMEOUT);
        REFLECT_REASON(NO_AP_FOUND);
        REFLECT_REASON(AUTH_FAIL);
        REFLECT_REASON(ASSOC_FAIL);
        REFLECT_REASON(HANDSHAKE_TIMEOUT);
    default:
        return "Unknown reason code!";
    }
}

connect_result_t try_connect_to_network(char *ssid, char *pass) {
    NPC(ssid);
    NPC(pass);
    conn_attempt_t *conn_attempt = ll_station_create_conn_attempt();
    ll_station_set_network_params(ssid, pass);
    ll_station_start_conn_fsm(conn_attempt);
    while (true) {
        ll_station_wait_for_change(conn_attempt);
        switch (ll_station_get_state(conn_attempt)) {
        case cas_Initial:
            ESP_LOGE(TAG,
                     "Connection attempt FSM transitioned into initial state!");
            abort();
        case cas_StartedConnection:
            ESP_LOGI(TAG, "Started connecting to network...");
            break;
        case cas_Failed:;
            uint8_t reason = ll_station_get_fail_reason(conn_attempt);
            ESP_LOGW(TAG, "Connection to access point failed!\nCode: %s",
                     esp_wifi_reflect_reason(reason));
            goto stop_fsm;
        case cas_ConnectSuccess:
            ESP_LOGI(TAG, "Connected to the access point!");
            break;
        case cas_DhcpSuccess:
            ESP_LOGI(TAG, "Got IP from network!");
            goto stop_fsm;
        }
    }

stop_fsm:
    ll_station_stop_conn_fsm(conn_attempt);

    if (ll_station_get_state(conn_attempt) == cas_DhcpSuccess) {
        ll_station_destroy_conn_attempt(conn_attempt);
        return cr_None;
    } else {
        uint8_t reason = ll_station_get_fail_reason(conn_attempt);
        ll_station_destroy_conn_attempt(conn_attempt);
        switch (reason) {
        case WIFI_REASON_NO_AP_FOUND:
            return cr_InvalidSsid;
        case WIFI_REASON_AUTH_FAIL:
            return cr_InvalidPass;
        default:
            return cr_TechnicalError;
        }
    }
}
