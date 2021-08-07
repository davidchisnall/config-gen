cmake_minimum_required(VERSION 3.12)
project(config-gen)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 20)

find_library(UCL_LIBRARY ucl REQUIRED)
find_path(UCL_INCLUDE_DIR ucl.h REQUIRED)

add_executable(config-gen config-gen.cc)
target_include_directories(config-gen PRIVATE ${UCL_INCLUDE_DIR})
target_link_libraries(config-gen PRIVATE ${UCL_LIBRARY})


enable_testing()
add_subdirectory(tests)
