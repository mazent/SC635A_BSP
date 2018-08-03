#ifndef AP_H_
#define AP_H_

#include "bsp.h"

typedef struct {
    char ssid[32];
    char password[64];
    bool ssid_hidden;
    uint8_t max_connection;
    enum {
        AUTH_OPEN = 0,
        AUTH_WEP,
        AUTH_WPA_WPA2_PSK
    } auth ;
} S_AP ;

bool AP_beg(const S_AP *) ;
void AP_end(void) ;

typedef enum {
    AP_EVN_START,
    AP_EVN_STOP,
    AP_EVN_STACONNECTED,
    AP_EVN_STADISCONNECTED,
    AP_EVN_STAIPASSIGNED
} AP_EVN ;

void AP_evn(AP_EVN, void *) ;

#endif
