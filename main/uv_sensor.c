#include "uv_sensor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "UV_SENSOR";
extern bool mqtt_connected;

// Handle ADC compartilhado - declarado externamente
extern adc_oneshot_unit_handle_t adc1_handle;

esp_err_t uv_sensor_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensor UV no GPIO %d", UV_SENSOR_GPIO);
    
    // Configura canal 4 (GPIO32)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &config));
    
    ESP_LOGI(TAG, "Sensor UV inicializado");
    return ESP_OK;
}

esp_err_t uv_sensor_read(int *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Lê o valor do ADC (0-4095)
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw_value);
    if (ret == ESP_OK) {
        *value = raw_value;
    }
    
    return ret;
}

void uv_sensor_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    int uv_value = 0;
    char message[256];
    int counter = 0;
    
    ESP_LOGI(TAG, "Task do sensor UV iniciada");
    
    // Aguarda inicialização
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        if (mqtt_connected && client != NULL) {
            esp_err_t res = uv_sensor_read(&uv_value);
            
            if (res == ESP_OK) {
                int64_t timestamp_ms = esp_timer_get_time() / 1000;
                
                // Converte para voltagem aproximada (0-3.3V)
                float voltage = (uv_value / 4095.0) * 3.3;
                
                snprintf(message, sizeof(message),
                    "{\"device_id\":\"ESP32_Client\",\"uv_raw\":%d,\"uv_voltage\":%.2f,\"counter\":%d,\"timestamp\":%lld}",
                    uv_value, voltage, counter, timestamp_ms);
                
                int msg_id = esp_mqtt_client_publish(client, TOPIC_UV_SENSOR, message, 0, 1, 0);
                ESP_LOGI(TAG, "Publicado [msg_id=%d]: %s", msg_id, message);
            } else {
                ESP_LOGW(TAG, "Falha ao ler sensor UV: %d", res);
            }
            counter++;
        } else {
            ESP_LOGW(TAG, "MQTT não conectado, aguardando...");
        }
        
        vTaskDelay(pdMS_TO_TICKS(15000)); // Publica a cada 15 segundos
    }
}
