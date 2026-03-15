cmake_minimum_required(VERSION 3.5)

project(tpnet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tpnet main.cpp)

target_sources(tpnet
    PRIVATE
        tcpserver.cpp
        tcpclient.cpp
        tileimage.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            tcpserver.h
            tcpclient.h
            tileimage.h
            common.h
)

