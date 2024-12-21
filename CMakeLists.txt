cmake_minimum_required(VERSION 3.28)
project(server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/slog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(slog STATIC
        lib/slog/slog.c
        lib/slog/slog.h
)

add_executable(server
        src/main.c
        src/http_handler.c
        src/server.c
        src/tpool.c
)

target_link_libraries(server PRIVATE slog)
