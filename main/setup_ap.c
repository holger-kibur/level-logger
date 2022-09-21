#include "setup_ap.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <errno.h>
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "const.h"
#include "util.h"
#include "esp_wifi_types.h"
#include <pthread.h>
#include <string.h>
#include <sys/errno.h>

static const char *TAG = "SETUP ACCESS POINT";
static const wifi_config_t SETUP_AP_CONFIG = {
    .ap = {
        .ssid = SETUP_AP_SSID,
        .ssid_len = 0, // Treat SSID as null terminated string.
        .ssid_hidden = 0, // Broadcast the SSID.
        .password = "", // No password because it's open.
        .channel = 1, // First channel, usable in all countries.
        .authmode = WIFI_AUTH_OPEN, // No security.
        .max_connection = 1, // Only one device should be able to connect at a
                             // time.
        .beacon_interval = 100, // Standard 802.11 beacon interval.
        .pairwise_cipher = WIFI_CIPHER_TYPE_NONE, // Open connection, we don't
                                                  // need a cipher.
        .ftm_responder = false, // ??
        .pmf_cfg = { // We don't need Protected Management Frame capability.
            .capable = false,
            .required = false,
        },
    },
};
static const httpd_config_t SETUP_HTTP_CONFIG = HTTPD_DEFAULT_CONFIG();
static const char *SETUP_FORM_HTML = "<!DOCTYPE html><html><form action=/ method=POST>SSID:<br><input name=ssid><br>Password:<br><input name=pass><br>Target:<br><input name=target><br>Device Name:<br><input name=dev_name><br><a href=status><input type=submit value=Connect></a></form></body></html>";
static const char *SETUP_LOADING_HTML = "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"2\"/><style>.loader{display: inline-block; border: 5px solid #f3f3f3; border-radius: 50%; border-top: 5px solid #000000; width: 20px; height: 20px; -webkit-animation: spin 1s linear infinite; /* Safari */ animation: spin 1s linear infinite;}/* Safari */@-webkit-keyframes spin{0%{-webkit-transform: rotate(0deg);}100%{-webkit-transform: rotate(360deg);}}@keyframes spin{0%{transform: rotate(0deg);}100%{transform: rotate(360deg);}}</style></head><body><div class=\"loader\"></div><h1 style=\"display: inline; margin-left: 10px;\">Connecting and verifying</h1></body></html>";
static const char *SETUP_SUCCESS_HTML = "<!DOCTYPE html><html><body><h1 style=\"color: #00cf0e;\">Success!</h1></body></html>";
static const char *SETUP_ERROR_HTML_FORMAT = "<!DOCTYPE html><html><body><h1 style=\"color: red;\">Error!</h1><h2>%s</h2></body></html>";

static setup_ap_server_t *glob_server = NULL;

static setup_error_t parse_netinfo_from_post(network_info_t *netinfo) {
    NPC(netinfo);
    netinfo->ssid = NULL;
    netinfo->password = NULL;
    netinfo->target = NULL;
    netinfo->devname = NULL;
    char *post_cursor = netinfo->buffer;
    while (post_cursor < strchr(netinfo->buffer, '\0')) {
        char *field = strtok_r(post_cursor, "=", &post_cursor);
        char *value = strtok_r(post_cursor, "&", &post_cursor);
        if (strcmp(field, "ssid") == 0) {
            netinfo->ssid = value;
        } else if (strcmp(field, "pass") == 0) {
            netinfo->password = value;
        } else if (strcmp(field, "target") == 0) {
            netinfo->target = value;
        } else if (strcmp(field, "dev_name") == 0) {
            netinfo->devname = value;
        } else {
            return se_UnknownField;
        }
    }
    if (netinfo->ssid == NULL) {
        return se_SsidMissing;
    }
    if (netinfo->password == NULL) {
        return se_PskMissing;
    }
    if (netinfo->target == NULL) {
        return se_TargetMissing;
    }
    if (netinfo->devname == NULL) {
        return se_DevnameMissing;
    }
    return se_None;
}

static setup_error_t netinfo_validate(network_info_t *netinfo) {
    if (strlen(netinfo->ssid) >= MAX_SSID_LEN) {
        return se_SsidTooLong;
    }
    if (strlen(netinfo->password) >= MAX_PSK_LEN) {
        return se_PskTooLong;
    }
    return se_None;
}

static const char *netinfo_error_explain(setup_error_t error) {
    switch (error) {
        case se_None:
            return "No error.";
        case se_UnmatchedPair:
            return "Form POST request content contains an incomplete field=value pair.";
        case se_UnknownField:
            return "Unknown field in form POST request content.";
        case se_SsidTooLong:
            return "Network SSID too long.";
        case se_PskTooLong:
            return "Network password (PSK) too long.";
        default:
            return "Unexplainable error.";
    }
}

static void resp_with_refresh(httpd_req_t *request) {
    ESP_EC(httpd_resp_set_status(request, "302"));
    ESP_EC(httpd_resp_set_hdr(request, "Location", "/"));
    ESP_EC(httpd_resp_send(request, "", 0));
}

static esp_err_t main_get_handler(httpd_req_t *request) {
    ESP_LOGI(TAG, "Received GET request from user!");

    // Decide which page to present to the user
    switch (get_setup_server_state(glob_server)) {
        case ss_WaitingForNetInfo:
            ESP_LOGI(TAG, "Waiting for network information. Responding with network information form page.");
            ESP_EC(httpd_resp_send(request, SETUP_FORM_HTML, HTTPD_RESP_USE_STRLEN));
            break;
        case ss_WaitingForConnection:
            ESP_LOGI(TAG, "Currently trying to connect. Responding with redirect to status.");
            ESP_EC(httpd_resp_send(request, SETUP_LOADING_HTML, HTTPD_RESP_USE_STRLEN));
            break;
        case ss_Failure:;
            char error_reason_buffer[256];

            ESP_LOGI(TAG, "Failed to connect and/or confirm target. Resonding with fail message!");
            setup_server_error_format(glob_server, 256, error_reason_buffer, SETUP_ERROR_HTML_FORMAT);
            ESP_EC(httpd_resp_send(request, error_reason_buffer, HTTPD_RESP_USE_STRLEN));
            reset_setup_server_state(glob_server);
            break;
        case ss_Success:
            ESP_LOGI(TAG, "Succeeded in connecting with network and confirming target. Responding with success mesage!");
            ESP_EC(httpd_resp_send(request, SETUP_SUCCESS_HTML, HTTPD_RESP_USE_STRLEN));
            break;
    }

    return ESP_OK;
}

static esp_err_t main_post_handler(httpd_req_t *request) {
    NPC(glob_server);
    ESP_LOGI(TAG, "Received POST request from form page!");
    
    // Check if the POST content length is more than the buffer size.
    int copy_len = request->content_len;
    int max_len = sizeof(glob_server->info.buffer);
    if (copy_len > max_len) {
        ESP_LOGW(TAG, "POST content is bigger than can fit in the buffer. Check form HTML.");
        // Truncate content to fit in buffer.
        copy_len = max_len;
    }

    int recv_status = httpd_req_recv(request, glob_server->info.buffer, copy_len);
    if (recv_status <= 0) {
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Post Content:\n%s", glob_server->info.buffer);
    fill_netinfo(glob_server);

    resp_with_refresh(request);

    return ESP_OK;
}

void setup_ap_config_netif() {
    ESP_EC(esp_netif_init());
    ESP_EC(esp_event_loop_create_default());

    esp_netif_ip_info_t ap_ip_info;
    esp_netif_set_ip4_addr(&ap_ip_info.ip, 192, 168, 1, 1);
    esp_netif_set_ip4_addr(&ap_ip_info.gw, 0, 0, 0, 0);
    esp_netif_set_ip4_addr(&ap_ip_info.netmask, 255, 255, 255, 0);

    esp_netif_t *setup_ap_netif = esp_netif_create_default_wifi_ap();
    ESP_EC(esp_netif_dhcps_stop(setup_ap_netif));
    ESP_EC(esp_netif_set_ip_info(setup_ap_netif, &ap_ip_info));
    ESP_EC(esp_netif_dhcps_start(setup_ap_netif));
}

void setup_ap_init() {
    ESP_LOGI(TAG, "Starting setup access point initialization!");

    // Set WIFI mode to AP so user can connect to it.
    ESP_EC(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Apply the AP configuration to the AP interface.
    ESP_EC(esp_wifi_set_config(WIFI_IF_AP, (wifi_config_t*)&SETUP_AP_CONFIG));

    // Start wifi driver, load everything into memory.
    ESP_EC(esp_wifi_start());

    ESP_LOGI(TAG, "Finished setup access point initialization!");
}

void setup_ap_deinit() {
    ESP_LOGI(TAG, "Starting setup access point deinitialization!");

    // Stop wifi driver.
    ESP_EC(esp_wifi_stop());

    ESP_LOGI(TAG, "Finished setup access point deinitialization!");
}

setup_ap_server_t *setup_ap_start_server() {
    // Create URI handlers.
    const httpd_uri_t main_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = main_get_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t main_post = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = main_post_handler,
        .user_ctx = NULL,
    };

    // Create the setup server object
    setup_ap_server_t *server = create_setup_server();

    // Globalize the created server
    NOT_NPC(glob_server);
    glob_server = server;

    // Start the setup server
    ESP_EC(httpd_start(&server->_server_handle, &SETUP_HTTP_CONFIG));
    httpd_register_uri_handler(server->_server_handle, &main_get);
    httpd_register_uri_handler(server->_server_handle, &main_post);
    
    return server;
}

void setup_ap_stop_server(setup_ap_server_t *server) {
    if (server != glob_server) {
        ESP_LOGE(TAG, "Called setup_ap_stop_server with unexpected server reference!\nExpected: %p, recieved: %p", glob_server, server);
        abort();
    }
    glob_server = NULL;
    destroy_setup_server(server);
}

setup_ap_server_t *create_setup_server() {
    setup_ap_server_t *ret = malloc(sizeof(setup_ap_server_t));
    memset(&ret->info, 0, sizeof(network_info_t));
    ret->_error = se_None;
    ret->_state = ss_WaitingForNetInfo;
    POSIX_EC(pthread_mutex_init(&ret->_mutex, NULL));
    POSIX_EC(pthread_cond_init(&ret->_release_to_connect, NULL));
    POSIX_EC(pthread_cond_init(&ret->_release_from_connect, NULL));
    memset(&ret->_server_handle, 0, sizeof(httpd_handle_t));
    return ret;
}

void destroy_setup_server(setup_ap_server_t *server) {
    NPC(server);
    POSIX_EC(pthread_mutex_destroy(&server->_mutex));
    POSIX_EC(pthread_cond_destroy(&server->_release_to_connect));
    POSIX_EC(pthread_cond_destroy(&server->_release_from_connect));
    ESP_EC(httpd_stop(server->_server_handle));
}

_setup_state_t get_setup_server_state(setup_ap_server_t *server) {
    NPC(server);
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    _setup_state_t state = server->_state;
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
    return state;
}

void reset_setup_server_state(setup_ap_server_t *server) {
    NPC(server);
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    server->_state = ss_WaitingForNetInfo;
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
}

void setup_server_error_format(setup_ap_server_t *server, int buflen, char *buffer, const char *format) {
    NPC(server);
    NPC(buffer);
    NPC(format);
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    int len_needed = snprintf(buffer, buflen, format, netinfo_error_explain(server->_error));
    if (len_needed >= buflen) {
        ESP_LOGW(TAG, "Setup server error message can't fit in provided buffer!");
    }
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
}

void fill_netinfo(setup_ap_server_t *server) {
    NPC(server);
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    server->_error = parse_netinfo_from_post(&server->info);
    if (server->_error == se_None) {
        server->_error = netinfo_validate(&server->info);
    }
    if (server->_error != se_None) {
        server->_state = ss_Failure;
    } else {
        server->_state = ss_WaitingForConnection;
        // TODO: Signal the condvar bruh
    }
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
}

void wait_for_netinfo_filled(setup_ap_server_t *server) {
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    while (server->_state != ss_WaitingForNetInfo) {
        POSIX_EC(pthread_cond_wait(&server->_release_to_connect, &server->_mutex));
    }
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
}

void tried_connecting(setup_ap_server_t *server, setup_error_t error) {
    NPC(server);
    POSIX_EC(pthread_mutex_lock(&server->_mutex));
    server->_error = error;
    server->_state = (error == se_None ? ss_Success : ss_Failure);
    POSIX_EC(pthread_cond_signal(&server->_release_from_connect))
    POSIX_EC(pthread_mutex_unlock(&server->_mutex));
}


