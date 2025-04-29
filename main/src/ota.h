#ifndef __OTA_H__
#define __OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

int set_firmware_type(char *fw_type);

int ota_parse_update(char *payload);

#ifdef __cplusplus
}
#endif

#endif /* __OTA_H__ */
