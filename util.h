#ifndef UTIL_H
#define UTIL_H

#include "tftp.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int file_exists(char filename[MAX_FILENAME]);
int file_write_ok(char filename[MAX_FILENAME]);
int file_read_ok(char filename[MAX_FILENAME]);
unsigned long long get_free_space(char* path);

#endif
