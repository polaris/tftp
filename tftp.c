#include "tftp.h"
#include "util.h"

size_t create_initial_packet(char* packet, const char* filename, mode_t mode, opcode_t opcode) {
    char* p;
    opcode_t o;
    size_t len;

    len = strlen(filename);
    if (len > MAX_FILENAME) {
        return 0;
    }

    bzero(packet, BSIZE);
    p = packet;
    o = htons(opcode);
    memcpy(p, &o, sizeof(o));
    p += 2;
    strlcpy(p, filename, len + 1);
    p += len + 1;
    len = strlen(modes[mode]);
    strlcpy(p, modes[mode], len + 1);
    p += len + 1;
    return (size_t)(p - packet);
}

size_t create_ack_packet(char* packet, bnum_t blocknumber) {
    char* p;
    opcode_t o;
    bnum_t b;
    bzero(packet, BSIZE);
    p = packet;
    o = htons(OP_ACK);
    memcpy(p, &o, sizeof(o));
    p += 2;
    b = htons(blocknumber);
    memcpy(p, &b, sizeof(b));
    p += 2;
    return (size_t)(p - packet);
}

size_t create_data_packet(char* packet, bnum_t blocknumber, char* data, ssize_t size) {
    char* p;
    opcode_t o;
    bnum_t b;

    if (size > DATASIZE) {
        return 0;
    }

    bzero(packet, BSIZE);
    p = packet;
    o = htons(OP_DATA);
    memcpy(p, &o, sizeof(o));
    p += 2;
    b = htons(blocknumber);
    memcpy(p, &b, sizeof(b));
    p += 2;
    memcpy(p, data, size);
    p += size;
    return (size_t)(p - packet);
}

size_t create_error_packet(char* packet, ecode_t error_code) {
    char* p;
    opcode_t o;
    ecode_t e;
    bzero(packet, BSIZE);
    p = packet;
    o = htons(OP_ERROR);
    memcpy(p, &o, sizeof(o));
    p += 2;
    e = htons(error_code);
    memcpy(p, &e, sizeof(e));
    p += 2;
    strcpy(p, error_strings[error_code]);
    p += strlen(error_strings[error_code]) + 1;
    return (size_t)(p - packet);
}

opcode_t parse_opcode(char* packet) {
    opcode_t o;
    memcpy(&o, packet, sizeof(o));
    return ntohs(o);
}

mode_t parse_mode(char* packet) {
    char* p;
    p = strchr(packet + 2, '\0');
    return (strcmp(p + 1, modes[MODE_NETASCII]) == 0) ? MODE_NETASCII : MODE_OCTET;
}

bnum_t parse_blocknumber(char* packet) {
    bnum_t o;
    memcpy(&o, packet + 2, sizeof(o));
    return ntohs(o);
}

ecode_t parse_errcode(char* packet) {
    ecode_t o;
    memcpy(&o, packet + 2, sizeof(o));
    return ntohs(o);    
}

void parse_filename(char* packet, size_t size, char filename[MAX_FILENAME]) {
    size_t offset = sizeof(unsigned short);
    size_t length = size - offset;
    memcpy(filename, packet + offset, MIN(length, MAX_FILENAME));
}

size_t parse_data(char* packet, size_t packet_size, char data[DATASIZE]) {
    memcpy(data, packet + 4, DATASIZE);
    return packet_size - 4;
}

void parse_errmsg(char* packet, size_t size, char errmsg[MAX_ERRMSG]) {
    size_t offset = 2 * sizeof(unsigned short);
    size_t length = size - offset;
    memcpy(errmsg, packet + offset, MIN(length, MAX_ERRMSG));
}
