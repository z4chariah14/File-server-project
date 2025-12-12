cmake_minimum_required(VERSION 3.22)

project(file-server-project
    VERSION 1.0
    LANGUAGES C
)


set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

# 2. Find the Threading Library (Crucial for comm.os logic)
# It is required to make pthread_create works. - Learned this from AI
find_package(Threads REQUIRED)

# 3. Define the Server Executable
add_executable(file_server server/file_server.c)

# Link the threading library to the server.
target_link_libraries(file_server Threads::Threads)

# 4. Define the Client Executable
add_executable(file_client client/file_client.c)
