cmake_minimum_required(VERSION 3.16)
project(v_3_6_2)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(v_3_6_2
        arg.cpp
        arg.h
        bt.cpp
        bt.h
        bv.cpp
        bv.h
        bwt_2_lcp.cpp
        bwt_2_lcp.h
        dna_convert.cpp
        huffcode.cpp
        huffcode.h
        main.cpp
        main.h
        main.randtest.cpp
        maxrep.cpp
        maxrep.h
        report.cpp
        report.h
        test_bt.cpp
        test_bv.cpp
        test_bwt_2_lcp.cpp
        test_huffcode.cpp
        test_huffman.cpp
        test_maxrep.cpp
        test_report.cpp
        test_wt.cpp
        uti.cpp
        uti.h
        wt.h
        wt_node.h)
