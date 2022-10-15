#ifndef LL_SCAN_H
#define LL_SCAN_H

#include "const.h"
#include "esp_wifi_types.h"

#include <stdbool.h>

typedef struct bg_scan_t {
    uint16_t scanned_ap_count;
    wifi_ap_record_t scanned_aps[AP_SCAN_MAX_APS];
} bg_scan_t;

bg_scan_t *ll_do_scan();
void ll_destroy_scan(bg_scan_t *scan);

#endif // LL_SCAN_H
