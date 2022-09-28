#ifndef STATION_H
#define STATION_H

#include "pthread.h"

typedef enum conn_attempt_state_t {
    cas_Initial,
} conn_attempt_state_t;

typedef struct conn_attempt_t {
    pthread_mutex_t mutex;

    // SYNCHRONIZED FIELDS
    conn_attempt_state_t state;

} conn_attempt_t;

void ll_station_init();
void ll_station_set_network_params(char *ssid, char *pass);
conn_attempt_t ll_station_create_conn_attempt();
void ll_station_destroy_conn_attempt(conn_attempt_t *conn_attempt);

#endif // STATION_H
