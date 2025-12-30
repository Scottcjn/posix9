# POSIX9 - POSIX Compatibility Layer for Mac OS 9
# CMakeLists.txt for Retro68 cross-compilation

cmake_minimum_required(VERSION 3.12)
project(posix9 C)

# Library sources
set(POSIX9_SOURCES
    src/posix9_file.c
    src/posix9_dir.c
    src/posix9_path.c
    src/posix9_socket.c
    src/posix9_thread.c
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Static library
add_library(posix9 STATIC ${POSIX9_SOURCES})

# Install headers
install(FILES include/posix9.h DESTINATION include)
install(DIRECTORY include/posix9 DESTINATION include)

# Install library
install(TARGETS posix9
    ARCHIVE DESTINATION lib
)

# Example application (if building standalone test)
option(BUILD_EXAMPLES "Build example applications" OFF)

if(BUILD_EXAMPLES)
    add_executable(posix9_test examples/test_file.c)
    target_link_libraries(posix9_test posix9)
endif()
