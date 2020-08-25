cmake_minimum_required(VERSION 3.16)
project(dna-bwt)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(bwt-juha)
add_subdirectory(findmaxrep)
add_subdirectory(findrepset)
add_subdirectory(converter)
add_subdirectory(preprocessor)
add_subdirectory(postprocessor)
