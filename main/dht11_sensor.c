#include "dht11_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "DHT11_SENSOR";
extern bool mqtt_connected;
static SemaphoreHandle_t dht11_mutex = NULL;

esp_err_t dht11_sensor_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensor DHT11 no GPIO %d", DHT11_GPIO);
    
    // Cria mutex para proteger leitura do DHT11
    if (dht11_mutex == NULL) {
        dht11_mutex = xSemaphoreCreateMutex();
        if (dht11_mutex == NULL) {
            ESP_LOGE(TAG, "Falha ao criar mutex do DHT11");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Mutex DHT11 criado com sucesso");
    }
    
    // Configura GPIO como saída inicialmente
    gpio_reset_pin(DHT11_GPIO);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    
    ESP_LOGI(TAG, "Sensor DHT11 inicializado");
    return ESP_OK;
}

esp_err_t dht11_sensor_read(int16_t *humidity, int16_t *temperature)
{
    if (humidity == NULL || temperature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Aguarda mutex por até 1 segundo
    if (xSemaphoreTake(dht11_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGW(TAG, "Timeout ao aguardar mutex");
        return ESP_ERR_TIMEOUT;
    }
    
    uint8_t bits[5] = {0, 0, 0, 0, 0};
    uint8_t cnt = 7;
    uint8_t idx = 0;
    *humidity = 0;
    *temperature = 0;
    
    // ====== SINAL DE START ======
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));  // 20ms LOW
    
    // Desabilita interrupções SOMENTE durante a comunicação rápida
    taskDISABLE_INTERRUPTS();
    
    gpio_set_level(DHT11_GPIO, 1);
    ets_delay_us(40);  // 40us HIGH
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    
    // ====== AGUARDA RESPOSTA ======
    // Espera sensor puxar LOW
    uint32_t timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) {
        if (++timeout > 200) {  // Aumentado de 100 para 200
            taskENABLE_INTERRUPTS();
            xSemaphoreGive(dht11_mutex);
            ESP_LOGW(TAG, "Timeout: sensor não puxou LOW");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }
    
    // Espera sensor puxar HIGH (resposta)
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 0) {
        if (++timeout > 200) {  // Aumentado de 100 para 200
            taskENABLE_INTERRUPTS();
            xSemaphoreGive(dht11_mutex);
            ESP_LOGW(TAG, "Timeout: sensor não respondeu HIGH");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }
    
    // Espera sensor voltar LOW (início da transmissão)
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) {
        if (++timeout > 200) {  // Aumentado de 100 para 200
            taskENABLE_INTERRUPTS();
            xSemaphoreGive(dht11_mutex);
            ESP_LOGW(TAG, "Timeout: início da transmissão");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }
    
    // ====== LÊ 40 BITS ======
    for (int i = 0; i < 40; i++) {
        // Espera LOW terminar
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 0) {
            if (++timeout > 150) {  // Aumentado de 100 para 150
                taskENABLE_INTERRUPTS();
                xSemaphoreGive(dht11_mutex);
                ESP_LOGW(TAG, "Timeout bit %d (LOW)", i);
                return ESP_FAIL;
            }
            ets_delay_us(1);
        }
        
        // Mede duração do HIGH
        uint32_t high_count = 0;
        while (gpio_get_level(DHT11_GPIO) == 1) {
            if (++high_count > 250) {  // Aumentado de 200 para 250
                taskENABLE_INTERRUPTS();
                xSemaphoreGive(dht11_mutex);
                ESP_LOGW(TAG, "Timeout bit %d (HIGH)", i);
                return ESP_FAIL;
            }
            ets_delay_us(1);
        }
        
        // Bit 1 se HIGH > 40us, senão bit 0
        if (high_count > 40) {
            bits[idx] |= (1 << cnt);
        }
        
        if (cnt == 0) {
            cnt = 7;
            idx++;
        } else {
            cnt--;
        }
    }
    
    // Reabilita interrupções
    taskENABLE_INTERRUPTS();
    
    // ====== CHECKSUM ======
    uint8_t checksum = (bits[0] + bits[1] + bits[2] + bits[3]) & 0xFF;
    if (bits[4] != checksum) {
        ESP_LOGW(TAG, "Checksum fail: calc=0x%02X recv=0x%02X", checksum, bits[4]);
        ESP_LOGD(TAG, "Data: %02X %02X %02X %02X %02X", bits[0], bits[1], bits[2], bits[3], bits[4]);
        xSemaphoreGive(dht11_mutex);
        return ESP_FAIL;
    }
    
    *humidity = bits[0];
    *temperature = bits[2];
    
    xSemaphoreGive(dht11_mutex);
    
    ESP_LOGI(TAG, "✓ Temp=%d°C Umid=%d%%", *temperature, *humidity);
    
    return ESP_OK;
}

void dht11_sensor_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    int16_t temperature = 0, humidity = 0;
    char message[256];
    int counter = 0;
    
    ESP_LOGI(TAG, "Task do sensor DHT11 iniciada");
    
    // Aguarda inicialização
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        if (mqtt_connected && client != NULL) {
            // Tenta ler até 3 vezes com intervalo de 2.5s entre tentativas
            esp_err_t res = ESP_FAIL;
            int retry = 0;
            const int max_retries = 3;
            
            for (retry = 0; retry < max_retries && res != ESP_OK; retry++) {
                if (retry > 0) {
                    ESP_LOGW(TAG, "Tentativa %d/%d...", retry + 1, max_retries);
                    vTaskDelay(pdMS_TO_TICKS(2500)); // DHT11 precisa de 2s mínimo
                }
                
                res = dht11_sensor_read(&humidity, &temperature);
            }
            
            if (res == ESP_OK) {
                // Obtém timestamp Unix em segundos e converte para milissegundos
                time_t now;
                time(&now);
                int64_t timestamp_ms = (int64_t)now * 1000;
                
                // Obtém horário formatado para log
                struct tm timeinfo;
                localtime_r(&now, &timeinfo);
                char time_str[64];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
                
                snprintf(message, sizeof(message),
                    "{\"device_id\":\"ESP32_Client\",\"temperature\":%d,\"humidity\":%d,\"counter\":%d,\"timestamp\":%lld,\"datetime\":\"%s\",\"retries\":%d}",
                    temperature, humidity, counter, timestamp_ms, time_str, retry);
                
                int msg_id = esp_mqtt_client_publish(client, TOPIC_DHT11, message, 0, 1, 0);
                ESP_LOGI(TAG, "Publicado [%s] [msg_id=%d]: Temp=%d°C, Umid=%d%% (tentativas:%d)", 
                         time_str, msg_id, temperature, humidity, retry);
                counter++;
            } else {
                ESP_LOGE(TAG, "Falha ao ler DHT11 após %d tentativas", max_retries);
            }
        } else {
            ESP_LOGW(TAG, "MQTT não conectado, aguardando...");
        }
        
        // Aguarda 10 segundos entre ciclos de leitura
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}