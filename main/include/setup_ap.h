#ifndef SETUP_AP_H
#define SETUP_AP_H

#include "esp_http_server.h"

typedef struct _network_info {
    char *ssid;
    char *password;
    char *target;
    char *devname;
} _network_info;

typedef enum _netinfo_error {
    nps_None,
    nps_UnmatchedPair,
    nps_UnknownField,
    nps_SsidTooLong,
    nps_PskTooLong,
} _netinfo_error;
typedef enum setup_ap_error {
    NO_ERROR = 0,
} setup_ap_error;

typedef httpd_handle_t setup_ap_server;

void setup_ap_config_netif();
void setup_ap_init();
void setup_ap_deinit();
setup_ap_server setup_ap_start_server();
setup_ap_server setup_ap_stop_server();

#endif // SETUP_AP_H
