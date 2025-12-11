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

#include "rom/ets_sys.h"


// #define WIFI_SSID "UFC_QUIXADA"
// #define WIFI_SSID "ExpoSE"
// #define WIFI_PASS ""

#define WIFI_SSID "brisa-2504280"
#define WIFI_PASS "eubcidpn"

#define AWS_IOT_ENDPOINT "a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com"
#define AWS_IOT_CLIENT_ID "ESP32_Client"

//Sensores
#define UV_SENSOR_GPIO 34
#define SOIL_MOISTURE_GPIO 35
#define DHT11_GPIO 4

//Atuadores
#define SOLENOID_GPIO 18

//Topicos
#define TOPIC_UV_SENSOR "esp32/uv"
#define TOPIC_SOIL_MOISTURE "esp32/soil_moisture"
#define TOPIC_DHT11 "esp32/dht11"
#define TOPIC_SOLENOID "esp32/solenoid"

// Certificados que funcionam (86cf2f...)
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_certificate_pem_crt_start[] asm("_binary_86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c_certificate_pem_crt_start");
extern const uint8_t device_private_pem_key_start[] asm("_binary_86cf2f23eee12460caeaf6c97ae117032f556f433959a2bfd83039ab5fa3ae0c_private_pem_key_start");

esp_mqtt_client_handle_t client = NULL;
extern bool mqtt_connected;

// Protótipos das tasks
void publish_sensor_data_task(void *pvParameters);
void button_status_task(void *pvParameters);

esp_err_t dht11_read(int gpio, int16_t *humidity, int16_t *temperature) {
    uint8_t data[5] = {0};
    uint8_t byte = 0, bit = 7;
    int i = 0;
    *humidity = 0;
    *temperature = 0;

    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio, 0);
    ets_delay_us(18000); // 18ms
    gpio_set_level(gpio, 1);
    ets_delay_us(30); // 20-40us
    gpio_set_direction(gpio, GPIO_MODE_INPUT);

    uint32_t timeout = 0;
    while (gpio_get_level(gpio) == 1) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(gpio) == 0) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }
    timeout = 0;
    while (gpio_get_level(gpio) == 1) { if (++timeout > 100) return ESP_FAIL; ets_delay_us(1); }

    // Lê 40 bits
    for (i = 0; i < 40; i++) {
        // Espera início do bit
        timeout = 0;
        while (gpio_get_level(gpio) == 0) { if (++timeout > 70) return ESP_FAIL; ets_delay_us(1); }
        // Mede duração do pulso alto
        int t = 0;
        while (gpio_get_level(gpio) == 1) { t++; if (t > 100) return ESP_FAIL; ets_delay_us(1); }
        // Se pulso > 40us, é 1, senão 0
        if (t > 40) data[byte] |= (1 << bit);
        if (bit == 0) { bit = 7; byte++; } else { bit--; }
    }
    // Checa checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) return ESP_FAIL;
    *humidity = data[0];
    *temperature = data[2];
    return ESP_OK;
}

// Callback customizado para receber mensagens MQTT
void custom_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    // Verifica se é um evento de dados
    if (event_id == MQTT_EVENT_DATA) {
        // Extrai o tópico
        char topic[128] = {0};
        snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
        
        // Verifica se é o tópico solenoid
        if (strncmp(topic, TOPIC_SOLENOID, strlen(TOPIC_SOLENOID)) == 0) {
            ESP_LOGI("SOLENOID_TEST", "═══════════════════════════════════════");
            ESP_LOGI("SOLENOID_TEST", "📨 Mensagem recebida no tópico: %s", topic);
            ESP_LOGI("SOLENOID_TEST", "═══════════════════════════════════════");
            
            // Extrai os dados
            char data[256] = {0};
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);
            
            // Imprime os dados JSON recebidos
            ESP_LOGI("SOLENOID_TEST", "📄 JSON recebido:");
            ESP_LOGI("SOLENOID_TEST", "%s", data);
            ESP_LOGI("SOLENOID_TEST", "═══════════════════════════════════════");
            
            // Aqui você pode adicionar lógica para processar o JSON
            // Por exemplo, controlar o solenóide baseado nos dados
        }
    }
}

void app_main(void)
{
    ESP_LOGI("APP_MAIN", "═══════════════════════════════════════════════");
    ESP_LOGI("APP_MAIN", "     🚀 INICIANDO PROJETO ESP32 AWS IoT 🚀    ");
    ESP_LOGI("APP_MAIN", "═══════════════════════════════════════════════");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI("APP_MAIN", "✅ NVS Flash inicializado");

    // Inicializa WiFi
    ESP_LOGI("APP_MAIN", "📡 Conectando ao WiFi...");
    wifi_manager_init(WIFI_SSID, WIFI_PASS, WIFI_AUTH_OPEN);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Registra o handler customizado antes de iniciar o MQTT
    ESP_LOGI("APP_MAIN", "🔧 Registrando handler customizado para mensagens MQTT");
    mqtt_manager_set_custom_handler(custom_mqtt_event_handler);

    // Inicializa MQTT com os certificados corretos
    ESP_LOGI("APP_MAIN", "🔐 Iniciando conexão MQTT com AWS IoT...");
    mqtt_manager_start(AWS_IOT_ENDPOINT, AWS_IOT_CLIENT_ID, 
                       aws_root_ca_pem_start, 
                       device_certificate_pem_crt_start, 
                       device_private_pem_key_start, 
                       &client);
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Se conectado, faz o subscribe no tópico solenoid
    if (mqtt_connected && client != NULL) {
        ESP_LOGI("APP_MAIN", "📥 Fazendo subscribe no tópico: %s", TOPIC_SOLENOID);
        int msg_id = esp_mqtt_client_subscribe(client, TOPIC_SOLENOID, 1);
        ESP_LOGI("APP_MAIN", "✅ Subscribe enviado, msg_id=%d", msg_id);
    } else {
        ESP_LOGW("APP_MAIN", "⚠️ MQTT não conectado, não foi possível fazer subscribe");
    }

    xTaskCreate(button_status_task, "button_task", 2048, NULL, 5, NULL);

    ESP_LOGI("APP_MAIN", "═══════════════════════════════════════════════");
    ESP_LOGI("APP_MAIN", "✅ Aplicação iniciada com sucesso!");
    ESP_LOGI("APP_MAIN", "📨 Aguardando mensagens no tópico: %s", TOPIC_SOLENOID);
    ESP_LOGI("APP_MAIN", "═══════════════════════════════════════════════");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

// Implementação das tasks
void publish_sensor_data_task(void *pvParameters) {
    int counter = 0;
    char message[200];
    int16_t temperature = 0, humidity = 0;
    while (1) {
        if (mqtt_connected) {
            esp_err_t res = dht11_read(DHT11_GPIO, &humidity, &temperature);
            if (res == ESP_OK) {
                int64_t timestamp_ms = esp_timer_get_time() / 1000;
                snprintf(message, sizeof(message),
                    "{\"device_id\":\"%s\",\"Temperatura\":%d,\"humidade\":%d,\"cnt\":%d,\"tempo\":%lld}",
                    AWS_IOT_CLIENT_ID, temperature, humidity, counter, timestamp_ms);
                esp_mqtt_client_publish(client, "esp32/data", message, 0, 1, 0);
                ESP_LOGI("PUBLISH_TASK", "Publicado: %s", message);
            } else {
                ESP_LOGW("PUBLISH_TASK", "Falha ao ler DHT11: %d", res);
            }
            counter++;
        } else {
            ESP_LOGW("PUBLISH_TASK", "MQTT não conectado, aguardando...");
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// Task de monitoramento de status
void button_status_task(void *pvParameters) {
    ESP_LOGI("BUTTON_TASK", "Task de status iniciada");
    while (1) {
        if (mqtt_connected) {
            ESP_LOGI("BUTTON_TASK", "✅ MQTT Conectado");
        } else {
            ESP_LOGW("BUTTON_TASK", "⚠️ MQTT Desconectado");
        }
        vTaskDelay(pdMS_TO_TICKS(30000)); // Log a cada 30 segundos
    }
}


