cmake_minimum_required(VERSION 3.16)
project(coderepeat)

set(HOME /Users/yoriyuki)

# Set the compiler to g++-11
#set(CMAKE_CXX_COMPILER g++-11)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(preprocessor)
add_subdirectory(findrepset)
add_subdirectory(findrepset_csa)
add_subdirectory(postprocessor)