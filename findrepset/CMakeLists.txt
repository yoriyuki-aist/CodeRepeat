cmake_minimum_required(VERSION 3.16)
project(findrepset C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(findrepset
        bitarray.h
        bittree.c
        bittree.h
        bwt.c
        bwt.h
        common.c
        common.h
        config.h
        cop.c
        cop.h
        enc.c
        enc.h
        filecop.c
        lcp.c
        lcp.h
        macros.h
        mmrs.c
        mmrs.h
        mrs.c
        mrs.h
        output_callbacks.c
        output_callbacks.h
        sorters.h
        tiempos.c
        tiempos.h
        tipos.h)
