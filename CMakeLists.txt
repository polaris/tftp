cmake_minimum_required (VERSION 3.9)

project(tftp VERSION 0.0.1 LANGUAGES C)

set(DEFAULT_OPTIONS
    -glldb
    -O0
    -fno-omit-frame-pointer
    -fno-common
    -fstrict-aliasing
    -Weverything
    -Wno-padded
    -pedantic
)

set(DEFAULT_SOURCES
    tftp.c
    print.c
    util.c
    fsm.c
)

add_library(foo STATIC
    ${DEFAULT_SOURCES}
)

target_compile_options(foo PRIVATE
    ${DEFAULT_OPTIONS}
)


set(CLIENT_SOURCES
    client.c
)

add_executable(tftp ${CLIENT_SOURCES})

target_compile_options(tftp PRIVATE
    ${DEFAULT_OPTIONS}
)

target_link_libraries(tftp PRIVATE foo)


set(SERVER_SOURCES
    server.c
)

add_executable(tftpd ${SERVER_SOURCES})

target_compile_options(tftpd PRIVATE
    ${DEFAULT_OPTIONS}
)

target_link_libraries(tftpd PRIVATE foo)

# if(testing_enabled)
    add_subdirectory(tests)
# endif()
