#ifndef PRINT_H
#define PRINT_H

#include <ctype.h>

void print_packet_error(char* packet, size_t size);
void print_system_error(char* msg);
void print_application_error(char* msg);

#endif
