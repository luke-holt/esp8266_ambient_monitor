#include <string.h>

/* For getenv functions */
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"

#include "driver/i2c.h"

/* Project components for I2C sensors */
#include "i2c_helpers.h"
#include "am2301b.h"
#include "ltr390.h"


#define WIFI_SSID               CONFIG_WIFI_SSID
#define WIFI_PASS               CONFIG_WIFI_PASS
#define MAX_RETRY               5

#define MQTT_URI                CONFIG_ESP_MQTT_URI 

#define MQTT_QOS                1
#define MQTT_RETAIN             0
#define MQTT_TOPIC_HUM          "home/humidity/office"
#define MQTT_TOPIC_TMP          "home/temperature/office"
#define MQTT_TOPIC_ALS          "home/luminosity/office"
#define MQTT_TOPIC_UVS          "home/uv_intensity/office"
#define MQTT_MAX_TOPIC_LEN      128

#define MQTT_MSG_AVAIL_BIT      0x1
#define MQTT_BROKER_CON         0x1 << 1
#define MQTT_BROKER_DIS         0x1 << 2


/* MQTT client instance */
static esp_mqtt_client_handle_t client;

/* FreeRTOS task handles */
static TaskHandle_t i2c_task_handle;

/* FreeRTOS event group */
static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_mqtt_event_group;

/* Wifi events */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

/**
 * @brief Macro to quickly form MQTT message struct
 * (mqtt_msg_t)
 * 
 */
#define MQTT_MSG_CREATE(topic, payload)         \
{                                               \
    .topic = topic,                             \
    .payload = payload,                         \
    .len = 0,                                   \
    .qos = MQTT_QOS,                            \
    .retain = MQTT_RETAIN                       \
};

/**
 * @brief Struct to hold a MQTT message
 * 
 */
typedef struct mqtt_msg_t
{
    char *topic;        // Topic string
    char *payload;      // Payload string
    char *len;          // Payload string length
    uint8_t qos;        // Quality of Service
    uint8_t retain;     // Retain flag for broker
} mqtt_msg_t;


static const char *TAG = "esp8266_ambient_monitor";

QueueHandle_t mqtt_msg_queue = NULL;

/* Wifi retry counter */
static int s_retry_num = 0;


static void wifi_event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data)
{

    if (event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else
    {
        if (event_base == WIFI_EVENT)
            ESP_LOGI(TAG, "WIFI_EVENT %d", event_id);
        else if (event_base == IP_EVENT)
            ESP_LOGI(TAG, "IP_EVENT %d", event_id);
        else
            ESP_LOGI(TAG, "Unknown event in %s: %d", __func__, event_id);
    }
}


static void mqtt_event_handler(
    void* arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void* event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", event_base, event_id);

    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;

    switch((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        xTaskNotify(i2c_task_handle, 0, eNoAction);

        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        break;
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


static void mqtt_init_client()
{
    s_mqtt_event_group = xEventGroupCreate();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    esp_mqtt_client_start(client);
}


static void i2c_sensors_task(void *pvParameters)
{
    /* init */

    int i2c_master_port = I2C_MASTER_PORT;

    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_stretch_tick = 300,
    };

    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, config.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &config));

    am2301b_init();

    char hum_payload[25];
    char tmp_payload[25];
    char als_payload[25];
    char uvs_payload[25];

    uint8_t ret;
    
    // mqtt_msg_t msg_hum, msg_tmp, msg_als, msg_uvs, *ptr_msg; 

loop:

    /* AM2301B temperature and humidity sensor */
    ret = am2301b_trigger_measurement(hum_payload, tmp_payload);

    if (ret == I2C_OK)
    {
        // ESP_LOGI(TAG, "HUM: %s, TMP: %s", hum_payload, tmp_payload);
        esp_mqtt_client_publish(client, MQTT_TOPIC_HUM, hum_payload, 0, MQTT_QOS, MQTT_RETAIN);
        esp_mqtt_client_publish(client, MQTT_TOPIC_HUM, tmp_payload, 0, MQTT_QOS, MQTT_RETAIN);
    }
    else ESP_LOGI(TAG, "AM2301B trouble");

    /* LTR390 ambient light sensor */
    ret = ltr390_trigger_measurement(als_payload, uvs_payload);

    if (ret == I2C_OK)
    {
        // ESP_LOGI(TAG, "LUM: %s, UVI: %s", als_payload, uvs_payload);
        esp_mqtt_client_publish(client, MQTT_TOPIC_ALS, als_payload, 0, MQTT_QOS, 0);
        esp_mqtt_client_publish(client, MQTT_TOPIC_UVS, uvs_payload, 0, MQTT_QOS, 0);
    }
    else ESP_LOGI(TAG, "ltr390 trouble");

    vTaskDelay(20000 / portTICK_PERIOD_MS);

    goto loop;

    /* end */

    i2c_driver_delete(I2C_MASTER_PORT);
    vTaskDelete(NULL);
}


static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };

    if (strlen((char *)wifi_config.sta.password))
    {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to SSID: %s",
                WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "failed to connect to SSID:%s",
                WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED WIFI EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}


void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();
    mqtt_init_client();

    xTaskCreate(
        i2c_sensors_task,
        "i2c sensors task",
        2048,
        NULL,
        5,
        &i2c_task_handle
    );
}
