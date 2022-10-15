#ifndef LL_RENDER_H
#define LL_RENDER_H

#define FORM_NAME_SSID "ssid"
#define FORM_NAME_PASSWORD "psk"
#define FORM_NAME_TARGET "target"
#define FORM_NAME_DEVNAME "devname"
#define KEY_LEN 32

#include "scan.h"

typedef struct page_entry_t {
    char key[KEY_LEN];
    int offset;
    int length;
} page_entry_t;

typedef struct page_table_t {
    page_entry_t *entries;
    uint32_t num_entries;
} page_table_t;

void init_page_table();
const char *label_authmode(wifi_auth_mode_t authmode);
void render_netlist_rows(bg_scan_t *scanned_networks);
char *render_form_page(bg_scan_t *scanned_networks);
char *render_loading_page();

#endif // LL_RENDER_H
