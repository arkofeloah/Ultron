cmake_minimum_required(VERSION 3.22.1)

project("ultron")

add_library(
        ultron
        SHARED
        native-lib.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        ultron
        ${log-lib})