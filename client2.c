#include "tftp.h"
#include "fsm.h"
#include "print.h"

void usage(const char* program_name);

int main(int argc, char* argv[]) {
    state_t cur_state;
    session_data_t data;
    struct hostent* host;
    struct sockaddr_in client;

    if (argc != 3) {
        usage(argv[0]);
        exit(1);
    }

    bzero(&data, sizeof(data));

    if ((data.sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    bzero(&client, sizeof (client));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);
    if (bind(data.sock, (struct sockaddr *) &client, sizeof (client)) < 0) {
        fprintf(stderr, "failed to bind socket: %s\n", strerror(errno));
        return -1;
    }

    host = gethostbyname(argv[1]);
    if (host == NULL) {
        fprintf(stderr, "failed to get address for '%s': %s\n", argv[1], hstrerror(h_errno));
        close(data.sock);
        return -1;
    }

    data.peer.sin_family = AF_INET;
    memcpy(&data.peer.sin_addr.s_addr, host->h_addr, host->h_length);
    data.peer.sin_port = htons(TFTP_PORT);

    data.role = ROLE_WRITER;
    data.mode = MODE_OCTET;

    strncpy(data.filename, argv[2], MAX_FILENAME);

    cur_state = STATE_INITIAL_CLIENT;

    while (data.quit != 1) {
        cur_state = run_state(cur_state, &data);
    }

    return 0;
}

void usage(const char* program_name) {
    printf("usage: %s <address> <filename>\n", program_name);
}
