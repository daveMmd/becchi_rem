cmake_minimum_required(VERSION 2.8)

aux_source_directory(. SRCS)
list(REMOVE_ITEM SRCS "test_parse.cpp")

add_executable(test_parse test_parse.cpp ${SRCS})

add_library(parse ${SRCS})