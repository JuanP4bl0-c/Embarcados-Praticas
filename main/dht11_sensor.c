#include "dht11_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DHT11_SENSOR";
extern bool mqtt_connected;

esp_err_t dht11_sensor_init(void)
{
    ESP_LOGI(TAG, "Inicializando sensor DHT11 no GPIO %d", DHT11_GPIO);
    
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
    
    uint8_t data[5] = {0};
    uint8_t byte = 0, bit = 7;
    int i = 0;
    *humidity = 0;
    *temperature = 0;

    // Envia sinal de início
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    ets_delay_us(18000); // 18ms
    gpio_set_level(DHT11_GPIO, 1);
    ets_delay_us(30); // 20-40us
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);

    // Aguarda resposta do sensor
    uint32_t timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) { 
        if (++timeout > 100) {
            ESP_LOGW(TAG, "Timeout esperando resposta inicial (1)");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }
    
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 0) { 
        if (++timeout > 100) {
            ESP_LOGW(TAG, "Timeout esperando resposta inicial (0)");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }
    
    timeout = 0;
    while (gpio_get_level(DHT11_GPIO) == 1) { 
        if (++timeout > 100) {
            ESP_LOGW(TAG, "Timeout esperando início dos dados");
            return ESP_FAIL;
        }
        ets_delay_us(1);
    }

    // Lê 40 bits de dados
    for (i = 0; i < 40; i++) {
        // Espera início do bit
        timeout = 0;
        while (gpio_get_level(DHT11_GPIO) == 0) { 
            if (++timeout > 70) {
                ESP_LOGW(TAG, "Timeout lendo bit %d (início)", i);
                return ESP_FAIL;
            }
            ets_delay_us(1);
        }
        
        // Mede duração do pulso alto
        int t = 0;
        while (gpio_get_level(DHT11_GPIO) == 1) { 
            t++; 
            if (t > 100) {
                ESP_LOGW(TAG, "Timeout lendo bit %d (duração)", i);
                return ESP_FAIL;
            }
            ets_delay_us(1);
        }
        
        // Se pulso > 40us, é 1, senão 0
        if (t > 40) {
            data[byte] |= (1 << bit);
        }
        
        if (bit == 0) { 
            bit = 7; 
            byte++; 
        } else { 
            bit--; 
        }
    }
    
    // Verifica checksum
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (data[4] != checksum) {
        ESP_LOGW(TAG, "Checksum inválido: esperado 0x%02X, recebido 0x%02X", checksum, data[4]);
        return ESP_FAIL;
    }
    
    *humidity = data[0];
    *temperature = data[2];
    
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
            esp_err_t res = dht11_sensor_read(&humidity, &temperature);
            
            if (res == ESP_OK) {
                int64_t timestamp_ms = esp_timer_get_time() / 1000;
                
                snprintf(message, sizeof(message),
                    "{\"device_id\":\"ESP32_Client\",\"temperature\":%d,\"humidity\":%d,\"counter\":%d,\"timestamp\":%lld}",
                    temperature, humidity, counter, timestamp_ms);
                
                int msg_id = esp_mqtt_client_publish(client, TOPIC_DHT11, message, 0, 1, 0);
                ESP_LOGI(TAG, "Publicado [msg_id=%d]: %s", msg_id, message);
            } else {
                ESP_LOGW(TAG, "Falha ao ler DHT11");
            }
            counter++;
        } else {
            ESP_LOGW(TAG, "MQTT não conectado, aguardando...");
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publica a cada 10 segundos
    }
}
