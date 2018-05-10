#include "fsm.h"
#include "util.h"
#include "print.h"

state_t do_state_initial_server(session_data_t* data);
state_t do_state_initial_client(session_data_t* data);
state_t do_state_receive(session_data_t* data);
state_t do_state_send(session_data_t* data);
state_t do_state_wait(session_data_t* data);
state_t do_state_exit(session_data_t* data);

state_t handle_ack(session_data_t* data);
state_t handle_data(session_data_t* data);
state_t handle_error(session_data_t* data);

typedef state_t state_func_t(session_data_t* data);

static state_func_t* const state_table[NUM_STATES] = {
    do_state_initial_server, do_state_initial_client, do_state_receive, do_state_send, do_state_wait, do_state_exit
};

state_t run_state(state_t cur_state, session_data_t* data) {
    return state_table[cur_state](data);
}

state_t do_state_initial_server(session_data_t* data) {
    struct sockaddr_in server;
    opcode_t opcode;
    ssize_t size;
    char buffer[DATASIZE];
    char filename[MAX_FILENAME];

    data->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (data->sock < 0) {
        perror("creating stream socket");
        return STATE_EXIT;
    }

    struct timeval tv;
    tv.tv_sec = RECV_TIMEOUT;
    tv.tv_usec = 0;
    if (setsockopt(data->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        print_system_error("failed to set receive timeout");
        return STATE_EXIT;
    }

    bzero(&server, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(0); // Let the system choose a random port.
    if (bind(data->sock, (struct sockaddr *) &server, sizeof (server)) < 0) {
        perror("binding socket");
        return STATE_EXIT;
    }

    opcode = parse_opcode(data->packet);
    if (!IS_RRQ(opcode) && !IS_WRQ(opcode)) {
        data->packet_size = create_error_packet(data->packet, EILLOP);
        data->complete = 1;
        return STATE_SEND;
    }

    data->mode = parse_mode(data->packet);

    bzero(filename, MAX_FILENAME);
    parse_filename(data->packet, data->packet_size, filename);

    if (IS_RRQ(opcode)) {
        data->role = ROLE_WRITER;
        if (!file_exists(filename)) {
            data->packet_size = create_error_packet(data->packet, ENOTFOUND);
            data->complete = 1;
            return STATE_SEND;
        }
        if (!file_read_ok(filename)) {
            data->packet_size = create_error_packet(data->packet, ENOACCESS);
            data->complete = 1;
            return STATE_SEND;
        }
        if ((data->fd = open(filename, O_RDONLY)) < 0) {
            data->packet_size = create_error_packet(data->packet, ENODEF);
            data->complete = 1;
            return STATE_SEND;
        }
        bzero(buffer, DATASIZE);
        size = read(data->fd, buffer, DATASIZE);
        if (size < 0) {
            print_system_error("failed to read data from file");
            data->packet_size = create_error_packet(data->packet, ENODEF);
            data->complete = 1;
            return STATE_SEND;
        }
        data->blocknumber = 1;
        data->packet_size = create_data_packet(data->packet, data->blocknumber, buffer, size);
    } else {
        data->role = ROLE_READER;
        if (file_exists(filename)) {
            data->packet_size = create_error_packet(data->packet, EEXISTA);
            data->complete = 1;
            return STATE_SEND;
        }
        if (!file_write_ok(filename)) {
            data->packet_size = create_error_packet(data->packet, ENOACCESS);
            data->complete = 1;
            return STATE_SEND;
        }
        if ((data->fd = open(filename, O_WRONLY | O_CREAT, 0666)) < 0) {
            data->packet_size = create_error_packet(data->packet, ENODEF);
            data->complete = 1;
            return STATE_SEND;
        }
        data->blocknumber = 0;
        data->packet_size = create_ack_packet(data->packet, data->blocknumber);
    }

    return STATE_SEND;
}

state_t do_state_initial_client(session_data_t* data) {
    opcode_t opcode;

    if (IS_WRITER(data->role)) {
        if (!file_exists(data->filename)) {
            print_application_error("source file does not exist");
            return STATE_EXIT;
        }
        if (!file_read_ok(data->filename)) {
            print_application_error("source file is not readable");
            return STATE_EXIT;
        }
        if ((data->fd = open(data->filename, O_RDONLY)) < 0) {
            print_system_error("failed to open source file");
            return STATE_EXIT;
        }
        opcode = OP_WRQ;
    } else {
        if (file_exists(data->filename)) {
            if (!file_write_ok(data->filename)) {
                print_application_error("source file is not writable");
                return STATE_EXIT;
            }
        }
        if ((data->fd = open(data->filename, O_WRONLY | O_CREAT, 0666)) < 0) {
            print_system_error("failed to open destination file");
            return STATE_EXIT;
        }
        opcode = OP_RRQ;
    }

    data->packet_size = create_initial_packet(data->packet, data->filename, data->mode, opcode);
    if (data->packet_size == 0) {
        print_application_error("failed to create initial packet");
        return STATE_EXIT;
    }

    return STATE_SEND;
}

state_t handle_ack(session_data_t* data) {
    bnum_t blocknumber;
    ssize_t size;
    char buffer[DATASIZE];
    char* p;
    char c;
    int i;

    if (!IS_WRITER(data->role)) {
        print_application_error("illegal opcode received");
        data->packet_size = create_error_packet(data->packet, EILLOP);
        data->complete = 1;
        return STATE_SEND;
    }

    blocknumber = parse_blocknumber(data->packet);
    if (blocknumber != data->blocknumber) {
        print_application_error("unexpected blocknumber received");
        data->packet_size = create_error_packet(data->packet, EILLOP);
        data->complete = 1;
        return STATE_SEND;
    }

    bzero(buffer, DATASIZE);
    if (IS_MODE_OCTET(data->mode)) {
        printf("octet\n");
        size = read(data->fd, buffer, DATASIZE);
        if (size < 0) {
            print_system_error("failed to read data from file");
            data->packet_size = create_error_packet(data->packet, ENODEF);
            data->complete = 1;
            return STATE_SEND;
        }
    } else {
        p = buffer;
        for (i = 0; i < DATASIZE; i++) {
            if (data->newline) {
                if (data->prevchar == '\n') {
                    c = '\n';
                } else {
                    c = '\0';
                }
                data->newline = 0;
            } else {
                size = read(data->fd, &c, 1);
                if (c == EOF) {
                    break;
                }
                if (c == '\n' || c == '\r') {
                    data->prevchar = c;
                    c = '\r';
                    data->newline = 1;
                }
            }
            *p++ = c;
        }
        size = p - buffer;
    }

    data->blocknumber += 1;
    data->packet_size = create_data_packet(data->packet, data->blocknumber, buffer, size);

    if (size < DATASIZE) {
        data->complete = 1;
    }

    return STATE_SEND;
}

state_t handle_data(session_data_t* data) {
    bnum_t blocknumber;
    char* p;
    char c;
    size_t count;

    if (!IS_READER(data->role)) {
        print_application_error("illegal opcode received");
        data->packet_size = create_error_packet(data->packet, EILLOP);
        data->complete = 1;
        return STATE_SEND;
    }

    if (get_free_space(data->filename) < data->packet_size-4) {
        print_application_error("disk full");
        data->packet_size = create_error_packet(data->packet, EDISKFULL);
        data->complete = 1;
        return STATE_SEND;
    }

    if (IS_MODE_OCTET(data->mode)) {
        printf("octet\n");
        if (write(data->fd, data->packet+4, (size_t)(data->packet_size-4)) < 0) {
            print_system_error("failed to write data to disk");
            data->packet_size = create_error_packet(data->packet, ENODEF);
            data->complete = 1;
            return STATE_SEND;
        }
    } else {
        count = data->packet_size - 4;
        p  = data->packet + 4;
        while (count--) {
            c = *p++;
            if (data->prevchar == '\r') {
                if (c == '\n') {
                    lseek(data->fd, -1, SEEK_CUR);
                } else if (c == '\0') {
                    goto skipit;
                }
            }
            write(data->fd, &c, 1);
        skipit:
            data->prevchar = c;
        }
    }

    blocknumber = parse_blocknumber(data->packet);
    if (data->packet_size < BSIZE) {
        data->complete = 1;
    }
    data->packet_size = create_ack_packet(data->packet, blocknumber);
    return STATE_SEND;
}

state_t handle_error(session_data_t* data) {
    print_packet_error(data->packet, data->packet_size);
    return STATE_EXIT;
}

state_t do_state_receive(session_data_t* data) {
    state_t new_state;
    opcode_t opcode;

    opcode = parse_opcode(data->packet);
    switch (opcode) {
        case OP_ACK:
            new_state = handle_ack(data);
            break;
        case OP_DATA:
            new_state = handle_data(data);
            break;
        case OP_ERROR:
            new_state = handle_error(data);
            break;
        default:
            data->packet_size = create_error_packet(data->packet, EILLOP);
            data->complete = 1;
            new_state = STATE_SEND;
            break;
    }

    return new_state;
}

state_t do_state_send(session_data_t* data) {
    if (sendto(data->sock, data->packet, data->packet_size, 0, (struct sockaddr *)&data->peer, sizeof(data->peer)) < 0) {
        print_system_error("failed to send packet");
        data->packet_size = create_error_packet(data->packet, ENODEF);
        data->complete = 1;
        return STATE_SEND;
    }
    if (data->complete == 1) {
        return STATE_EXIT;
    }
    return STATE_WAIT;
}

state_t do_state_wait(session_data_t* data) {
    socklen_t peer_len;
    ssize_t count;

    peer_len = sizeof(data->peer);
    count = recvfrom(data->sock, data->packet, BSIZE, 0, (struct sockaddr *)&data->peer, &peer_len);
    if (count < 0) {
        data->packet_size = create_error_packet(data->packet, ENODEF);
        data->complete = 1;
        return STATE_SEND;
    }

    if (data->tid == 0) {
        data->tid = ntohs(data->peer.sin_port);
    }

    if (ntohs(data->peer.sin_port) != data->tid) {
        data->packet_size = create_error_packet(data->packet, EUNKNOWN);
        return STATE_SEND;
    }

    data->packet_size = (size_t)count;

    return STATE_RECEIVE;
}

state_t do_state_exit(session_data_t* data) {
    close(data->sock);
    close(data->fd);
    data->quit = 1;
    return STATE_EXIT;
}
