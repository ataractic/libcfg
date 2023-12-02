cmake_minimum_required(VERSION 3.10)
project(cfg)

set(SOURCES src/cfg.c)
set(C_STANDARD 17)

add_library(
    ${CMAKE_PROJECT_NAME}
    SHARED
    ${SOURCES}
)