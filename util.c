#include "util.h"
#include <unistd.h>

int file_exists(char filename[MAX_FILENAME]) {
    return access(filename, F_OK) != -1 ? 1 : 0;
}
