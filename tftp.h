#ifndef TFTP_H
#define TFTP_H

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/errno.h>
#include <unistd.h>

#define OP_RRQ   1
#define IS_RRQ(op) ((op) == OP_RRQ)
#define OP_WRQ   2
#define IS_WRQ(op) ((op) == OP_WRQ)
#define OP_DATA  3
#define IS_DATA(op) ((op) == OP_DATA)
#define OP_ACK   4
#define IS_ACK(op) ((op) == OP_ACK)
#define OP_ERROR 5
#define IS_ERROR(op) ((op) == OP_ERROR)
#define IS_NOOP(op) ((op) < 1 || (op) > 5)

typedef unsigned short opcode_t;

#define MODE_NETASCII 1
#define MODE_OCTET    2

static const char* modes[2] = {
    "netascii",
    "octet"
};

typedef unsigned short mode_t;

#define ENODEF    0   /* Not defined, see error message (if any). */
#define ENOTFOUND 1   /* File not found. */
#define ENOACCESS 2   /* Access violation. */
#define EDISKFULL 3   /* Disk full or allocation exceeded. */
#define EILLOP    4   /* Illegal TFTP operation. */
#define EUNKNOWN  5   /* Unknown transfer ID. */
#define EEXISTA   6   /* File already exists. */
#define ENOUSER   7   /* No such user. */
#define EOACK     8   /* Options acknowledgment not accepted. */

#define NUM_ERROR_CODES 9

static const char* error_strings[NUM_ERROR_CODES] = {
    "Not defined, see error message (if any).",
    "File not found.",
    "Access violation.",
    "Disk full or allocation exceeded.",
    "Illegal TFTP operation.",
    "Unknown transfer ID",
    "File already exists.",
    "No such user.",
    "Options acknowledgment not accepted."
};

typedef unsigned short ecode_t;

#define ROLE_WRITER 1
#define IS_WRITER(role) ((role) == ROLE_WRITER)
#define ROLE_READER 2
#define IS_READER(role) ((role) == ROLE_READER)

typedef unsigned short role_t;

typedef unsigned short bnum_t;
typedef unsigned short tid_t;

#define BSIZE 516
#define DATASIZE 512
#define TFTP_PORT 69
#define MAX_ERRMSG 512
#define MAX_FILENAME 512
#define RECV_TIMEOUT 10

size_t create_initial_packet(char* packet, const char* filename, mode_t mode, opcode_t opcode);
size_t create_ack_packet(char* packet, bnum_t blocknumber);
size_t create_data_packet(char* packet, bnum_t blocknumber, char* data, ssize_t size);
size_t create_error_packet(char* packet, ecode_t error_code);

opcode_t parse_opcode(char* packet);
bnum_t parse_blocknumber(char* packet);
ecode_t parse_errcode(char* packet);
void parse_filename(char* packet, size_t size, char filename[MAX_FILENAME]);
void parse_errmsg(char* packet, size_t size, char errmsg[MAX_ERRMSG]);

#endif
