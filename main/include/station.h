#ifndef STATION_H
#define STATION_H

#include "pthread.h"

typedef enum conn_attempt_state_t {
    cas_Initial,
    cas_StartedConnection,
    cas_Failed,
    cas_ConnectSuccess,
    cas_DhcpSuccess,
} conn_attempt_state_t;

typedef struct conn_attempt_t {
    pthread_mutex_t mutex;
    pthread_cond_t state_changed;

    // SYNCHRONIZED FIELDS
    conn_attempt_state_t state;
    uint8_t fail_reason;

} conn_attempt_t;

void ll_station_init();
void ll_station_set_network_params(char *ssid, char *pass);
conn_attempt_t *ll_station_create_conn_attempt();
void ll_station_destroy_conn_attempt(conn_attempt_t *conn_attempt);
void ll_station_start_conn_fsm(conn_attempt_t *conn_attempt);
void ll_station_stop_conn_fsm(conn_attempt_t *conn_attempt);
void ll_station_wait_for_change(conn_attempt_t *conn_attempt);
conn_attempt_state_t ll_station_get_state(conn_attempt_t *conn_attempt);
uint8_t ll_station_get_fail_reason(conn_attempt_t *conn_attempt);

#endif // STATION_H
