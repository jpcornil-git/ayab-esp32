
#ifndef _SRV_SPIFFS_H_
#define _SRV_SPIFFS_H_

void srv_spiffs_start(char *base_path);
void srv_spiffs_restart();
void srv_spiffs_stop(void);

#endif