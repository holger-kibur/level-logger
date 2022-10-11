#ifndef LL_RENDER_H
#define LL_RENDER_H

#define FORM_NAME_SSID "ssid"
#define FORM_NAME_PASSWORD "psk"
#define FORM_NAME_TARGET "target"
#define FORM_NAME_DEVNAME "devname"

#include "scan.h"

const char *label_authmode(wifi_auth_mode_t authmode);
void render_netlist_rows(bg_scan_t *scanned_networks);
char *render_form_page(bg_scan_t *scanned_networks);
char *render_loading_page();

#endif // LL_RENDER_H
