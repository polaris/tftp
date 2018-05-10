#include "../tftp.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_create_initial_packet_1) {
    char packet[BSIZE];
    size_t len;
    mode_t mode;
    opcode_t opcode;
    char filename[MAX_FILENAME];

    len = create_initial_packet(packet, "test/foo/bar/baz", MODE_OCTET, OP_RRQ);
    mode = parse_mode(packet);
    opcode = parse_opcode(packet);
    parse_filename(packet, len, filename);

    ck_assert_int_eq(len, 25);
    ck_assert_int_eq(mode, MODE_OCTET);
    ck_assert_int_eq(opcode, OP_RRQ);
    ck_assert_str_eq(filename, "test/foo/bar/baz");
} END_TEST

START_TEST(test_create_initial_packet_2) {
    char packet[BSIZE];
    size_t len;
    mode_t mode;
    opcode_t opcode;
    char filename[MAX_FILENAME];

    len = create_initial_packet(packet, "test/foo/bar/baz", MODE_NETASCII, OP_WRQ);
    mode = parse_mode(packet);
    opcode = parse_opcode(packet);
    parse_filename(packet, len, filename);

    ck_assert_int_eq(len, 28);
    ck_assert_int_eq(mode, MODE_NETASCII);
    ck_assert_int_eq(opcode, OP_WRQ);
    ck_assert_str_eq(filename, "test/foo/bar/baz");
} END_TEST

START_TEST(test_create_ack_packet) {
    char packet[BSIZE];
    bnum_t block_number;
    size_t len;

    len = create_ack_packet(packet, 1234);
    block_number = parse_blocknumber(packet);

    ck_assert_int_eq(len, 4);
    ck_assert_int_eq(block_number, 1234);
} END_TEST

START_TEST(test_create_data_packet) {
    char packet[BSIZE];
    char input[DATASIZE];
    char data[DATASIZE];
    bnum_t block_number;
    size_t packet_size;
    size_t data_size;
    int i;

    for (i = 0; i < DATASIZE; i++) {
        input[i] = i;
    }

    packet_size = create_data_packet(packet, 1234, input, DATASIZE);
    block_number = parse_blocknumber(packet);
    data_size = parse_data(packet, packet_size, data);

    ck_assert_mem_eq(data, input, DATASIZE);
    ck_assert_int_eq(packet_size, BSIZE);
    ck_assert_int_eq(data_size, DATASIZE);
    ck_assert_int_eq(block_number, 1234);
} END_TEST

START_TEST(test_create_oversized_data_packet) {
    char packet[BSIZE];
    char data[DATASIZE + 1];
    size_t packet_size;

    packet_size = create_data_packet(packet, 1234, data, DATASIZE + 1);

    ck_assert_int_eq(packet_size, 0);
} END_TEST

START_TEST(test_create_error_packet) {
    char packet[BSIZE];
    char errmsg[MAX_ERRMSG];
    size_t packet_size;
    ecode_t ecode;
    int i;

    for (i = 0; i <= EOACK; i++) {
        packet_size = create_error_packet(packet, i);
        ecode = parse_errcode(packet);
        parse_errmsg(packet, packet_size, errmsg);

        ck_assert_int_eq(ecode, i);
        ck_assert_str_eq(errmsg, error_strings[i]);
    }
} END_TEST

START_TEST(test_parse_opcode) {
    opcode_t opcode;
    char packet[BSIZE];
    int i;

    for (i = 0; i <= EOACK; i++) {
        create_error_packet(packet, i++);
        opcode = parse_opcode(packet);
        ck_assert_int_eq(opcode, OP_ERROR);
    }
} END_TEST

START_TEST(test_parse_mode) {
    mode_t mode;
    char packet[BSIZE];

    create_initial_packet(packet, "filename", MODE_NETASCII, OP_WRQ);
    mode = parse_mode(packet);
    ck_assert_int_eq(mode, MODE_NETASCII);

    create_initial_packet(packet, "filename", MODE_OCTET, OP_WRQ);
    mode = parse_mode(packet);
    ck_assert_int_eq(mode, MODE_OCTET);
} END_TEST

START_TEST(test_parse_blocknumber) {
    bnum_t blocknumber;
    char packet[BSIZE];

    create_data_packet(packet, 4321, "test", 5);
    blocknumber = parse_blocknumber(packet);
    ck_assert_int_eq(blocknumber, 4321);

    create_ack_packet(packet, 1234);
    blocknumber = parse_blocknumber(packet);
    ck_assert_int_eq(blocknumber, 1234);
} END_TEST

START_TEST(test_parse_errcode) {
    ecode_t ecode;
    char packet[BSIZE];
    int i;

    for (i = 0; i <= EOACK; i++) {
        create_error_packet(packet, i);
        ecode = parse_errcode(packet);
        ck_assert_int_eq(ecode, i);
    }
} END_TEST

START_TEST(test_parse_filename) {
    char packet[BSIZE];
    size_t packet_size;
    char filename[MAX_FILENAME];

    packet_size = create_initial_packet(packet, "test", MODE_OCTET, OP_WRQ);

    parse_filename(packet, packet_size, filename);
    ck_assert_str_eq(filename, "test");
} END_TEST

START_TEST(test_parse_data) {
    char packet[BSIZE];
    char data[DATASIZE];
    char input[DATASIZE];
    size_t packet_size;
    size_t data_size;
    int i;

    for (i = 0; i < DATASIZE; i++) {
        input[i] = i;
    }

    packet_size = create_data_packet(packet, 1234, input, DATASIZE);
    data_size = parse_data(packet, packet_size, data);

    ck_assert_mem_eq(data, input, DATASIZE);
    ck_assert_int_eq(data_size, DATASIZE);
} END_TEST

START_TEST(test_parse_errmsg) {
    size_t packet_size;
    char packet[BSIZE];
    char errmsg[MAX_ERRMSG];
    int i;

    for (i = 0; i <= EOACK; i++) {
        packet_size = create_error_packet(packet, i);
        parse_errmsg(packet, packet_size, errmsg);
        ck_assert_str_eq(errmsg, error_strings[i]);
    }
} END_TEST

Suite* tftp_case(void) {
    Suite* suite;
    TCase* test_case;

    test_case = tcase_create("Core");
    tcase_add_test(test_case, test_create_initial_packet_1);
    tcase_add_test(test_case, test_create_initial_packet_2);
    tcase_add_test(test_case, test_create_ack_packet);
    tcase_add_test(test_case, test_create_data_packet);
    tcase_add_test(test_case, test_create_oversized_data_packet);
    tcase_add_test(test_case, test_create_error_packet);
    tcase_add_test(test_case, test_parse_opcode);
    tcase_add_test(test_case, test_parse_mode);
    tcase_add_test(test_case, test_parse_blocknumber);
    tcase_add_test(test_case, test_parse_errcode);
    tcase_add_test(test_case, test_parse_filename);
    tcase_add_test(test_case, test_parse_data);
    tcase_add_test(test_case, test_parse_errmsg);

    suite = suite_create("TFTP");
    suite_add_tcase(suite, test_case);

    return suite;
}

int main(void) {
    int number_failed;
    Suite* suite;
    SRunner* runner;

    suite = tftp_case();
    runner = srunner_create(suite);

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
