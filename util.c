#include "util.h"
#include <unistd.h>

int file_exists(char filename[MAX_FILENAME]) {
    return access(filename, F_OK) != -1 ? 1 : 0;
}

int file_write_ok(char filename[MAX_FILENAME]) {
    return access(filename, W_OK) != -1 ? 1 : 0;
}

int file_read_ok(char filename[MAX_FILENAME]) {
    return access(filename, R_OK) != -1 ? 1 : 0;
}
