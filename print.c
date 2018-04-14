#include "print.h"
#include "tftp.h"

void print_packet_error(char* packet, size_t size) {
    char errmsg[MAX_ERRMSG];
    parse_errmsg(packet, (size_t)size, errmsg);
    fprintf(stderr, "tftp: %s (error code %d)\n", errmsg, parse_errcode(packet));
}

void print_system_error(char* msg) {
    fprintf(stderr, "tftp: %s: %s\n", msg, strerror(errno));
}

void print_application_error(char* msg) {
    fprintf(stderr, "tftp: %s\n", msg);
}
