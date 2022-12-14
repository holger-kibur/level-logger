#ifndef SETUP_AP_H
#define SETUP_AP_H

#include "esp_http_server.h"
#include "scan.h"

#include <pthread.h>

typedef struct network_info_t {
    // This field STORES the data for the other fields in a contiguous array of
    // c-strings.
    char buffer[256];

    // These fields are ALIASES of the buffer field.
    char *ssid;
    char *password;
    char *target;
    char *devname;
} network_info_t;

typedef enum setup_error_t {
    se_None,
    se_GenNetConnect,
    se_UnmatchedPair,
    se_UnknownField,
    se_SsidTooLong,
    se_SsidMissing,
    se_SsidIncorrect,
    se_PskTooLong,
    se_PskMissing,
    se_PskIncorrect,
    se_TargetMissing,
    se_DevnameMissing,
} setup_error_t;

typedef enum _setup_state_t {
    ss_WaitingForNetInfo,
    ss_WaitingForConnection,
    ss_Failure,
    ss_Success,
} _setup_state_t;

typedef struct setup_ap_server_t {
    pthread_mutex_t _mutex;
    pthread_cond_t _release_to_connect;

    // UNSYNCHRONRIZED FIELDS
    network_info_t info;
    httpd_handle_t _server_handle;
    bg_scan_t *scan;

    // SYNCHRONIZED FIELDS
    setup_error_t _error;
    _setup_state_t _state;
} setup_ap_server_t;

setup_ap_server_t *setup_ap_start_server(bg_scan_t *initial_scan);
void setup_ap_stop_server(setup_ap_server_t *server);

setup_ap_server_t *create_setup_server(bg_scan_t *initial_scan);
void destroy_setup_server(setup_ap_server_t *server);
void wait_for_netinfo_filled(setup_ap_server_t *server);
void tried_connecting(setup_ap_server_t *server, setup_error_t error);
_setup_state_t get_setup_server_state(setup_ap_server_t *server);
void reset_setup_server_state(setup_ap_server_t *server);
void setup_server_error_format(
    setup_ap_server_t *server, int buflen, char *buffer, const char *format);
void fill_netinfo(setup_ap_server_t *server);

#endif // SETUP_AP_H
