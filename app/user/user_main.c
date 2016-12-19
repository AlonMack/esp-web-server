#include <user_interface.h>
#include <osapi.h>
#include <driver/uart.h>

LOCAL os_timer_t test_timer;

extern void ets_wdt_enable(void);

extern void ets_wdt_disable(void);

extern int ets_uart_printf(const char *fmt, ...);

void wifi_handle_event_cb(System_Event_t *evt) {
    ets_uart_printf("event %x\n", evt->event);
    switch (evt->event) {
        case EVENT_STAMODE_CONNECTED:
            ets_uart_printf("connect to ssid %s, channel %d\n",
                            evt->event_info.connected.ssid,
                            evt->event_info.connected.channel);
            break;
        case EVENT_STAMODE_DISCONNECTED:
            ets_uart_printf("disconnect from ssid %s, reason %d\n",
                            evt->event_info.disconnected.ssid,
                            evt->event_info.disconnected.reason);
            break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:
            ets_uart_printf("mode: %d -> %d\n",
                            evt->event_info.auth_change.old_mode,
                            evt->event_info.auth_change.new_mode);
            break;
        case EVENT_STAMODE_GOT_IP:
            ets_uart_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
                            IP2STR(&evt->event_info.got_ip.ip),
                            IP2STR(&evt->event_info.got_ip.mask),
                            IP2STR(&evt->event_info.got_ip.gw));
            os_printf("\n");
            break;
        case EVENT_SOFTAPMODE_STACONNECTED:
            ets_uart_printf("station: " MACSTR "join, AID = %d\n",
                            MAC2STR(evt->event_info.sta_connected.mac),
                            evt->event_info.sta_connected.aid);
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            ets_uart_printf("station: " MACSTR "leave, AID = %d\n",
                            MAC2STR(evt->event_info.sta_disconnected.mac),
                            evt->event_info.sta_disconnected.aid);
            break;
        default:
            break;
    }
}

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
    struct softap_config config;

    wifi_softap_get_config(&config);

    os_memset(config.ssid, 0, 32);
    os_memset(config.password, 0, 64);
    os_memcpy(config.ssid, "ESP8266_new", 11);
    os_memcpy(config.password, "12345678", 8);
    config.authmode = AUTH_WPA_WPA2_PSK;
    config.ssid_len = 0;
    config.max_connection = 4;

    wifi_softap_set_config(&config);

}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void) {
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void user_init(void) {
    uart_init(BIT_RATE_9600, BIT_RATE_9600);
    ets_wdt_enable();
    ets_wdt_disable();
    ets_uart_printf("SDK version:%s\n", system_get_sdk_version());

    wifi_set_opmode(SOFTAP_MODE);

    user_set_softap_config();

    os_timer_disarm(&test_timer);
    os_timer_setfn(&test_timer, (os_timer_func_t *) checkCount, NULL);
    os_timer_arm(&test_timer, 1000, 1);
    wifi_set_event_handler_cb(wifi_handle_event_cb);
}