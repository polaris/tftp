#ifndef UTIL_H
#define UTIL_H

#include "tftp.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int file_exists(char filename[MAX_FILENAME]);

#endif