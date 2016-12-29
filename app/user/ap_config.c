#include <user_interface.h>
#include <osapi.h>
#include "ap_config.h"

void ICACHE_FLASH_ATTR setup_wifi_ap_mode(void) {
    wifi_set_opmode(SOFTAP_MODE);
    struct softap_config apconfig;
    if (wifi_softap_get_config(&apconfig)) {
        wifi_softap_dhcps_stop();
        char macaddr[6];
        wifi_get_macaddr(SOFTAP_IF, macaddr);
        os_memset(apconfig.ssid, 0, sizeof(apconfig.ssid));
        os_memset(apconfig.password, 0, sizeof(apconfig.password));
        apconfig.ssid_len = os_sprintf(apconfig.ssid, WIFI_CONFIGURATION_SSID, MAC2STR(macaddr));
        os_sprintf(apconfig.password, WIFI_CONFIGURATION_PASSWORD, MAC2STR(macaddr));
        apconfig.authmode = AUTH_WPA_WPA2_PSK;
        apconfig.ssid_hidden = 0;
        apconfig.channel = 7;
        apconfig.max_connection = 10;
        if (!wifi_softap_set_config(&apconfig)) {
            DEBUG_LOGGING("NOOLITE not set AP config!\r\n");
        }
        struct ip_info ipinfo;
        if (wifi_get_ip_info(SOFTAP_IF, &ipinfo)) {
            IP4_ADDR(&ipinfo.ip, 192, 168, 4, 1);
            IP4_ADDR(&ipinfo.gw, 192, 168, 4, 1);
            IP4_ADDR(&ipinfo.netmask, 255, 255, 255, 0);
            if (!wifi_set_ip_info(SOFTAP_IF, &ipinfo)) {
                DEBUG_LOGGING("NOOLITE not set IP config!\r\n");
            } else {
                DEBUG_LOGGING("WEB SERVER IP: " IPSTR "\r\n", IP2STR(&ipinfo.ip));
            }
        }
        wifi_softap_dhcps_start();
    }
    if (wifi_get_phy_mode() != PHY_MODE_11N)
        wifi_set_phy_mode(PHY_MODE_11N);
    if (wifi_station_get_auto_connect() == 0)
        wifi_station_set_auto_connect(1);
    DEBUG_LOGGING("NOOLITE in AP mode configured.\r\n");
    if (wifi_softap_get_config(&apconfig)) {
        DEBUG_LOGGING("AP config: SSID: %s, PASSWORD: %s, CHANNEL: %u\r\n", apconfig.ssid, apconfig.password, apconfig.channel);
    }
}