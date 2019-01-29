cmake_minimum_required(VERSION 3.12)
project(osureplayparser C)

set(CMAKE_C_STANDARD 99)

include_directories(include)
include_directories(lzma/src/easylzma)

add_library(
	osureplayparser
	include/osu_replay_parser.h
	src/osu_replay_parser.c
	src/uncompress_lzma.c
	lzma/src/
	lzma/src/easylzma
	lzma/src/pavlov
	include/decompressor.h)