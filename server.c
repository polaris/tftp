#include "tftp.h"
#include "fsm.h"
#include "print.h"

void handle_client_request(char* initial_request, size_t initial_request_size, struct sockaddr_in client);

int main() {
    int sock;
    int enable;
    ssize_t count;
    socklen_t client_len;
    struct sockaddr_in server, client;
    char packet[BSIZE];

    signal(SIGCHLD, SIG_IGN); // Avoiding zombie processes

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("creating stream socket");
        exit(1);
    }

    enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("set socket option SO_REUSEADDR");
        close(sock);
        exit(1);
    }

    bzero(&server, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(TFTP_PORT);
    if (bind(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        perror("binding socket");
        close(sock);
        exit(1);
    }

    while (1) {
        client_len = sizeof(client);
        count = recvfrom(sock, packet, BSIZE, 0, (struct sockaddr *)&client, &client_len);
        if (count < 0) {
            print_system_error("failed to receive data");
            close(sock);
            exit(1);
        }

        if (fork() == 0) { // Child process
            printf("process forked\n");
            close(sock);
            handle_client_request(packet, (size_t)count, client);
            break;
        }
    }

    return 0;
}

void handle_client_request(char* initial_request, size_t initial_request_size, struct sockaddr_in client) {
    state_t cur_state;
    session_data_t data;

    bzero(&data, sizeof(data));
    data.peer = client;
    data.tid = ntohs(client.sin_port);
    memcpy(data.packet, initial_request, initial_request_size);

    cur_state = STATE_INITIAL_SERVER;

    while (data.quit != 1) {
        cur_state = run_state(cur_state, &data);
    }
}
