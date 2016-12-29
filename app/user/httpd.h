#ifndef ESP_WEB_SERVER_HTTPD_H
#define ESP_WEB_SERVER_HTTPD_H

#include "util.h"

void config_server_init(void);

int config_httpdSend(HttpdConnData *conn, const char *data, int len);

void config_httpdStartResponse(HttpdConnData *conn, int code);

void config_httpdHeader(HttpdConnData *conn, const char *field, const char *val);

void config_httpdEndHeaders(HttpdConnData *conn);

#endif //ESP_WEB_SERVER_HTTPD_H
