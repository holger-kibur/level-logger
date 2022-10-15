#ifndef CONST_H
#define CONST_H

#define AP_SSID "Level Sensor Setup"
#define MINIMUM_NETWORK_RSSI -80
#define AP_SCAN_TIME_MS 500
#define COUNTRY_CODE                                                           \
    { 'U', 'S', 'O' } // united states, outdoor environment
#define LOWEST_CHAN 1
#define HIGHEST_CHAN 11
#define AP_SCAN_MAX_APS 8

#define PAGE_SMALL_SCRATCHPAD_SIZE 512
#define PAGE_LARGE_SCRATCHPAD_SIZE 1024
#define PAGE_TABLE_PART_NAME "page_table"
#define PAGE_CONTENT_PART_NAME "page_content"
#define PAGE_PART_TYPE 0x40
#define PAGE_PART_SUBTYPE 0x00
#define TEMPLATE_BUFFER_SIZE 1024

#endif // CONST_H
