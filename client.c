#include "tftp.h"
#include "fsm.h"
#include "print.h"

#include <getopt.h>

void usage(const char* program_name);

int main(int argc, char* argv[]) {
    state_t cur_state;
    session_data_t data;
    struct hostent* host;
    struct sockaddr_in client;
    char command[4];
    char address[255];
    char filename[MAX_FILENAME];
    int ch, a, f, c, netascii;

    struct option longopts[] = {
        { "address",  required_argument, NULL, 'a' },
        { "filename", required_argument, NULL, 'r' },
        { "commnad",  required_argument, NULL, 'c' },
        { "netascii", no_argument,       NULL, 'n' },
        { "help",     no_argument,       NULL, 'h' },
        { NULL,       0,                 NULL, 0   }
    };

    a = f = c = netascii = 0;

    while ((ch = getopt_long(argc, argv, "a:f:c:m:h", longopts, NULL)) != -1) {
        switch (ch) {
        case 'a':
            a = 1;
            strncpy(address, optarg, 255);
            break;
        case 'f':
            f = 1;
            strncpy(filename, optarg, 128);
            break;
        case 'c':
            c = 1;
            strncpy(command, optarg, 4);
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        case 'n':
            netascii = 1;
            break;
        case 0:
            break;
        default:
            usage(argv[0]);
        }
    }

    if (a == 0 || f == 0 || c == 0) {
        usage(argv[0]);
        exit(1);
    }

    argc -= optind;
    argv += optind;

    bzero(&data, sizeof(data));

    if (strncasecmp(command, "put", 3) == 0) {
        data.role = ROLE_WRITER;
    } else if (strncasecmp(command, "get", 3) == 0) {
        data.role = ROLE_READER;
    } else {
        print_application_error("unknown command");
        return -1;
    }

    if ((data.sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        print_system_error("failed to create socket");
        return -1;
    }

    bzero(&client, sizeof (client));
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);
    if (bind(data.sock, (struct sockaddr *) &client, sizeof (client)) < 0) {
        print_system_error("failed to bind socket");
        return -1;
    }

    host = gethostbyname(address);
    if (host == NULL) {
        print_application_error("failed to get host by name");
        close(data.sock);
        return -1;
    }

    data.peer.sin_family = AF_INET;
    memcpy(&data.peer.sin_addr.s_addr, host->h_addr, host->h_length);
    data.peer.sin_port = htons(TFTP_PORT);

    // data.mode = netascii == 1 ? MODE_NETASCII : MODE_OCTET;
    data.mode = MODE_OCTET;

    strncpy(data.filename, filename, MAX_FILENAME);

    cur_state = STATE_INITIAL_CLIENT;

    while (data.quit != 1) {
        cur_state = run_state(cur_state, &data);
    }

    return 0;
}

void usage(const char* program_name) {
    printf("usage: %s -a <address> -f <filename> -c <command>\n", program_name);
}
