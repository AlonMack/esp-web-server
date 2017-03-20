#ifndef __UTILS_H_
#define __UTILS_H_

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;

struct HttpdPriv {
    char *sendBuff;
    int sendBuffLen;
};

struct HttpdConnData {
    struct espconn *conn;
    HttpdPriv *priv;
    int postLen;
};

#endif /* __UTILS_H_ */