cmake_minimum_required(VERSION 3.16)
project(findmaxrep)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(findmaxrep
        main.cpp
        main.h
        arg.h
        arg.cpp
        bt.h
        bt.cpp
        bv.h
        bv.cpp
        bwt_2_lcp.h
        bwt_2_lcp.cpp
        huffcode.h
        huffcode.cpp
        maxrep.h
        maxrep.cpp
        report.h
        report.cpp
        uti.h
        uti.cpp
        wt.h
        wt_node.h)
