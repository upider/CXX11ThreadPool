cmake_minimum_required(VERSION 3.10)
project(thread_pool)

set(CMAKE_CXX_COMPILER "g++")
add_compile_options(-std=c++11 -w -g)

aux_source_directory(src SRC)
add_library(thread_pool SHARED ${SRC})
target_link_libraries(thread_pool pthread)
target_include_directories(thread_pool PUBLIC include)

add_executable(test01 ./example/test01.cpp)
target_link_libraries(test01 pthread thread_pool)
add_executable(test02 ./example/test02.cpp)
target_link_libraries(test02 pthread thread_pool)
add_executable(test03 ./example/test03.cpp)
target_link_libraries(test03 pthread thread_pool)
add_executable(test04 ./example/test04.cpp)
target_link_libraries(test04 pthread thread_pool)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
