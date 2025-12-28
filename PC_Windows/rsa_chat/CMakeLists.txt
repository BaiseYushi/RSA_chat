cmake_minimum_required(VERSION 3.24)
project(rsa_chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core)

add_executable(rsa_chat
        main.cpp
        MainWindow.h MainWindow.cpp
        SetupPage.h SetupPage.cpp
        ChatPage.h ChatPage.cpp
        rsa_chat_core.h rsa_chat_core.cpp
        app_icon.rc
)

target_link_libraries(rsa_chat
        PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Core
)

add_executable(rsa_chat_echo_server
        echo_server.cpp
)

target_link_libraries(rsa_chat_echo_server
        PRIVATE
        Qt6::Core
        Qt6::Network
)


if (WIN32)
    add_custom_command(TARGET rsa_chat POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:rsa_chat>
    )

    add_custom_command(TARGET rsa_chat_echo_server POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:rsa_chat_echo_server>
    )
endif()