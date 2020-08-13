cmake_minimum_required(VERSION 3.16)
project(postprocessor)

set(CMAKE_CXX_STANDARD 17)

include_directories(. zlib)

AUX_SOURCE_DIRECTORY(zlib ZLIB_SOURCES) # note: cmake needs to be re-run if files are added

add_executable(postprocessor
        main.cpp
        ${ZLIB_SOURCES})

target_compile_definitions(postprocessor PRIVATE EMIT_UTF_8_JSON)
#target_compile_definitions(postprocessor PRIVATE EXPORT_TEXT_LENGTH) # add a "length" field to the emitted JSON