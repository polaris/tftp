#ifndef FSM_H
#define FSM_H

#include "tftp.h"

typedef enum { STATE_INITIAL_SERVER, STATE_INITIAL_CLIENT, STATE_RECEIVE, STATE_SEND, STATE_WAIT, STATE_EXIT, NUM_STATES } state_t;

typedef struct session_data {
    int sock;
    int fd;
    int complete;
    int quit;
    size_t packet_size;
    bnum_t blocknumber;
    role_t role;
    mode_t mode;
    tid_t tid;
    char packet[BSIZE];
    char filename[MAX_FILENAME];
    struct sockaddr_in peer;
} session_data_t;

state_t run_state(state_t cur_state, session_data_t* data);

#endif
