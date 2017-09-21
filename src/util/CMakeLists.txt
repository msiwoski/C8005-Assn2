project(util)

set(SOURCES vector.c ring_buffer.c log.c)
add_library(util ${SOURCES})
target_compile_options(util PRIVATE -std=c11)
target_include_directories(util PRIVATE ${CMAKE_SOURCE_DIR}/include/assn2/util)
target_link_libraries(util -lpthread -lrt)