cmake_minimum_required(VERSION 3.22)

project(file-server-project
    VERSION 1.0
    LANGUAGES C
)

add_compile_definitions(_GNU_SOURCE)

set(CMAKE_C_STANDARD 23)

<<<<<<< HEAD

# Turn on warnings that help flag the most common potential issues in code.
set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wconversion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdouble-promotion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wduplicated-branches")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wduplicated-cond")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wlogical-op")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmisleading-indentation")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wnull-dereference")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-conversion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunreachable-code")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused")
# Enable debugging through GDB.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

# Some setups don't detect clang-tidy automatically, so make sure it gets picked
# up here.
set(CMAKE_C_CLANG_TIDY "clang-tidy")

# Find the Threading Library (Crucial for comm.os logic)
# It is required to make pthread_create works.
=======
# 2. Find the Threading Library (Crucial for comm.os logic)
# It is required to make pthread_create works - Learned this from AI
>>>>>>> 70f7e7ebb6bd6caab02ba73e91db059b5deb8fe8
find_package(Threads REQUIRED)

# Define the Server Executable
add_executable(file_server server/file_server.c)

# Link the threading library to the server.
target_link_libraries(file_server Threads::Threads)

# 4. Define the Client Executable
add_executable(file_client client/file_client.c)
