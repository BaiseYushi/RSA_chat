cmake_minimum_required(VERSION 3.22.1)
project(rsa_chat_android)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
        rsa_chat_core
        SHARED
        rsa_chat_core.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(
        rsa_chat_core
        ${log-lib}
        ${android-lib}
)