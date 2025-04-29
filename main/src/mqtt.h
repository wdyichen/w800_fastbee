#ifndef __MQTT_H__
#define __MQTT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BROKER_URI            "mqtt://iot.fastbee.cn:1883"

#define USER_NAME             "admin"
#define PASSWORD              "P63SLQEDIOTU5A5A"

#define PRODUCT_ID            "3916"
#define DEVICE_ID             "D62MU97K8BZ6"

#define CLIENT_ID             "S&" DEVICE_ID "&" PRODUCT_ID "&1"

#define TOPIC_OTA_UPDATE      "/" PRODUCT_ID "/" DEVICE_ID "/ota/get"
#define TOPIC_MQTT_SUBSCRIBE  "/" PRODUCT_ID "/" DEVICE_ID "/function/get"

#define TOPIC_DEV_INFO_GET    "/" PRODUCT_ID "/" DEVICE_ID "/info/get"
#define TOPIC_MONITOR_GET     "/" PRODUCT_ID "/" DEVICE_ID "/monitor/get"

#define TOPIC_MQTT_PUBLISH    "/" PRODUCT_ID "/" DEVICE_ID "/property/post"
#define TOPIC_MONITOR_PUBLISH "/" PRODUCT_ID "/" DEVICE_ID "/monitor/post"

#ifdef __cplusplus
}
#endif

#endif /* __MQTT_H__ */
