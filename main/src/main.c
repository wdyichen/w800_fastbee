#include <stdio.h>
#include <math.h>
#include <string.h>
#include "wm_event.h"
#include "wm_wifi.h"
#include "wm_nvs.h"
#include "wm_utils.h"
#include "wm_mqtt_client.h"
#include "wm_drv_gpio.h"
#include "lwipopts.h"
#include "webnet.h"
#include "wn_module.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "cJSON/cJSON.h"
#include "led.h"
#include "ota.h"
#include "mqtt.h"
#include "wps_app.h"
#include "lwip/ip_addr.h"

#define LOG_TAG "iot"
#include "wm_log.h"

#define TASK_STACK           512
#define TASK_PRIO            1

#define SOFTAP_SSID          "fastbee-device"
#define SOFTAP_PWD           "12345678"
#define SOFTAP_CHANNEL       6
#define SOFTAP_MAX_STA_CONN  3

#define KEY_PIN              WM_GPIO_NUM_21
#define KEY_PIN2             WM_GPIO_NUM_0

#define NVS_GROUP_NAME       "fastbee"
#define NVS_CONFIG_NAME      "iot_config"

#define QUEUE_MSG_TYPE_KEY   1
#define QUEUE_MSG_TYPE_KEY2  2
#define QUEUE_MSG_TYPE_CLOSE 3
#define QUEUE_MSG_TYPE_MQTT  4
#define QUEUE_MSG_TYPE_TEMP  5
#define QUEUE_MSG_TYPE_PUB   6

struct iot_config {
    bool is_config;
    char ssid[33];
    char pwd[65];
};

static QueueHandle_t work_queue     = NULL;
static struct iot_config iot_config = { 0 };
static wm_mqtt_client_handle_t mqtt_client;

int get_value_from_json(const char *json_string)
{
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        return -1;
    }

    if (!cJSON_IsArray(json)) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON *json_item = cJSON_GetArrayItem(json, 0);
    if (json_item == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    cJSON *json_value = cJSON_GetObjectItem(json_item, "value");
    if (json_value == NULL) {
        cJSON_Delete(json);
        return -1;
    }

    if (!cJSON_IsString(json_value)) {
        cJSON_Delete(json);
        return -1;
    }

    int value = atoi(json_value->valuestring);

    cJSON_Delete(json);

    return value;
}

void save_config(const char *ssid, const char *password)
{
    int ret;
    wm_nvs_handle_t handle = NULL;

    ret = wm_nvs_open(NULL, NVS_GROUP_NAME, WM_NVS_OP_READ_WRITE, &handle);

    if (!ret){
        strcpy(iot_config.ssid, ssid);
        strcpy(iot_config.pwd, password);
        iot_config.is_config = true;
        wm_nvs_set_blob(handle, NVS_CONFIG_NAME, &iot_config, sizeof(iot_config));
        wm_nvs_close(handle);

        int msg = QUEUE_MSG_TYPE_CLOSE;
        xQueueSend(work_queue, &msg, 0);
    }
}

static void cgi_status_handler(struct webnet_session *session, void *handler_priv)
{
    const char *mimetype;
    static const char *status = "Device detected!";

    mimetype = mime_get_type(".html");

    /* set http header */
    session->request->result_code = 200;
    webnet_session_set_header(session, mimetype, 200, "Ok", strlen(status));
    webnet_session_write(session, (const uint8_t *)status, strlen(status));

    return;
}

static void cgi_config_handler(struct webnet_session *session, void *handler_priv)
{
    const char *mimetype;
    struct webnet_request *request;
    static const char *response_success = "Configuration successful!";
    static const char *response_fail    = "Configuration failed!";

    request  = session->request;
    mimetype = mime_get_type(".html");

    if (request->query_counter >= 2) {
        const char *ssid     = webnet_request_get_query(request, "SSID");
        const char *password = webnet_request_get_query(request, "password");

        if (ssid && password) {
            session->request->result_code = 200;
            webnet_session_set_header(session, mimetype, 200, "Ok", strlen(response_success));
            webnet_session_write(session, (const uint8_t *)response_success, strlen(response_success));

            save_config(ssid, password);
        } else {
            session->request->result_code = 500;
            webnet_session_set_header(session, mimetype, 500, "Internal Server Error", strlen(response_fail));
            webnet_session_write(session, (const uint8_t *)response_fail, strlen(response_fail));
        }
    }
}

static void led_state_upload(int state)
{
    char message[128];

    sprintf(message, "[{\"id\":\"switch\",\"remark\":\"\",\"value\":\"%d\"}]", state);

    if (mqtt_client) {
        wm_log_info("publish message is %s", message);

        wm_mqtt_client_publish(mqtt_client, TOPIC_MQTT_PUBLISH, message, 0, 1, 0);
    }
}

static void temp_state_upload(float temp)
{
    char message[128];

    sprintf(message, "[{\"id\":\"temp\",\"remark\":\"\",\"value\":\"%.1f\"}]", temp);

    if (mqtt_client) {
        wm_log_info("publish message is %s", message);

        wm_mqtt_client_publish(mqtt_client, TOPIC_MQTT_PUBLISH, message, 0, 1, 0);
    }
}

#if 0
static void temp_calc_timer(TimerHandle_t xTimer)
{
    int msg = QUEUE_MSG_TYPE_TEMP;
    xQueueSend(work_queue, &msg, 0);
}
#endif

static void event_handler_ap(wm_event_group_t group, int event, wm_wifi_event_data_t *data, void *priv)
{
    if (event == WM_EVENT_WIFI_AP_START) {
        webnet_init(0);
        webnet_cgi_set_root("/");
        webnet_cgi_register("status", cgi_status_handler, NULL);
        webnet_cgi_register("config", cgi_config_handler, NULL);
    }
}

static void event_handler_sta_got_ip(wm_event_group_t group, int event, wm_lwip_event_data_t *data, void *priv)
{
    if (event == WM_EVENT_WIFI_STA_GOT_IP) {
        char ip[16];
        ipaddr_ntoa_r((ip_addr_t *)&data->sta_got_ip_info.ip, ip, sizeof(ip));
        wm_log_info("got ip: %s", ip);
        int msg = QUEUE_MSG_TYPE_MQTT;
        xQueueSend(work_queue, &msg, 0);
    }
}

static void event_handler_sta_disconnected(wm_event_group_t group, int event, void *data, void *priv)
{
    if (event == WM_EVENT_WIFI_STA_DISCONNECTED) {
        wm_wifi_connect();
        wm_log_info("retry to connect to the AP");
    }
}

static void event_handler_mqtt_connected(wm_mqtt_event_data_t *data, void *priv)
{
    int msg = QUEUE_MSG_TYPE_PUB;

    wm_log_info("MQTTS connected");

    xQueueSend(work_queue, &msg, 0);
}

static void event_handler_mqtt_disconnected(wm_mqtt_event_data_t *data, void *priv)
{
    int msg = QUEUE_MSG_TYPE_MQTT;

    wm_log_info("MQTTS disconnected");

    xQueueSend(work_queue, &msg, 0);
}

static void event_handler_mqtt_recved_data(wm_mqtt_event_data_t *data, void *priv)
{
    int ret = WM_ERR_SUCCESS;
    int last_state;

    wm_log_info("MQTT recved");
    wm_log_info("Topic: %.*s", data->mqtt_client_data_info.topic_len, data->mqtt_client_data_info.topic);
    wm_log_info("Payload: %.*s", data->mqtt_client_data_info.payload_len, data->mqtt_client_data_info.payload);

    if (strstr(data->mqtt_client_data_info.topic, TOPIC_OTA_UPDATE) /*||
        strstr(data->mqtt_client_data_info.topic, TOPIC_OTA_RESPONSE)*/) {
        ret = ota_parse_update(data->mqtt_client_data_info.payload);
        if (ret != WM_ERR_SUCCESS) {
            wm_log_error("OTA update parse error: %d", ret);
        }
    } else if (strstr(data->mqtt_client_data_info.topic, TOPIC_MQTT_SUBSCRIBE)) {
        int value = get_value_from_json(data->mqtt_client_data_info.payload);

        if (strstr(data->mqtt_client_data_info.payload, "\"id\":\"switch\""))
        {
            if (value)
                led_on();
            else
                led_off();
        }
        else if (strstr(data->mqtt_client_data_info.payload, "\"id\":\"brightness\""))
        {
            last_state = led_state();
            led_brightness(value);
            if (led_state() != last_state)
                led_state_upload(led_state());
        }
        else if (strstr(data->mqtt_client_data_info.payload, "\"id\":\"red\""))
        {
            last_state = led_state();
            led_red(value);
            if (led_state() != last_state)
                led_state_upload(led_state());
        }
        else if (strstr(data->mqtt_client_data_info.payload, "\"id\":\"green\""))
        {
            last_state = led_state();
            led_green(value);
            if (led_state() != last_state)
                led_state_upload(led_state());
        }
        else if (strstr(data->mqtt_client_data_info.payload, "\"id\":\"blue\""))
        {
            last_state = led_state();
            led_blue(value);
            if (led_state() != last_state)
                led_state_upload(led_state());
        }
    }
}

static void event_handler_mqtt_event(wm_event_group_t group, int event, wm_mqtt_event_data_t *data, void *priv)
{
    switch (event) {
        case WM_EVENT_MQTT_CLIENT_CONNECTED:
            event_handler_mqtt_connected(data, priv);
            break;
        case WM_EVENT_MQTT_CLIENT_DISCONNECTED:
            event_handler_mqtt_disconnected(data, priv);
            break;
        case WM_EVENT_MQTT_CLIENT_DATA:
            event_handler_mqtt_recved_data(data, priv);
            break;
        default:
            break;
    }
}

static void softap_config_wifi(void)
{
    int ret;
    wm_wifi_config_t wifi_config = {
        .ap = {
            .ssid = SOFTAP_SSID,
            .ssid_len = strlen(SOFTAP_SSID),
            .channel = SOFTAP_CHANNEL,
            //.password = SOFTAP_PWD,
            .max_connection = SOFTAP_MAX_STA_CONN,
            .authmode = WM_WIFI_AUTH_OPEN,//WM_WIFI_AUTH_WPA2_PSK,
            .pairwise_cipher = WM_WIFI_CIPHER_TYPE_NONE,//WM_WIFI_CIPHER_TYPE_CCMP,
        },
    };

    wm_wifi_set_config(WM_WIFI_IF_AP, &wifi_config);

    wm_event_add_callback(WM_WIFI_EV, WM_EVENT_WIFI_AP_START, (wm_event_callback)event_handler_ap, NULL);

    ret = wm_wifi_ap_start();
    if (WM_ERR_SUCCESS == ret) {
        wm_log_info("start softap ok");
    } else {
        wm_log_error("start softap error");
    }
}

static void wifi_connect(void)
{
    wm_wifi_config_t conf;

    wm_log_info("wifi connect to '%s', pwd %s", iot_config.ssid, iot_config.pwd);

    memset(&conf, 0, sizeof(conf));

    strcpy((char *)conf.sta.ssid, iot_config.ssid);
    strcpy((char *)conf.sta.password, iot_config.pwd);

    wm_wifi_set_config(WM_WIFI_IF_STA, &conf);

    wm_wifi_connect();
}

static void check_config(void)
{
    int ret;
    wm_nvs_handle_t handle = NULL;
    size_t blob_len = sizeof(iot_config);

    wm_event_add_callback(WM_LWIP_EV, WM_EVENT_ANY_TYPE, (wm_event_callback)event_handler_sta_got_ip, NULL);
    wm_event_add_callback(WM_WIFI_EV, WM_EVENT_ANY_TYPE, event_handler_sta_disconnected, NULL);

    ret = wm_nvs_open(NULL, NVS_GROUP_NAME, WM_NVS_OP_READ_ONLY, &handle);
    if (!ret) {
        ret = wm_nvs_get_blob(handle, NVS_CONFIG_NAME, &iot_config, &blob_len);
        if ((WM_ERR_SUCCESS == ret) && (iot_config.is_config == true)) {
            wm_log_info("have valid config, connect...");
            wifi_connect();
        } else {
            wm_log_info("invalid config, enter config...");
            wps_app_init();
            softap_config_wifi();
        }
        wm_nvs_close(handle);
    }
}

static void key_callback(void *arg)
{
    int msg = QUEUE_MSG_TYPE_KEY;

    xQueueSend(arg, &msg, 0);
}

static void key2_callback(void *arg)
{
    int msg = QUEUE_MSG_TYPE_KEY2;

    xQueueSend(arg, &msg, 0);
}

static void key_init(void *queue)
{
    wm_drv_gpio_init("gpio");

    wm_drv_gpio_iomux_func_sel(KEY_PIN, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(KEY_PIN, WM_GPIO_DIR_INPUT);
    wm_drv_gpio_set_pullmode(KEY_PIN, WM_GPIO_FLOAT);
    wm_drv_gpio_set_intr_mode(KEY_PIN, WM_GPIO_IRQ_TRIG_FALLING_EDGE);
    wm_drv_gpio_add_isr_callback(KEY_PIN, key_callback, queue);
    wm_drv_gpio_enable_isr(KEY_PIN);

    wm_drv_gpio_iomux_func_sel(KEY_PIN2, WM_GPIO_IOMUX_FUN5);
    wm_drv_gpio_set_dir(KEY_PIN2, WM_GPIO_DIR_INPUT);
    wm_drv_gpio_set_pullmode(KEY_PIN2, WM_GPIO_PULL_UP);
    wm_drv_gpio_set_intr_mode(KEY_PIN2, WM_GPIO_IRQ_TRIG_FALLING_EDGE);
    wm_drv_gpio_add_isr_callback(KEY_PIN2, key2_callback, queue);
    wm_drv_gpio_enable_isr(KEY_PIN2);
}

static void led_flashing(int count)
{
    int state = led_state();

    for (int i = 0; i < count; i++) {
        if (led_state())
            led_off();
        else
            led_on();

        vTaskDelay(pdMS_TO_TICKS(300));
    }

    if (state)
        led_off();
    else
        led_on();
}

static int mqtt_connect(void)
{
    int ret = WM_ERR_SUCCESS;

    wm_mqtt_client_config_t mqtt_cfg = {
        .uri       = BROKER_URI,
        .client_id = CLIENT_ID,
        .username  = USER_NAME,
        .password  = PASSWORD,
    };

    if (mqtt_client) {
        wm_log_info("mqtt reconnect...");
        wm_mqtt_client_reconnect(mqtt_client);
    } else {
        mqtt_client = wm_mqtt_client_init(&mqtt_cfg);
        if (mqtt_client) {
            wm_event_add_callback(WM_MQTT_EV, WM_EVENT_ANY_TYPE, (wm_event_callback)event_handler_mqtt_event, mqtt_client);

            wm_log_info("mqtt connect...");
            ret = wm_mqtt_client_connect(mqtt_client);
            if (ret != WM_ERR_SUCCESS) {
                wm_log_error("Failed to connect MQTT client.");
            }
        } else {
            wm_log_error("Failed to initialize MQTT client.");
            return WM_ERR_FAILED;
        }
    }

    return ret;
}

static float get_random_temperature(void)
{
    float min     = 20.0;
    float max     = 50.0;
    int precision = 1; //0.1

    float increment = pow(10.0, precision);

    int randomInt = rand() % ((int)(max * increment) - (int)(min * increment) + 1) + (int)(min * increment);

    return (float)randomInt / increment;
}

static void work_task(void *param)
{
    void *msg = NULL;
    int ret;
    wm_nvs_handle_t handle = NULL;

    while (1) {
        if (xQueueReceive(param, (void **)&msg, portMAX_DELAY) == pdTRUE) {
            switch ((int)msg) {
                case QUEUE_MSG_TYPE_KEY:
                {
                    uint32_t time = xTaskGetTickCount();
                    while (0 == wm_drv_gpio_data_get(KEY_PIN)) {
                        if ((xTaskGetTickCount() - time) >= pdMS_TO_TICKS(3000)) {
                            //long press >= 3s
                            wm_log_info("reset config");
                            ret = wm_nvs_open(NULL, NVS_GROUP_NAME, WM_NVS_OP_READ_WRITE, &handle);
                            if (!ret) {
                                iot_config.is_config = false;
                                wm_nvs_set_blob(handle, NVS_CONFIG_NAME, &iot_config, sizeof(iot_config));
                                wm_nvs_close(handle);
                                led_flashing(3);
                                wm_system_reboot();
                            }
                            break;
                        }
                    }
                    if ((xTaskGetTickCount() - time) <= pdMS_TO_TICKS(500)) {
                        //short press <= 500ms
                        if (led_state()) {
                            wm_log_info("led turn off");
                            led_off();
                        } else {
                            wm_log_info("led turn on");
                            led_on();
                        }
                        led_state_upload(led_state());
                    }
                    break;
                }
                case QUEUE_MSG_TYPE_KEY2:
                {
                    uint32_t time = xTaskGetTickCount();
                    while (0 == wm_drv_gpio_data_get(KEY_PIN2)) {
                        if ((xTaskGetTickCount() - time) >= pdMS_TO_TICKS(3000)) {
                            //long press >= 3s
                            led_breathing(1);
                            break;
                        }
                    }
                    if ((xTaskGetTickCount() - time) <= pdMS_TO_TICKS(500)) {
                        //short press <= 500ms
                        led_breathing(0);
                    }
                    break;
                }
                case QUEUE_MSG_TYPE_CLOSE:
                {
                    wm_event_remove_callback(WM_WIFI_EV, WM_EVENT_WIFI_AP_START, (wm_event_callback)event_handler_ap, NULL);
                    wm_close_webnet_thread();
                    wm_wifi_ap_stop();
                    //wps_app_uninit();
                    wifi_connect();
                    led_flashing(3);
                    break;
                }
                case QUEUE_MSG_TYPE_MQTT:
                {
                    mqtt_connect();
                    break;
                }
                case QUEUE_MSG_TYPE_TEMP:
                {
                    temp_state_upload(get_random_temperature());
                    break;
                }
                case QUEUE_MSG_TYPE_PUB:
                {
                    set_firmware_type("INIT");

                    wm_mqtt_client_subscribe(mqtt_client, TOPIC_OTA_UPDATE, 1);
                    wm_mqtt_client_subscribe(mqtt_client, TOPIC_MQTT_SUBSCRIBE, 0);

                    //temp_state_upload(get_random_temperature());
                    led_state_upload(led_state());
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
}

int main(void)
{
    //TimerHandle_t xTimer;

    work_queue = xQueueCreate(4, sizeof(void *));
    xTaskCreate(work_task, "work", TASK_STACK, work_queue, TASK_PRIO, NULL);

    //srand(xTaskGetTickCount());
    //xTimer = xTimerCreate("temp", pdMS_TO_TICKS(15 * 1000), pdTRUE, NULL, temp_calc_timer);
    //xTimerStart(xTimer, 0);

    key_init(work_queue);

    led_init();

    wm_wifi_init();

    check_config();

    return 0;
}
