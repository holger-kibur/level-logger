#ifndef SETUP_AP_H
#define SETUP_AP_H

#include <pthread.h>
#include "esp_http_server.h"

typedef struct network_info_t {
    char *ssid;
    char *password;
    char *target;
    char *devname;
} network_info_t;

typedef enum setup_error_t {
    se_None,
    se_UnmatchedPair,
    se_UnknownField,
    se_SsidTooLong,
    se_PskTooLong,
} setup_error_t;

typedef enum _setup_state_t {
    ss_WaitingForNetInfo,
    ss_WaitingForConnection,
    ss_Failure,
    ss_Success,
} _setup_state_t;

typedef struct setup_ap_server_t {
    network_info_t info;
    setup_error_t _error;
    _setup_state_t _state;
    pthread_mutex_t _mutex;
    pthread_cond_t _release_to_connect;
    pthread_cond_t _release_from_connect;
    httpd_handle_t _server_handle;
} setup_ap_server_t;

void setup_ap_config_netif();
void setup_ap_init();
void setup_ap_deinit();
setup_ap_server_t *setup_ap_start_server();
void setup_ap_stop_server(setup_ap_server_t *server);

setup_ap_server_t *create_setup_server();
void destroy_setup_server(setup_ap_server_t *server);
void wait_for_netinfo_filled(setup_ap_server_t *server);
void tried_connecting(setup_ap_server_t *server, setup_error_t error);
_setup_state_t get_setup_server_state(setup_ap_server_t *server);
_setup_state_t get_setup_server_state(setup_ap_server_t *server);
void reset_setup_server_state(setup_ap_server_t *server);
void setup_server_error_format(setup_ap_server_t *server, int buflen, char *buffer, const char *format);

#endif // SETUP_AP_H
