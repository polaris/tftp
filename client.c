#include "tftp.h"
#include "print.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/errno.h>
#include <unistd.h>

void usage(const char* program_name);

int tftp_write_file(int fd, const char* to, const char* address, mode_t mode);
int tftp_read_file(const char* from, const char* to, const char* address, mode_t mode);

int initiate_connection(const char* address, const char* filename, mode_t mode, opcode_t opcode, struct connection* conn);

int main(int argc, char* argv[]) {
    int inputfile;
    // int result, ch, a, r, l;
    // char address[255];
    // char remote[128];
    // char local[128];

    // static struct option longopts[] = {
    //     { "address",    required_argument, NULL, 'a' },
    //     { "remote",     required_argument, NULL, 'r' },
    //     { "local",      required_argument, NULL, 'l' },
    //     { "method",     required_argument, NULL, 'm' },
    //     { "help",       no_argument,       NULL, 'h' },
    //     { NULL,         0,                 NULL, 0   }
    // };

    if (argc != 4) {
        usage(argv[0]);
        exit(1);
    }

    // a = r = l = 0;

    // while ((ch = getopt_long(argc, argv, "a:r:l:m:h", longopts, NULL)) != -1) {
    //     switch (ch) {
    //     case 'a':
    //         a = 1;
    //         strncpy(address, optarg, 255);
    //         break;
    //     case 'r':
    //         r = 1;
    //         strncpy(remote, optarg, 128);
    //         break;
    //     case 'l':
    //         l = 1;
    //         strncpy(local, optarg, 128);
    //         break;
    //     case 'h':
    //         usage(argv[0]);
    //         exit(0);
    //     case 0:
    //         break;
    //     default:
    //         usage(argv[0]);
    //     }
    // }


    // printf("%s, %s, %s", address, remote, local);

    // argc -= optind;
    // argv += optind;

    if ((inputfile = open(argv[3], O_RDONLY)) < 0) {
        print_system_error("Failed to open input file");
        return -1;
    }

    int result = tftp_write_file(inputfile, argv[2], argv[1], MODE_OCTET);
 
    close(inputfile);

    return result;
}

void usage(const char* program_name) {
    printf("usage: %s <address> <remote filename> <local filename>\n", program_name);
}

int tftp_write_file(int inputfile, const char* to, const char* address, mode_t mode) {
    ssize_t count, size;
    size_t buflen;
    char packet[BSIZE], data[DATASIZE];
    socklen_t server_len;
    bnum_t blocknumber;
    tid_t tid;
    opcode_t opcode;
    struct connection conn;
    struct sockaddr_in endpoint;

    if (initiate_connection(address, to, mode, OP_WRQ, &conn) != 0) {
        return -1;
    }

    blocknumber = 0;
    tid = 0;
    bzero(&endpoint, sizeof(endpoint));

    do {
        server_len = sizeof(endpoint);
        if ((count = recvfrom(conn.sock, packet, BSIZE, 0, (struct sockaddr *)&endpoint, &server_len)) < 0) {
            print_system_error("Failed to receive ack from server");
            close(conn.sock);
            return -1;
        }

        opcode = parse_opcode(packet);
        if (IS_ERROR(opcode)) {
            print_packet_error(packet, (size_t)count);
            close(conn.sock);
            return -1;
        } else if (IS_ACK(opcode)) {
            if (parse_blocknumber(packet) != blocknumber) {
                print_application_error("Invalid block number");
                close(conn.sock);
                return -1;
            } else {
                if (blocknumber == 0) {
                    tid = ntohs(endpoint.sin_port);
                    conn.endpoint = endpoint;
                } else {
                    if (tid != ntohs(endpoint.sin_port) || endpoint.sin_addr.s_addr != conn.endpoint.sin_addr.s_addr) {
                        buflen = create_error_packet(packet, EUNKNOWN);
                        if (sendto(conn.sock, packet, buflen, 0, (struct sockaddr *)&conn.endpoint, sizeof(conn.endpoint)) < 0) {
                            print_system_error("Failed to send data");
                            close(conn.sock);
                            return -1;
                        }
                    }
                }
            }
        } else {
            print_application_error("Unexpected opcode returned from server");
            close(conn.sock);
            return -1;
        }

        blocknumber += 1;

        size = read(inputfile, data, DATASIZE);
        if (size == EOF) {
            break;
        }

        buflen = create_data_packet(packet, blocknumber, data, size);
        if (sendto(conn.sock, packet, buflen, 0, (struct sockaddr *)&conn.endpoint, sizeof(conn.endpoint)) < 0) {
            print_system_error("Failed to send data");
            close(conn.sock);
            return -1;
        }
    } while (size == 512);

    close(conn.sock);

    return 0;
}

int tftp_read_file(const char* from, const char* to, 
                    const char* address, mode_t mode) {
    int outputfile;
    ssize_t count;
    char packet[BSIZE];
    socklen_t server_len;

    struct connection conn;
    if (initiate_connection(address, from, mode, OP_RRQ, &conn) != 0) {
        return -1;
    }
    
    if ((outputfile = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
        fprintf(stderr, "failed to open output file '%s': %s",
            to, strerror(errno));
        close(conn.sock);
        return -1;
    }

    do {
        server_len = sizeof(conn.endpoint);
        count = recvfrom(conn.sock, packet, BSIZE, 0,
            (struct sockaddr *)&conn.endpoint, &server_len);
        if (IS_ERROR(parse_opcode(packet))) {
            fprintf(stderr, "tftp: %s\n", packet + 4);
        } else {
            write(outputfile, packet+4, (size_t)(count-4));

            /* Send an ACK packet. The block number we want to ack is already
               in the packet so we just need to change the opcode. Note that
               the ACK is sent to the port number which the server just sent
               the data from, NOT to port 69. */
            *(short *)packet = htons(OP_ACK);
            sendto(conn.sock, packet, 4, 0, (struct sockaddr *)&conn.endpoint, 
                sizeof(conn.endpoint));
        }
    } while (count == BSIZE);

    close(outputfile);
    close(conn.sock);

    return 0;
}

int initiate_connection(const char* address, const char* filename, mode_t mode, opcode_t opcode, struct connection* conn) {
    ssize_t count;
    size_t buflen;
    struct hostent* host;
    char packet[BSIZE];
    struct sockaddr_in client;
    
    if ((conn->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    bzero(&client, sizeof (client));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);
    if (bind(conn->sock, (struct sockaddr *) &client, sizeof (client)) < 0) {
        fprintf(stderr, "failed to bind socket: %s\n", strerror(errno));
        return -1;
    }


    host = gethostbyname(address);
    if (host == NULL) {
        fprintf(stderr, "failed to get address for '%s': %s\n", address, hstrerror(h_errno));
        close(conn->sock);
        return -1;
    }

    bzero((void *)&conn->endpoint, sizeof(conn->endpoint));
    conn->endpoint.sin_family = AF_INET;
    memcpy(&conn->endpoint.sin_addr.s_addr, host->h_addr, host->h_length);
    conn->endpoint.sin_port = htons(TFTP_PORT);

    buflen = create_initial_packet(packet, filename, mode, opcode);

    count = sendto(conn->sock, packet, buflen, 0,
        (struct sockaddr *)&conn->endpoint, sizeof(conn->endpoint));
    if (count < 0) {
        fprintf(stderr, "failed to send request: %s\n", strerror(errno));
        close(conn->sock);
        return -1;
    }

    return 0;
}
