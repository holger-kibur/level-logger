#include "render.h"

#include "const.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_wifi_types.h"
#include "scan.h"
#include "util.h"

#include <string.h>

static const char *TAG = "ll_render";

static page_table_t glob_page_table;
static char glob_template[TEMPLATE_BUFFER_SIZE];
static char glob_scratch_small[PAGE_SMALL_SCRATCHPAD_SIZE];
static char glob_scratch_large[PAGE_LARGE_SCRATCHPAD_SIZE];

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

void init_page_table() {
    const esp_partition_t *table_part = esp_partition_find_first(
        PAGE_PART_TYPE,
        PAGE_PART_SUBTYPE,
        PAGE_TABLE_PART_NAME);

    // First thing in partition is a 32 bit unsigned integer telling us how
    // much memory to allocate to store the table string.
    uint32_t table_len = 0;
    ESP_EC(esp_partition_read(table_part, 0, &table_len, 4));

    // Next thing in the partition is a 32 bit unsigned integer telling us how
    // many entries are in the table.
    uint32_t num_entries = 0;
    ESP_EC(esp_partition_read(table_part, 4, &num_entries, 4));

    ESP_LOGI(
        TAG,
        "Page table num entries: %ld, length: %ld",
        num_entries,
        table_len);

    // Allocate memory to store the rest of the table
    char *table_buffer = malloc(table_len + 1); // Store null terminator
    NPC(table_buffer);

    // Allocate memory to store the entries
    page_entry_t *entries = malloc(sizeof(page_entry_t) * num_entries);
    NPC(entries);

    // Read the table into the buffer
    // There is a newline after the first 8 bytes.
    ESP_EC(esp_partition_read(table_part, 8 + 1, table_buffer, table_len));

    // Clip off trailing newline with null terminator
    table_buffer[table_len - 1] = '\0';

    // Parse the entries from the table
    char *cursor = table_buffer;
    for (int i = 0; i < num_entries; i++) {
        if (!cursor) {
            ESP_LOGE(TAG, "Reached end of page table prematurely. Aborting!");
            abort();
        }
        char *entry_row = strtok_r(cursor, "\n", &cursor);
        if (sscanf(
                entry_row,
                "%31s %d %d",
                entries[i].key,
                &entries[i].offset,
                &entries[i].length) != 3) {
            ESP_LOGE(TAG, "Couldn't scan page table row: %s", entry_row);
            abort();
        }

        // Check if the size is small enough
        if (entries[i].length >= TEMPLATE_BUFFER_SIZE) {
            ESP_LOGE(
                TAG,
                "Page %s is too big to fit in template buffer!\nSize: "
                "%d\nMaximum: %d",
                entries[i].key,
                entries[i].length,
                TEMPLATE_BUFFER_SIZE - 1);
            abort();
        }
    }

    // Make sure we parsed all rows
    if (cursor != NULL) {
        ESP_LOGW(TAG, "More table rows left in table!");
    }

    // Move page entry array to global struct
    glob_page_table.entries = entries;
    entries = NULL;
    glob_page_table.num_entries = num_entries;
    // Deallocate table buffer
    free(table_buffer);

    ESP_LOGI(TAG, "Page table initialized (key, offset, length):");
    for (int i = 0; i < glob_page_table.num_entries; i++) {
        page_entry_t *entry = &glob_page_table.entries[i];
        ESP_LOGI(TAG, "%s %d %d", entry->key, entry->offset, entry->length);
    }
}

void load_page_template(const char *page_key) {
    for (int i = 0; i < glob_page_table.num_entries; i++) {
        page_entry_t *entry = &glob_page_table.entries[i];
        if (strcmp(page_key, entry->key) == 0) {
            const esp_partition_t *content_part = esp_partition_find_first(
                PAGE_PART_TYPE,
                PAGE_PART_SUBTYPE,
                PAGE_CONTENT_PART_NAME);
            ESP_EC(esp_partition_read(
                content_part,
                entry->offset,
                glob_template,
                entry->length));
            glob_template[entry->length] = '\0';
            goto template_loaded;
        }
    }

    // Couldn't find the page in the table
    ESP_LOGE(TAG, "Couldn't find page with key %s in the table!", page_key);

template_loaded:;
}

void render_netlist_rows(bg_scan_t *scanned_networks) {
    NPC(scanned_networks);

    load_page_template("netlist_item");

    int cur_offset = 0;
    for (uint16_t i = 0; i < scanned_networks->scanned_ap_count; i++) {
        wifi_ap_record_t *ap_record = &scanned_networks->scanned_aps[i];
        ESP_LOGD(
            TAG,
            "render_netlist_rows: snprintf(\nbuf: %p\nsize: %d\ntemplate: "
            "%p\nssid: %p\nrssi: %s\nauthmode: %p\n)",
            glob_scratch_small + cur_offset,
            sizeof(glob_scratch_small) - cur_offset,
            glob_template,
            ap_record->ssid,
            label_rssi(ap_record->rssi),
            label_authmode(ap_record->authmode));
        cur_offset += snprintf(
            glob_scratch_small + cur_offset,
            sizeof(glob_scratch_small) - cur_offset,
            glob_template,
            ap_record->ssid,
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
    load_page_template("form_template");
    int len_needed = snprintf(
        glob_scratch_large,
        sizeof(glob_scratch_large),
        glob_template,
        FORM_NAME_SSID,
        FORM_NAME_PASSWORD,
        FORM_NAME_TARGET,
        FORM_NAME_DEVNAME,
        glob_scratch_small);
    if (len_needed >= sizeof(glob_scratch_large)) {
        ESP_LOGW(TAG, "Couldn't fully render form page into large scratchpad!");
    }

    ESP_LOGI(TAG, "Finished rendering form page: \n%s\n", glob_scratch_large);

    return glob_scratch_large;
}
