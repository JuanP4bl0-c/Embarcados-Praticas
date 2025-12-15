#include "solenoid.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SOLENOID";
static bool solenoid_state = false;

esp_err_t solenoid_init(void)
{
    ESP_LOGI(TAG, "Inicializando solenoide no GPIO %d", SOLENOID_GPIO);
    
    // Configura GPIO como saída
    gpio_reset_pin(SOLENOID_GPIO);
    gpio_set_direction(SOLENOID_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(SOLENOID_GPIO, 0); // Inicia desligado
    
    solenoid_state = false;
    
    ESP_LOGI(TAG, "Solenoide inicializado (estado: OFF)");
    return ESP_OK;
}

esp_err_t solenoid_set_state(bool state)
{
    gpio_set_level(SOLENOID_GPIO, state ? 1 : 0);
    solenoid_state = state;
    
    ESP_LOGI(TAG, "Solenoide: %s", state ? "LIGADO" : "DESLIGADO");
    return ESP_OK;
}

bool solenoid_get_state(void)
{
    return solenoid_state;
}

void solenoid_control(bool state)
{
    solenoid_set_state(state);
}

void solenoid_mqtt_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    // Verifica se é um evento de dados
    if (event_id == MQTT_EVENT_DATA) {
        // Extrai o tópico
        char topic[128] = {0};
        snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
        
        // Verifica se é o tópico do solenoide
        if (strncmp(topic, TOPIC_SOLENOID, strlen(TOPIC_SOLENOID)) == 0) {
            ESP_LOGI(TAG, "═══════════════════════════════════════");
            ESP_LOGI(TAG, "Comando recebido no tópico: %s", topic);
            ESP_LOGI(TAG, "═══════════════════════════════════════");
            
            // Extrai os dados
            char data[256] = {0};
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);
            
            ESP_LOGI(TAG, "JSON recebido: %s", data);
            
            // Parse simples do JSON (sem biblioteca cJSON para evitar dependências)
            // Procura por "state":true ou "state":false
            bool new_state = false;
            
            if (strstr(data, "\"state\":true") != NULL || 
                strstr(data, "\"state\":\"on\"") != NULL ||
                strstr(data, "\"state\":1") != NULL ||
                strstr(data, "\"estado\":true") != NULL ||
                strstr(data, "\"estado\":\"ligado\"") != NULL) {
                new_state = true;
            } else if (strstr(data, "\"state\":false") != NULL ||
                       strstr(data, "\"state\":\"off\"") != NULL ||
                       strstr(data, "\"state\":0") != NULL ||
                       strstr(data, "\"estado\":false") != NULL ||
                       strstr(data, "\"estado\":\"desligado\"") != NULL) {
                new_state = false;
            } else {
                ESP_LOGW(TAG, "Formato de comando não reconhecido");
                ESP_LOGW(TAG, "Use: {\"state\":true} ou {\"state\":false}");
                return;
            }
            
            // Aplica o novo estado
            solenoid_set_state(new_state);
            ESP_LOGI(TAG, "Comando processado com sucesso!");
            
            ESP_LOGI(TAG, "═══════════════════════════════════════");
        }
    }
}
