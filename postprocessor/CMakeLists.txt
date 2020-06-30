cmake_minimum_required(VERSION 3.16)
project(postprocessor)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(postprocessor
        main.cpp
		stringescape.cpp)

#target_compile_definitions(postprocessor PUBLIC EMIT_UTF_8_JSON)