#ifndef LL_CLIENT_H
#define LL_CLIENT_H

typedef enum connect_error_t {
    cr_None,
    cr_InvalidSsid,
} connect_result_t;

connect_result_t try_connect_to_network(char *ssid, char *pass);

#endif // LL_CLIENT_H
