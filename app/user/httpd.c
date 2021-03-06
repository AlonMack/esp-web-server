#include <user_config.h>
#include <user_interface.h>
#include <osapi.h>
#include <mem.h>
#include <gpio.h>
#include "espconn.h"
#include "util.h"
#include "httpd.h"
#include "../../pages/pages.h"

static struct espconn esp_conn;
static esp_tcp esptcp;
static unsigned char killConn;

static HttpdPriv connPrivData[MAX_CONN];
static HttpdConnData connData[MAX_CONN];

int ICACHE_FLASH_ATTR config_httpdSend(HttpdConnData *conn, const char *data, int len) {
    if (len < 0)
        len = strlen(data);
    if (conn->priv->sendBuffLen + len > MAX_SENDBUFF_LEN)
        return 0;
    os_memcpy(conn->priv->sendBuff + conn->priv->sendBuffLen, data, len);
    conn->priv->sendBuffLen += len;
    return 1;
}

void ICACHE_FLASH_ATTR config_httpdStartResponse(HttpdConnData *conn, int code) {
    char buff[128];
    config_httpdSend(conn, buff, os_sprintf(buff, "HTTP/1.0 %d OK\r\nServer: ESPOOLITE-Config-Server/0.1\r\n", code));
}

void ICACHE_FLASH_ATTR config_httpdHeader(HttpdConnData *conn, const char *field, const char *val) {
    char buff[256];
    config_httpdSend(conn, buff, os_sprintf(buff, "%s: %s\r\n", field, val));
}

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

char ICACHE_FLASH_ATTR *str_replace(const char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = os_strlen(rep);
    if (!with)
        with = "";
    len_with = os_strlen(with);

    ins = (char *) orig;
    for (count = 0; tmp = (char *) os_strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = (char *) os_malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = (char *) os_strstr(orig, rep);
        len_front = ins - orig;
        tmp = (char *) os_strncpy(tmp, orig, len_front) + len_front;
        tmp = (char *) os_strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    os_strcpy(tmp, orig);
    return result;
}

static void ICACHE_FLASH_ATTR config_server_recv(void *arg, char *data, unsigned short len) {
    DEBUG_LOGGING("config_server_recv\r\n");
    char sendBuff[MAX_SENDBUFF_LEN];
    HttpdConnData *conn = config_httpdFindConnData(arg);

    if (conn == NULL)
        return;
    conn->priv->sendBuff = sendBuff;
    conn->priv->sendBuffLen = 0;

    char buff[1024];

    config_httpdStartResponse(conn, 200);
    config_httpdHeader(conn, "Content-Type", "text/html");
    config_httpdEndHeaders(conn);
    DEBUG_LOGGING(data, "\r\n");
    if (os_strncmp(data, "GET /mypage", 11) == 0) {
        GPIO_OUTPUT_SET(12, 0);
        GPIO_OUTPUT_SET(4, 0);
        GPIO_OUTPUT_SET(13, 1);
        GPIO_OUTPUT_SET(0, 1);
        os_delay_us(50000);
        if (!config_httpdSend(conn, buff, os_sprintf(buff, "%s", str_replace(page_html, "${1gpio_state}", "1")))) {
            DEBUG_LOGGING("Error httpdSend: pageStart out-of-memory\r\n");
        }
    } else if(os_strncmp(data, "GET / ", 6) == 0) {
        GPIO_OUTPUT_SET(12, 1);
        GPIO_OUTPUT_SET(4, 1);
        GPIO_OUTPUT_SET(13, 0);
        GPIO_OUTPUT_SET(0, 0);
        os_delay_us(50000);
        if (!config_httpdSend(conn, buff, os_sprintf(buff, "%s", str_replace(index_html, "${1gpio_state}", "0")))) {
            DEBUG_LOGGING("Error httpdSend: pageStart out-of-memory\r\n");
        }
    }
    killConn = 1;

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

static void ICACHE_FLASH_ATTR config_httpdRetireConn(HttpdConnData *conn) {
    conn->conn = NULL;
}

static void ICACHE_FLASH_ATTR noolite_config_server_discon(void *arg) {
    DEBUG_LOGGING("noolite_config_server_discon\r\n");
    int i;
    for (i = 0; i < MAX_CONN; i++) {
        if (connData[i].conn != NULL) {
            if (connData[i].conn->state == ESPCONN_NONE || connData[i].conn->state >= ESPCONN_CLOSE) {
                connData[i].conn = NULL;
                config_httpdRetireConn(&connData[i]);
            }
        }
    }
}

static void ICACHE_FLASH_ATTR noolite_config_server_connect(void *arg) {
    DEBUG_LOGGING("noolite_config_server_connect\r\n");

    struct espconn *conn = arg;
    int i;
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

    espconn_regist_recvcb(conn, config_server_recv);
    espconn_regist_sentcb(conn, noolite_config_server_sent);
    espconn_regist_disconcb(conn, noolite_config_server_discon);
}

void ICACHE_FLASH_ATTR config_server_init() {
    DEBUG_LOGGING("config_server_init()\r\n");

    int i;
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