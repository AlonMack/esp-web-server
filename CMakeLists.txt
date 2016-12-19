cmake_minimum_required(VERSION 3.6)
project(esp_web_server)

set(CMAKE_CXX_STANDARD 11)
include_directories(C:/Espressif/ESP8266_SDK/include)
include_directories(C:/Espressif/ESP8266_SDK/include/json)
include_directories(app/include)

set(SOURCE_FILES
    app/include/driver/uart.h
    app/driver/uart.c
    app/include/driver/uart_register.h
    app/include/user_config.h
        app/user/user_main.c
        )

add_executable(esp_web_server ${SOURCE_FILES})