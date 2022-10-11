#include "render.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "scan.h"
#include "util.h"

static const char *TAG = "ll_render";

static const char *FORM_TEMPLATE =
    "<!DOCTYPE html><form action=/ method=POST>Network Name: <input "
    "name=%s><br>Password: <input name=%s><br>Target: <input "
    "name=%s><br>Device Name: <input name=%s><br><input type=submit "
    "value=Connect></form><table><tr><th>Network Name (SSID)<th>Signal "
    "Strength<th>Security Type</tr>%s</table>";
static const char *NETLIST_ROW_TEMPLATE =
    "<tr><td>%s</td><td>%s</td><td>%s</td></tr>";

static char glob_scratch_small[512];
static char glob_scratch_large[1024];

const char *label_authmode(wifi_auth_mode_t authmode) {
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        return "Open";
    case WIFI_AUTH_OWE:
    case WIFI_AUTH_MAX:
        return "Unknown";
    default:
        return "Protected";
    }
}

const char *label_rssi(int8_t rssi) {
    if (rssi >= -50) {
        return "Excellent";
    } else if (rssi >= -55) {
        return "High";
    } else if (rssi >= -75) {
        return "Medium";
    } else if (rssi >= -80) {
        return "Low";
    } else {
        return "Unreliable";
    }
}

void render_netlist_rows(bg_scan_t *scanned_networks) {
    NPC(scanned_networks);

    int cur_offset = 0;
    for (uint16_t i = 0; i < scanned_networks->scanned_ap_count; i++) {
        wifi_ap_record_t *ap_record = &scanned_networks->scanned_aps[i];
        ESP_LOGD(TAG,
                 "render_netlist_rows: snprintf(\nbuf: %p\nsize: %d\ntemplate: "
                 "%p\nssid: %p\nrssi: %s\nauthmode: %p\n)",
                 glob_scratch_small + cur_offset,
                 sizeof(glob_scratch_small) - cur_offset, NETLIST_ROW_TEMPLATE,
                 ap_record->ssid, label_rssi(ap_record->rssi),
                 label_authmode(ap_record->authmode));
        cur_offset += snprintf(glob_scratch_small + cur_offset,
                               sizeof(glob_scratch_small) - cur_offset,
                               NETLIST_ROW_TEMPLATE, ap_record->ssid,
                               label_rssi(ap_record->rssi),
                               label_authmode(ap_record->authmode));
        if (cur_offset >= sizeof(glob_scratch_small)) {
            ESP_LOGW(
                TAG,
                "Couldn't fully render network table into small scratchpad!");
            break;
        }
    }

    ESP_LOGI(TAG, "Finished rendering netlist rows:\n%s\n", glob_scratch_small);
}

char *render_form_page(bg_scan_t *scanned_networks) {
    NPC(scanned_networks);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    // First fill small scratchpad with network table
    render_netlist_rows(scanned_networks);

    // Then render full form page into large scratchpad
    int len_needed =
        snprintf(glob_scratch_large, sizeof(glob_scratch_large), FORM_TEMPLATE,
                 FORM_NAME_SSID, FORM_NAME_PASSWORD, FORM_NAME_TARGET,
                 FORM_NAME_DEVNAME, glob_scratch_small);
    if (len_needed >= sizeof(glob_scratch_large)) {
        ESP_LOGW(TAG, "Couldn't fully render form page into large scratchpad!");
    }

    ESP_LOGI(TAG, "Finished rendering form page: \n%s\n", glob_scratch_large);

    return glob_scratch_large;
}
