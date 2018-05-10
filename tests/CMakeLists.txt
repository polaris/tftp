cmake_minimum_required (VERSION 3.9)

add_executable(tests test-tftp.c)

target_link_libraries(tests check)
target_link_libraries(tests foo)
