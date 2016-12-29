cmake_minimum_required(VERSION 3.6)
project(esp_web_server)

set(CMAKE_CXX_STANDARD 11)
include_directories(C:/Espressif/ESP8266_SDK/include)
include_directories(C:/Espressif/ESP8266_SDK/include/json)
include_directories(app/include)

set(SOURCE_FILES
        app/user/user_main.c
        app/include/user_config.h
        app/include/driver/uart.h
        app/include/driver/uart_register.h
        app/driver/uart.c
        app/user/util.h
        app/user/ap_config.h
        app/user/ap_config.c
        app/user/httpd.h
        app/user/httpd.c

        pages/pages.h
        )

add_executable(esp_web_server ${SOURCE_FILES})