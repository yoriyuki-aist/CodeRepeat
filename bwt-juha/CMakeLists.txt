cmake_minimum_required(VERSION 3.16)
project(bwt_juha)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(bwt
        bwt.cpp
        bwt.hpp
        dcsuffixsort.hpp
        stringsort.hpp
        partition.hpp
        difference_cover.cpp
        difference_cover.hpp
        dcsamplesort.hpp
        doubling.hpp
        distribute.hpp
        kmpsplit.hpp)
add_executable(ibwt
        ibwt.cpp
        ibwt.hpp)
add_executable(dnabwt
        dnabwt.cpp
        dnavector.hpp
        bwt.hpp
        dcsuffixsort.hpp
        stringsort.hpp
        partition.hpp
        difference_cover.cpp
        difference_cover.hpp
        dcsamplesort.hpp
        doubling.hpp
        distribute.hpp
        kmpsplit.hpp)
add_executable(bwtsa
        bwtsa.cpp
        suffixsort.hpp
        doubling.hpp
        partition.hpp
        stringsort.hpp
        checker.hpp
        difference_cover.cpp
        difference_cover.hpp
        timing.hpp)
