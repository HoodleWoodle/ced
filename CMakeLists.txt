cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER zig cc)
set(CMAKE_CPP_COMPILER zig c++)

project(ced)

add_subdirectory(lib/zwerg/)

file(GLOB_RECURSE CFILES ${CMAKE_SOURCE_DIR}/*.c)
	SET_SOURCE_FILES_PROPERTIES(${CFILES} PROPERTIES LANGUAGE CXX)

include_directories(src/)
add_executable(${PROJECT_NAME}
	src/common.h
	src/doc.h
 	src/gui.c
	src/main.c
)
target_link_libraries(${PROJECT_NAME} zwerg)
