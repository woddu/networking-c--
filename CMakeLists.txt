cmake_minimum_required(VERSION 3.16)
project(MsgpackNetworking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("E:/msys64/ucrt64/include")

# Build server
add_executable(server server.cpp)

add_executable(multithreadserver multithreadserver.cpp)

# Build client
add_executable(client client.cpp)

# If you need Winsock on Windows:
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(multithreadserver PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
endif()