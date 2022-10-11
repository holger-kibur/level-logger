#include "scan.h"
#include "const.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "util.h"
#include <string.h>

static const wifi_scan_config_t SCAN_CONFIG = {
    .ssid = NULL,
    .bssid = NULL,
    .channel = 0,
    .show_hidden = true,
    .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    .scan_time =
        {
            .passive = AP_SCAN_TIME_MS,
        },
};

static const wifi_country_t SCAN_COUNTRY = {
    .cc = COUNTRY_CODE,
    .schan = LOWEST_CHAN,
    .nchan = HIGHEST_CHAN,
    .policy = WIFI_COUNTRY_POLICY_AUTO,
};

static const char *TAG = "ll_scan";

static void handle_scan_complete(void *handler_data, esp_event_base_t base,
                                 int32_t id, void *event_data) {
    NPC(handler_data);

    bg_scan_t *bg_scan = (bg_scan_t *)handler_data;
    bg_scan->scanned_ap_count = AP_SCAN_MAX_APS;
    ESP_EC(esp_wifi_scan_get_ap_records(&bg_scan->scanned_ap_count,
                                        bg_scan->scanned_aps));
}

static void filter_scan_results(bg_scan_t *bg_scan) {
    NPC(bg_scan);

    int num_removed = 0;
    for (int i = 0; i < bg_scan->scanned_ap_count; i++) {
        wifi_ap_record_t *netrec = &bg_scan->scanned_aps[i];

        // If there is no SSID, remove it
        if (strlen((char *)netrec->ssid) == 0) {
            num_removed++;
            goto removed;
        }

        // Check if its a duplicate
        for (int j = i - 1 - num_removed; j >= 0; j--) {
            wifi_ap_record_t *back_netrec = &bg_scan->scanned_aps[j];
            if (strcmp((char *)netrec->ssid, (char *)back_netrec->ssid) == 0) {
                // Duplicate SSID with weaker signal, remove it
                num_removed++;
                goto removed;
            }
        }

        // Hoist the scan result entry up the list
        if (num_removed > 0) {
            bg_scan->scanned_aps[i - num_removed] = bg_scan->scanned_aps[i];
        }

    removed:;
    }

    // Trim the list
    bg_scan->scanned_ap_count -= num_removed;
}

bg_scan_t *ll_do_scan() {
    // Allocate and init background scan state struct
    bg_scan_t *bg_scan = malloc(sizeof(bg_scan_t));
    NPC(bg_scan);
    bg_scan->scanned_ap_count = AP_SCAN_MAX_APS;
    memset(&bg_scan->scanned_aps, 0, sizeof(bg_scan->scanned_aps));

    // Register handler
    ESP_EC(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                      handle_scan_complete, bg_scan));

    // Perform the scan
    ESP_EC(esp_wifi_set_country(&SCAN_COUNTRY));
    ESP_EC(esp_wifi_scan_start(&SCAN_CONFIG, true));
    ESP_EC(esp_wifi_scan_stop());

    // Unregister handler
    ESP_EC(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                        handle_scan_complete));

    // Filter the access point list
    filter_scan_results(bg_scan);

    return bg_scan;
}

void ll_destroy_scan(bg_scan_t *scan) {
    NPC(scan);
    free(scan);
}
