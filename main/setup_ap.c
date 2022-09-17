#include "setup_ap.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "const.h"
#include "esp_wifi_types.h"
#include <string.h>

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

static _netinfo_error glob_error = nps_None;

static _netinfo_error netinfo_from_post(_network_info *netinfo, char *post_cursor) {
    while (*post_cursor != '\0') {
        char *field = strtok_r(post_cursor, "=", &post_cursor);
        if (*post_cursor == '\0') {
            return nps_UnmatchedPair;
        }
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
            return nps_UnknownField;
        }
    }
    return nps_None;
}

static _netinfo_error netinfo_validate(_network_info *netinfo) {
    if (strlen(netinfo->ssid) >= MAX_SSID_LEN) {
        return nps_SsidTooLong;
    }
    if (strlen(netinfo->password) >= MAX_PSK_LEN) {
        return nps_PskTooLong;
    }
    return nps_None;
}

static const char *netinfo_error_explain(_netinfo_error error) {
    switch (error) {
        case nps_None:
            return "No error.";
        case nps_UnmatchedPair:
            return "Form POST request content contains an incomplete field=value pair.";
        case nps_UnknownField:
            return "Unknown field in form POST request content.";
        case nps_SsidTooLong:
            return "Network SSID too long.";
        case nps_PskTooLong:
            return "Network password (PSK) too long.";
        default:
            return "Unexplainable error.";
    }
}

static esp_err_t form_get_handler(httpd_req_t *request) {
    ESP_LOGI(TAG, "Received GET request for form page!");
    // Send back the SSID + password form.
    esp_err_t send_status = httpd_resp_send(request, SETUP_FORM_HTML, HTTPD_RESP_USE_STRLEN);
    if (send_status) {
        ESP_LOGE(TAG, "Couldn't send response to form GET request!\nError Code: %s", esp_err_to_name(send_status));
    }
    return send_status;
}

static esp_err_t form_post_handler(httpd_req_t *request) {
    ESP_LOGI(TAG, "Received POST request from form page!");
    char post_content[256];
    
    // Check if the POST content length is more than the buffer size.
    int copy_len = request->content_len;
    if (copy_len > sizeof(post_content)) {
        ESP_LOGW(TAG, "POST content is bigger than can fit in the buffer. Check form HTML.");
        // Truncate content to fit in buffer.
        copy_len = sizeof(post_content);
    }

    int recv_status = httpd_req_recv(request, post_content, copy_len);
    if (recv_status <= 0) {
        return ESP_FAIL;
    }
    
    _network_info netinfo;
    glob_error = netinfo_from_post(&netinfo, post_content);
    if (glob_error == nps_None) {
        glob_error = netinfo_validate(&netinfo);
    }

    ESP_LOGV(TAG, "Post Content:\n%s", post_content);

    // Respond with 302 redirect to status page
    httpd_resp_set_status(request, "302");
    httpd_resp_set_hdr(request, "Location", "status");
    httpd_resp_send(request, "", 0);

    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *request) {
    esp_err_t send_status = httpd_resp_send(request, SETUP_LOADING_HTML, HTTPD_RESP_USE_STRLEN);
    if (send_status) {
        ESP_LOGE(TAG, "Couldn't send response to setup GET request!\nError Code: %s", esp_err_to_name(send_status));
    }
    return send_status;
}

void setup_ap_config_netif() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_ip_info_t ap_ip_info;
    esp_netif_set_ip4_addr(&ap_ip_info.ip, 192, 168, 1, 1);
    esp_netif_set_ip4_addr(&ap_ip_info.gw, 0, 0, 0, 0);
    esp_netif_set_ip4_addr(&ap_ip_info.netmask, 255, 255, 255, 0);

    esp_netif_t *setup_ap_netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(setup_ap_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(setup_ap_netif, &ap_ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(setup_ap_netif));
}

void setup_ap_init() {
    ESP_LOGI(TAG, "Starting setup access point initialization!");

    // Set WIFI mode to AP so user can connect to it.
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // Apply the AP configuration to the AP interface.
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, (wifi_config_t*)&SETUP_AP_CONFIG));

    // Start wifi driver, load everything into memory.
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Finished setup access point initialization!");
}

void setup_ap_deinit() {
    ESP_LOGI(TAG, "Starting setup access point deinitialization!");

    // Stop wifi driver.
    ESP_ERROR_CHECK(esp_wifi_stop());

    ESP_LOGI(TAG, "Finished setup access point deinitialization!");
}

setup_ap_server setup_ap_start_server() {
    // Create URI handlers.
    const httpd_uri_t form_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = form_get_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t form_post = {
        .uri = "/",
        .method = HTTP_POST,
        .handler = form_post_handler,
        .user_ctx = NULL,
    };
    const httpd_uri_t status_get = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL,
    };

    // Start the http server.
    setup_ap_server server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &SETUP_HTTP_CONFIG));
    httpd_register_uri_handler(server, &form_get);
    httpd_register_uri_handler(server, &form_post);
    httpd_register_uri_handler(server, &status_get);
    
    return server;
}

