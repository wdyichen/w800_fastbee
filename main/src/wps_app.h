#ifndef __WPS_APP_H__
#define __WPS_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

int wps_app_init(void);

int wps_app_uninit(void);

void save_config(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif

#endif /* __WPS_APP_H__ */
