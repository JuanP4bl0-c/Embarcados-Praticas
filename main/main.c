#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "esp_adc/adc_oneshot.h"

// Módulos dos sensores e atuadores
#include "uv_sensor.h"
#include "soil_moisture.h"
#include "dht11_sensor.h"
#include "solenoid.h"
#include "plant_config.h"


// #define WIFI_SSID "UFC_QUIXADA"
// #define WIFI_PASS ""

#define WIFI_SSID "brisa-2504280"
#define WIFI_PASS "eubcidpn"

#define AWS_IOT_ENDPOINT "a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com"
#define AWS_IOT_CLIENT_ID "ESP32_Client"

static const char *TAG = "APP_MAIN";

adc_oneshot_unit_handle_t adc1_handle = NULL;

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_certificate_pem_crt_start[] asm("_binary_86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c_certificate_pem_crt_start");
extern const uint8_t device_private_pem_key_start[] asm("_binary_86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c_private_pem_key_start");

esp_mqtt_client_handle_t client = NULL;
extern bool mqtt_connected;

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash inicializado");

    ESP_LOGI(TAG, "Conectando ao WiFi: %s", WIFI_SSID);
    wifi_manager_init(WIFI_SSID, WIFI_PASS, WIFI_AUTH_OPEN);
    vTaskDelay(pdMS_TO_TICKS(5000));

    mqtt_manager_set_custom_handler(solenoid_mqtt_handler);
    mqtt_manager_start(AWS_IOT_ENDPOINT, AWS_IOT_CLIENT_ID, 
                       aws_root_ca_pem_start, 
                       device_certificate_pem_crt_start, 
                       device_private_pem_key_start, 
                       &client);
    vTaskDelay(pdMS_TO_TICKS(3000));

    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc1_handle));
    ESP_LOGI(TAG, "ADC Start");
    
    plant_config_init();
    
    uv_sensor_init();
    soil_moisture_init();
    dht11_sensor_init();
    solenoid_init();
    ESP_LOGI(TAG, "Todos os dispositivos inicializados");

    if (mqtt_connected && client != NULL) {
        ESP_LOGI(TAG, "subscribe nos tópicos.");
        int msg_id1 = esp_mqtt_client_subscribe(client, TOPIC_SOLENOID, 1);
        int msg_id2 = esp_mqtt_client_subscribe(client, TOPIC_PLANT_CONFIG, 1);
        ESP_LOGI(TAG, "Subscribed: %s (msg_id=%d)", TOPIC_SOLENOID, msg_id1);
        ESP_LOGI(TAG, "Subscribed: %s (msg_id=%d)", TOPIC_PLANT_CONFIG, msg_id2);
        
        // Publica configuração atual
        plant_config_publish(client);
    } else {
        ESP_LOGW(TAG, "MQTT não conectado, não foi possível fazer subscribe");
    }


    xTaskCreate(uv_sensor_task, "uv_task", 4096, (void*)client, 5, NULL);
    xTaskCreate(soil_moisture_task, "soil_task", 4096, (void*)client, 5, NULL);
    xTaskCreate(dht11_sensor_task, "dht11_task", 4096, (void*)client, 5, NULL);

    ESP_LOGI(TAG, "Sistema iniciado com sucesso!");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


