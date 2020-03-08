cmake_minimum_required(VERSION 3.10)
project(thread_pool)

add_definitions(-Wall -Wextra -std=c++11 -rdynamic)
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")
set(CXX_FLAGS -g -Wall -Wextra -Wconversion -std=c++11 -rdynamic)
#-Werror
#-DVALGRIND
#-DCHECK_PTHREAD_RETURN_VALUE
#-D_FILE_OFFSET_BITS=64
#-Wconversion
#-Wno-unused-parameter
#-Wold-style-cast
#-Woverloaded-virtual
#-Wpointer-arith
#-Wshadow
#-Wwrite-strings
#-march=native
#-MMD
#)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	list(APPEND CXX_FLAGS "-Wno-null-dereference")
	list(APPEND CXX_FLAGS "-Wno-sign-conversion")
	list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
	list(APPEND CXX_FLAGS "-Wthread-safety")
	list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()

aux_source_directory(src SRC)
add_library(thread_pool SHARED ${SRC})
target_link_libraries(thread_pool pthread)
target_include_directories(thread_pool PUBLIC include)

add_executable(test01 ./example/test01.cpp)
target_link_libraries(test01 thread_pool)
add_executable(test02 ./example/test02.cpp)
target_link_libraries(test02 thread_pool)
add_executable(test03 ./example/test03.cpp)
target_link_libraries(test03 thread_pool)
add_executable(test04 ./example/test04.cpp)
target_link_libraries(test04 thread_pool)
add_executable(test05 ./example/test05.cpp)
target_link_libraries(test05 thread_pool)
add_executable(test06 ./example/test06.cpp)
target_link_libraries(test06 thread_pool)
target_include_directories(test06 PUBLIC include)
add_executable(test07 ./example/test07.cpp)
target_link_libraries(test07 thread_pool)
target_include_directories(test07 PUBLIC include)
add_executable(test08 ./example/test08.cpp)
target_link_libraries(test08 thread_pool)
target_include_directories(test08 PUBLIC include)
add_executable(test09 ./example/test09.cpp)
target_link_libraries(test09 thread_pool)
target_include_directories(test09 PUBLIC include)
add_executable(test10 ./example/test10.cpp)
target_link_libraries(test10 thread_pool)
target_include_directories(test10 PUBLIC include)
add_executable(test11 ./example/test11.cpp)
target_link_libraries(test11 thread_pool)
target_include_directories(test11 PUBLIC include)

add_executable(rwlock_test ./example/rwlock_test.cpp)
target_link_libraries(rwlock_test thread_pool)
target_include_directories(rwlock_test PUBLIC include)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
