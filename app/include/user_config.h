#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

    #define WIFI_CONFIGURATION_SSID "ESP8266"
    #define WIFI_CONFIGURATION_PASSWORD "987654321"

    #define MAX_CONN 8

    #define MAX_SENDBUFF_LEN 2048

    #define DEBUG_LOGGING
    #ifdef DEBUG_LOGGING
        #undef DEBUG_LOGGING
        #define DEBUG_LOGGING(...) ets_uart_printf(__VA_ARGS__);
        #else
        #define DEBUG_LOGGING(...)
    #endif
#endif
