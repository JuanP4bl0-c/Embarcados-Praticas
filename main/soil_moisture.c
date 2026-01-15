#include "soil_moisture.h"
#include "plant_config.h"
#include "solenoid.h"
#include "system_commands.h"
#include "power_manager.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "SOIL_MOISTURE";
extern bool mqtt_connected;

// Handle ADC compartilhado - declarado externamente
extern adc_oneshot_unit_handle_t adc1_handle;

esp_err_t soil_moisture_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensor de umidade do solo no GPIO %d", SOIL_MOISTURE_GPIO);
    
    // Configura canal 5 (GPIO33)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_5, &config));
    
    ESP_LOGI(TAG, "Sensor de umidade do solo inicializado");
    return ESP_OK;
}

esp_err_t soil_moisture_read(int *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Lê o valor do ADC (0-4095)
    // Valores altos = solo seco, valores baixos = solo úmido
    int raw_value = 0;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_5, &raw_value);
    if (ret == ESP_OK) {
        *value = raw_value;
    }
    
    return ret;
}

void soil_moisture_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    int moisture_value = 0;
    char message[256];
    int counter = 0;
    
    ESP_LOGI(TAG, "Task do sensor de umidade do solo iniciada");
    
    // Aguarda inicialização
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        if (mqtt_connected && client != NULL) {
            esp_err_t res = soil_moisture_read(&moisture_value);
            
            if (res == ESP_OK) {
                int64_t timestamp_ms = esp_timer_get_time() / 1000;
                
                // Converte para porcentagem aproximada (0-100%)
                // 4095 (seco) -> 0%, 0 (úmido) -> 100%
                int moisture_percent = 100 - ((moisture_value * 100) / 4095);
                
                snprintf(message, sizeof(message),
                    "{\"device_id\":\"ESP32_Client\",\"moisture_raw\":%d,\"moisture_percent\":%d,\"counter\":%d,\"timestamp\":%lld}",
                    moisture_value, moisture_percent, counter, timestamp_ms);
                
                int msg_id = esp_mqtt_client_publish(client, TOPIC_SOIL_MOISTURE, message, 0, 1, 0);
                ESP_LOGI(TAG, "Publicado [msg_id=%d]: %s", msg_id, message);
                
                // Marca que publicou dados
                power_manager_mark_sensor_published("soil");
                
                // Verifica se precisa irrigar automaticamente
                if (plant_config_should_irrigate(moisture_percent)) {
                    ESP_LOGW(TAG, "Acionando irrigação automática!");
                    
                    // Publica alerta no MQTT
                    char alert_msg[256];
                    snprintf(alert_msg, sizeof(alert_msg),
                        "{\"device_id\":\"ESP32_Client\",\"type\":\"auto_irrigation\","
                        "\"moisture\":%d,\"threshold\":%d,\"timestamp\":%lld}",
                        moisture_percent, 
                        plant_config_get()->soil_moisture_min - plant_config_get()->irrigation_threshold,
                        timestamp_ms);
                    esp_mqtt_client_publish(client, TOPIC_ALERTS, alert_msg, 0, 1, 0);
                    
                    // Liga o solenoide
                    solenoid_control(true);
                    
                    // Mantém ligado por 10 segundos
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    
                    // Desliga o solenoide
                    solenoid_control(false);
                    ESP_LOGI(TAG, "Irrigação automática concluída");
                }
            } else {
                ESP_LOGW(TAG, "Falha ao ler sensor de umidade: %d", res);
            }
            counter++;
        } else {
            ESP_LOGW(TAG, "MQTT não conectado, aguardando...");
        }
        
        // Usa período configurável com power management
        int delay_ms = system_commands_get_read_period_ms();
        
        if (power_manager_should_sleep(delay_ms)) {
            power_manager_sleep(delay_ms);
        } else {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

void soil_moisture_force_publish(esp_mqtt_client_handle_t client)
{
    if (client == NULL) {
        ESP_LOGW(TAG, "Cliente MQTT NULL");
        return;
    }
    
    int moisture_value = 0;
    esp_err_t res = soil_moisture_read(&moisture_value);
    
    if (res == ESP_OK) {
        char message[256];
        int64_t timestamp_ms = esp_timer_get_time() / 1000;
        int moisture_percent = 100 - ((moisture_value * 100) / 4095);
        
        snprintf(message, sizeof(message),
            "{\"device_id\":\"ESP32_Client\",\"moisture_raw\":%d,\"moisture_percent\":%d,\"forced\":true,\"timestamp\":%lld}",
            moisture_value, moisture_percent, timestamp_ms);
        
        esp_mqtt_client_publish(client, TOPIC_SOIL_MOISTURE, message, 0, 1, 0);
        ESP_LOGI(TAG, "Umidade do solo forçada: %d%% (%d raw)", moisture_percent, moisture_value);
    } else {
        ESP_LOGW(TAG, "Falha ao ler sensor de umidade do solo");
    }
}
