cmake_minimum_required(VERSION 3.16)
project(postprocessor)

set(CMAKE_CXX_STANDARD 17)

include_directories(. zlib)
find_package(ZLIB)

add_executable(postprocessor
        main.cpp
        zlib/strict_fstream.hpp
        zlib/zstr.hpp)

target_link_libraries(postprocessor ZLIB::ZLIB)
target_compile_definitions(postprocessor PRIVATE EMIT_UTF_8_JSON)
#target_compile_definitions(postprocessor PRIVATE EXPORT_TEXT_LENGTH) # add a "length" field to the emitted JSON