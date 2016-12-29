#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include "util.h"
#include "ap_connect.h"

static struct espconn esp_conn;
static esp_tcp esptcp;
static unsigned char killConn;
static const char *pageIndex = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n    <meta charset=\"UTF-8\">\n    <title>Title</title>\n</head>\n<body>\nMy page\n</body>\n</html>";
//Private data for http connection
struct HttpdPriv {
    char *sendBuff;
    int sendBuffLen;
};

//Connection pool
static HttpdPriv connPrivData[MAX_CONN];
static HttpdConnData connData[MAX_CONN];

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
                DEBUG_LOGGING("CONFIGURATION WEB SERVER IP: "
                                IPSTR
                                "\r\n", IP2STR(&ipinfo.ip));
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
        DEBUG_LOGGING("AP config: SSID: %s, PASSWORD: %s, CHANNEL: %u\r\n", apconfig.ssid, apconfig.password,
                apconfig.channel);
    }
}

int ICACHE_FLASH_ATTR config_httpdSend(HttpdConnData *conn, const char *data, int len) {
    if (len < 0)
        len = strlen(data);
    if (conn->priv->sendBuffLen + len > MAX_SENDBUFF_LEN)
        return 0;
    os_memcpy(conn->priv->sendBuff + conn->priv->sendBuffLen, data, len);
    conn->priv->sendBuffLen += len;
    return 1;
}

//Start the response headers.
void ICACHE_FLASH_ATTR config_httpdStartResponse(HttpdConnData *conn, int code) {
    char buff[128];
    int l;
    l = os_sprintf(buff, "HTTP/1.0 %d OK\r\nServer: ESPOOLITE-Config-Server/0.1\r\n", code);
    config_httpdSend(conn, buff, l);
}

//Send a http header.
void ICACHE_FLASH_ATTR config_httpdHeader(HttpdConnData *conn, const char *field, const char *val) {
    char buff[256];
    int l;
    l = os_sprintf(buff, "%s: %s\r\n", field, val);
    config_httpdSend(conn, buff, l);
}

//Finish the headers.
void ICACHE_FLASH_ATTR config_httpdEndHeaders(HttpdConnData *conn) {
    config_httpdSend(conn, "\r\n", -1);
}

static ICACHE_FLASH_ATTR HttpdConnData ICACHE_FLASH_ATTR *config_httpdFindConnData(void *arg) {
    int i;
    for (i = 0; i < MAX_CONN; i++) {
        if (connData[i].conn == (struct espconn *) arg)
            return &connData[i];
    }
    DEBUG_LOGGING("FindConnData: Couldn't find connection for %p\n", arg);
    return NULL; //WtF?
}

static ICACHE_FLASH_ATTR void ICACHE_FLASH_ATTR config_xmitSendBuff(HttpdConnData *conn) {
    if (conn->priv->sendBuffLen != 0) {
        DEBUG_LOGGING("xmitSendBuff\r\n");
        espconn_sent(conn->conn, (uint8_t *) conn->priv->sendBuff, conn->priv->sendBuffLen);
        conn->priv->sendBuffLen = 0;
    }
}

static unsigned char ICACHE_FLASH_ATTR
noolite_config_server_get_key_val(char *key, unsigned char maxlen, char *str, char *retval) {
    unsigned char found = 0;
    char *keyptr = key;
    char prev_char = '\0';
    *retval = '\0';

    while (*str && *str != '\r' && *str != '\n' && !found) {
        if (*str == *keyptr) {
            if (keyptr == key && !(prev_char == '?' || prev_char == '&')) {
                str++;
                continue;
            }
            keyptr++;
            if (*keyptr == '\0') {
                str++;
                keyptr = key;
                if (*str == '=') {
                    found = 1;
                }
            }
        } else {
            keyptr = key;
        }
        prev_char = *str;
        str++;
    }
    if (found == 1) {
        found = 0;
        while (*str && *str != '\r' && *str != '\n' && *str != ' ' && *str != '&' && maxlen > 0) {
            *retval = *str;
            maxlen--;
            str++;
            retval++;
            found++;
        }
        *retval = '\0';
    }
    return found;
}

static void ICACHE_FLASH_ATTR
noolite_config_server_process_page(struct HttpdConnData *conn, char *page, char *request) {
    char ssid[32];
    char pass[64];
    char buff[1024];
    char html_buff[1024];
    char version_buff[10];
    int len;
    static struct ip_info ipConfig;
    char save[2] = {'0', '\0'};
    char status[32] = "[status]";
    struct station_config stationConf;

    os_memset(status, 0, sizeof(status));
    os_memset(ssid, 0, sizeof(ssid));
    os_memset(pass, 0, sizeof(pass));

    config_httpdStartResponse(conn, 200);
    config_httpdHeader(conn, "Content-Type", "text/html");
    config_httpdEndHeaders(conn);

    // page header
    len = os_sprintf(buff, pageIndex);
    if (!config_httpdSend(conn, buff, len)) {
        DEBUG_LOGGING("Error httpdSend: pageStart out-of-memory\r\n");
    }
    killConn = 1;
}

static void ICACHE_FLASH_ATTR noolite_config_server_recv(void *arg, char *data, unsigned short len) {
    DEBUG_LOGGING("noolite_config_server_recv\r\n");

    char sendBuff[MAX_SENDBUFF_LEN];
    HttpdConnData *conn = config_httpdFindConnData(arg);

    if (conn == NULL)
        return;
    conn->priv->sendBuff = sendBuff;
    conn->priv->sendBuffLen = 0;

    char page[16];
    os_memset(page, 0, sizeof(page));

    noolite_config_server_get_key_val("page", sizeof(page), data, page);
    noolite_config_server_process_page(conn, page, data);
    config_xmitSendBuff(conn);
    return;
}

static void ICACHE_FLASH_ATTR noolite_config_server_sent(void *arg) {
    HttpdConnData *conn = config_httpdFindConnData(arg);

    if (conn == NULL) return;
    if (killConn) {
        espconn_disconnect(conn->conn);
    }
}

static void ICACHE_FLASH_ATTR noolite_config_server_connect(void *arg) {
    DEBUG_LOGGING("noolite_config_server_connect\r\n");

    struct espconn *conn = arg;
    int i;
    //Find empty conndata in pool
    for (i = 0; i < MAX_CONN; i++)
        if (connData[i].conn == NULL) break;
    DEBUG_LOGGING("Con req, conn=%p, pool slot %d\n", conn, i);
    if (i == MAX_CONN) {
        DEBUG_LOGGING("Conn pool overflow!\r\n");
        espconn_disconnect(conn);
        return;
    }
    connData[i].conn = conn;
    connData[i].postLen = 0;
    connData[i].priv = &connPrivData[i];

    espconn_regist_recvcb(conn, noolite_config_server_recv);
    espconn_regist_sentcb(conn, noolite_config_server_sent);
}

void ICACHE_FLASH_ATTR config_server_init() {
    int i;
    DEBUG_LOGGING("config_server_init()\r\n");

    for (i = 0; i < MAX_CONN; i++) {
        connData[i].conn = NULL;
    }

    esptcp.local_port = 80;
    esp_conn.type = ESPCONN_TCP;
    esp_conn.state = ESPCONN_NONE;
    esp_conn.proto.tcp = &esptcp;
    espconn_regist_connectcb(&esp_conn, noolite_config_server_connect);
    espconn_accept(&esp_conn);
}