cmake_minimum_required(VERSION 3.16)
project(MsgpackNetworking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(msgpack-cxx_DIR "C:/msys64/mingw64/lib/cmake/msgpack-cxx")

# Find MessagePack (C++ bindings)
find_package(msgpack-cxx REQUIRED)

# Build server
add_executable(server server.cpp)

# Build client
add_executable(client client.cpp)

# Link against msgpack-cxx
target_link_libraries(server PRIVATE msgpackcxx)
target_link_libraries(client PRIVATE msgpackcxx)

# If you need Winsock on Windows:
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
    target_link_libraries(client PRIVATE ws2_32)
endif()