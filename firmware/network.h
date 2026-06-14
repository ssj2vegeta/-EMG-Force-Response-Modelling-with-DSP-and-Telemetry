#ifndef NETWORK_H
#define NETWORK_H

#include "esp_err.h"

void network_init(void);
void wifi_connect_init(void);
esp_err_t wifi_connect_sta(char *ssid, char *pass, int timeout);
void wifi_disconnect(void);

#endif

