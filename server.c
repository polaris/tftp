#include "tftp.h"
#include "util.h"
#include "print.h"

void handle_client_request(char* initial_request, size_t initial_request_size, struct sockaddr_in client);
void handle_read_request(int sock, struct sockaddr_in client, char filename[MAX_FILENAME]);
void handle_write_request(int sock, struct sockaddr_in client, char filename[MAX_FILENAME]);

int send_ack(int sock, struct sockaddr_in peer, bnum_t blocknumber);
int send_error(int sock, struct sockaddr_in peer, ecode_t ecode);

int main() {
    int sock;
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
            close(sock);
            handle_client_request(packet, (size_t)count, client);
            break;
        }
    }

    return 0;
}

void handle_client_request(char* initial_request, size_t initial_request_size, struct sockaddr_in client) {
    opcode_t opcode;
    char filename[MAX_FILENAME];
    int sock;
    struct sockaddr_in server;

    opcode = parse_opcode(initial_request);
    bzero(filename, MAX_FILENAME);
    parse_filename(initial_request, initial_request_size, filename);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("creating stream socket");
        exit(1);
    }

    bzero(&server, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(0); // Let the system choose a random port.
    if (bind(sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        perror("binding socket");
        close(sock);
        exit(1);
    }

    if (IS_RRQ(opcode)) {
        // handle_read_request(sock, client, filename);
    } else if (IS_WRQ(opcode)) {
        if (send_ack(sock, client, 0) == 0) {
            handle_write_request(sock, client, filename);
        }
    }

    close(sock);
}

// void handle_read_request(int sock, struct sockaddr_in client, char filename[MAX_FILENAME]) {
// }

void handle_write_request(int sock, struct sockaddr_in peer, char filename[MAX_FILENAME]) {
    int outputfile;
    ssize_t count;
    socklen_t peer_len;
    bnum_t blocknumber;
    char packet[BSIZE];
    opcode_t opcode;

    if (file_exists(filename)) {
        send_error(sock, peer, EEXISTA);
        return;
    }

    if ((outputfile = open(filename, O_WRONLY | O_CREAT, 0666)) < 0) {
        print_system_error("failed to open input file");
        send_error(sock, peer, ENODEF);
        return;
    }

    do {
        peer_len = sizeof(peer);
        // TODO: implement receive timeout
        count = recvfrom(sock, packet, BSIZE, 0, (struct sockaddr *)&peer, &peer_len);
        opcode = parse_opcode(packet);
        if (IS_ERROR(opcode)) {
            fprintf(stderr, "tftp: %s\n", packet + 4);
            break;
        } else if (IS_DATA(opcode)) {
            if (write(outputfile, packet+4, (size_t)(count-4)) < 0) {
                break;
            }
            blocknumber = parse_blocknumber(packet);
            if (send_ack(sock, peer, blocknumber) != 0) {
                break;
            }
        } else {
            send_error(sock, peer, EILLOP);
            break;
        }
    } while (count == BSIZE);

    close(outputfile);
}

int send_ack(int sock, struct sockaddr_in peer, bnum_t blocknumber) {
    char packet[BSIZE];
    size_t buflen;

    buflen = create_ack_packet(packet, blocknumber);
    if (sendto(sock, packet, buflen, 0, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        print_system_error("failed to send packet");
        return -1;
    }

    return 0;
}

int send_error(int sock, struct sockaddr_in peer, ecode_t ecode) {
    char packet[BSIZE];
    size_t buflen;

    buflen = create_error_packet(packet, ecode);
    if (sendto(sock, packet, buflen, 0, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        print_system_error("failed to send packet");
        return -1;
    }

    return 0;
}
