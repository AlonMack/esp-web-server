#include <user_interface.h>
#include <osapi.h>

LOCAL os_timer_t test_timer;

extern int ets_uart_printf(const char *fmt, ...);

void ICACHE_FLASH_ATTR checkCount(void) {
    uint8 i = wifi_softap_get_station_num();
    if (i == 0) {
        ets_uart_printf("0\r\n");
    } else if (i == 1) {
        ets_uart_printf("1\r\n");
    } else if (i == 2) {
        ets_uart_printf("2\r\n");
    }
}

void ICACHE_FLASH_ATTR user_set_softap_config(void) {
    wifi_set_opmode(SOFTAP_MODE);

    struct softap_config config;

    wifi_softap_get_config(&config);

    os_memset(config.ssid, 0, 32);
    os_memset(config.password, 0, 64);
    os_memcpy(config.ssid, WIFI_CONFIGURATION_SSID, sizeof(config.ssid));
    os_memcpy(config.password, WIFI_CONFIGURATION_PASSWORD, sizeof(config.password));
    config.authmode = AUTH_WPA_WPA2_PSK;
    config.ssid_len = 0;
    config.max_connection = 4;

    wifi_softap_set_config(&config);

    os_timer_disarm(&test_timer);
    os_timer_setfn(&test_timer, (os_timer_func_t *) checkCount, NULL);
    os_timer_arm(&test_timer, 1000, 1);
}