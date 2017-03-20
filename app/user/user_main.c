#include <user_interface.h>
#include <driver/uart.h>
#include <gpio.h>
#include "ap_config.h"
#include "httpd.h"

extern void ets_wdt_enable(void);

extern void ets_wdt_disable(void);

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

    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
    GPIO_OUTPUT_SET(12, 1);
    GPIO_OUTPUT_SET(13, 1);
    GPIO_OUTPUT_SET(14, 0);

    GPIO_OUTPUT_SET(4, 1);
    GPIO_OUTPUT_SET(0, 1);
    GPIO_OUTPUT_SET(2, 0);

    setup_wifi_ap_mode();
    config_server_init();
}