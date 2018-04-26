#include "util.h"
#include <unistd.h>
#include <sys/statvfs.h>

int file_exists(char filename[MAX_FILENAME]) {
    return access(filename, F_OK) != -1 ? 1 : 0;
}

int file_write_ok(char filename[MAX_FILENAME]) {
    return access(filename, W_OK) != -1 ? 1 : 0;
}

int file_read_ok(char filename[MAX_FILENAME]) {
    return access(filename, R_OK) != -1 ? 1 : 0;
}

unsigned long long get_free_space(char* path) {
    unsigned long long result;
    struct statvfs sfs;

    result = 0;
    if (statvfs(path, &sfs) != -1) {
        result = (unsigned long long)sfs.f_bsize * sfs.f_bfree;
    }

    return result;
}
