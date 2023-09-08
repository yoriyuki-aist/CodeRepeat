cmake_minimum_required(VERSION 3.16)
project(preprocessor)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
include_directories(/Users/yoriyuki/include)
link_directories(/Users/yoriyuki/lib)

add_executable(findrepset_csa
        main.cpp)

target_include_directories(findrepset_csa PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    /Users/yoriyuki/include
)

target_link_libraries(findrepset_csa PRIVATE
    /Users/yoriyuki/lib/libsdsl.a
    /Users/yoriyuki/lib/libdivsufsort.a 
    /Users/yoriyuki/lib/libdivsufsort64.a 
      # Replace with the correct library file name
)