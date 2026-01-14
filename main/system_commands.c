#include "system_commands.h"
#include "solenoid.h"
#include "dht11_sensor.h"
#include "uv_sensor.h"
#include "soil_moisture.h"
#include "plant_config.h"
#include "ntp_sync.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "SYS_CMD";

// Configuração global do sistema
static system_config_t system_config = {
    .read_period_minutes = 1,  // Padrão: 1 minuto
    .solenoid_enabled = true   // Padrão: habilitado
};

void system_commands_init(void)
{
    ESP_LOGI(TAG, "Módulo de comandos do sistema inicializado");
    ESP_LOGI(TAG, "Período de leitura padrão: %d minutos", system_config.read_period_minutes);
    ESP_LOGI(TAG, "Tópico de comandos: %s", TOPIC_SYSTEM_COMMANDS);
}

int system_commands_get_read_period_ms(void)
{
    return system_config.read_period_minutes * 60 * 1000;
}

void system_commands_set_read_period_minutes(int minutes)
{
    if (minutes < 1) {
        ESP_LOGW(TAG, "Período mínimo é 1 minuto, ajustando...");
        minutes = 1;
    } else if (minutes > 1440) {
        ESP_LOGW(TAG, "Período máximo é 1440 minutos (24h), ajustando...");
        minutes = 1440;
    }
    
    system_config.read_period_minutes = minutes;
    ESP_LOGI(TAG, "Período de leitura atualizado: %d minutos", minutes);
}

void system_commands_publish_all_data(esp_mqtt_client_handle_t client)
{
    if (client == NULL) {
        ESP_LOGW(TAG, "Cliente MQTT NULL, não é possível publicar dados");
        return;
    }
    
    ESP_LOGI(TAG, "📤 Publicando todos os dados dos sensores...");
    
    // Publica DHT11
    float temperature, humidity;
    if (dht11_read_data(&temperature, &humidity)) {
        char dht_payload[256];
        time_t now = time(NULL);
        char time_str[64];
        ntp_get_time_string(time_str, sizeof(time_str));
        
        snprintf(dht_payload, sizeof(dht_payload),
                "{\"temperature\":%.1f,\"humidity\":%.1f,\"timestamp\":%lld,\"datetime\":\"%s\"}",
                temperature, humidity, (long long)(now * 1000), time_str);
        
        esp_mqtt_client_publish(client, "esp32/sensor/dht11", dht_payload, 0, 1, 0);
        ESP_LOGI(TAG, "✅ DHT11: T=%.1f°C, H=%.1f%%", temperature, humidity);
    }
    
    // Publica UV (força leitura)
    uv_sensor_force_publish(client);
    
    // Publica Soil Moisture (força leitura)
    soil_moisture_force_publish(client);
    
    // Publica configuração da planta
    plant_config_publish(client);
    
    ESP_LOGI(TAG, "✅ Todos os dados publicados!");
}

void system_commands_publish_status(esp_mqtt_client_handle_t client)
{
    if (client == NULL) {
        ESP_LOGW(TAG, "Cliente MQTT NULL");
        return;
    }
    
    char status_payload[512];
    time_t now = time(NULL);
    char time_str[64];
    ntp_get_time_string(time_str, sizeof(time_str));
    
    snprintf(status_payload, sizeof(status_payload),
            "{"
            "\"status\":\"online\","
            "\"read_period_minutes\":%d,"
            "\"solenoid_state\":%s,"
            "\"solenoid_enabled\":%s,"
            "\"uptime_seconds\":%lld,"
            "\"timestamp\":%lld,"
            "\"datetime\":\"%s\""
            "}",
            system_config.read_period_minutes,
            solenoid_get_state() ? "true" : "false",
            system_config.solenoid_enabled ? "true" : "false",
            (long long)(now),
            (long long)(now * 1000),
            time_str);
    
    esp_mqtt_client_publish(client, "esp32/status", status_payload, 0, 1, 0);
    ESP_LOGI(TAG, "Status do sistema publicado");
}

void system_commands_mqtt_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    if (event_id == MQTT_EVENT_DATA) {
        char topic[128] = {0};
        snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
        
        // Verifica se é o tópico de comandos
        if (strncmp(topic, TOPIC_SYSTEM_COMMANDS, strlen(TOPIC_SYSTEM_COMMANDS)) == 0) {
            ESP_LOGI(TAG, "═══════════════════════════════════════");
            ESP_LOGI(TAG, "📥 Comando recebido: %s", topic);
            
            char data[512] = {0};
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);
            ESP_LOGI(TAG, "Payload: %s", data);
            
            // Cliente MQTT do evento
            esp_mqtt_client_handle_t client = event->client;
            
            // ========== COMANDO: Ligar Solenoide ==========
            if (strstr(data, "\"command\":\"solenoid_on\"") != NULL) {
                ESP_LOGI(TAG, "🔧 Comando: LIGAR SOLENOIDE");
                solenoid_set_state(true);
                system_config.solenoid_enabled = true;
                system_commands_publish_status(client);
            }
            // ========== COMANDO: Desligar Solenoide ==========
            else if (strstr(data, "\"command\":\"solenoid_off\"") != NULL) {
                ESP_LOGI(TAG, "🔧 Comando: DESLIGAR SOLENOIDE");
                solenoid_set_state(false);
                system_config.solenoid_enabled = false;
                system_commands_publish_status(client);
            }
            // ========== COMANDO: Publicar Todos os Dados ==========
            else if (strstr(data, "\"command\":\"publish_all\"") != NULL) {
                ESP_LOGI(TAG, "🔧 Comando: PUBLICAR TODOS OS DADOS");
                system_commands_publish_all_data(client);
            }
            // ========== COMANDO: Alterar Período de Leitura ==========
            else if (strstr(data, "\"command\":\"set_read_period\"") != NULL) {
                // Parse do campo "minutes"
                char *minutes_str = strstr(data, "\"minutes\":");
                if (minutes_str != NULL) {
                    int minutes = 0;
                    sscanf(minutes_str, "\"minutes\":%d", &minutes);
                    
                    ESP_LOGI(TAG, "🔧 Comando: ALTERAR PERÍODO DE LEITURA");
                    ESP_LOGI(TAG, "Novo período: %d minutos", minutes);
                    
                    system_commands_set_read_period_minutes(minutes);
                    system_commands_publish_status(client);
                } else {
                    ESP_LOGW(TAG, "❌ Campo 'minutes' não encontrado");
                }
            }
            // ========== COMANDO: Solicitar Status ==========
            else if (strstr(data, "\"command\":\"get_status\"") != NULL) {
                ESP_LOGI(TAG, "🔧 Comando: SOLICITAR STATUS");
                system_commands_publish_status(client);
            }
            // ========== COMANDO: Reiniciar ESP32 ==========
            else if (strstr(data, "\"command\":\"restart\"") != NULL) {
                ESP_LOGI(TAG, "🔧 Comando: REINICIAR ESP32");
                ESP_LOGW(TAG, "Reiniciando em 3 segundos...");
                
                char ack[128];
                snprintf(ack, sizeof(ack), "{\"message\":\"Restarting ESP32 in 3 seconds...\"}");
                esp_mqtt_client_publish(client, "esp32/status", ack, 0, 1, 0);
                
                vTaskDelay(pdMS_TO_TICKS(3000));
                esp_restart();
            }
            // ========== COMANDO DESCONHECIDO ==========
            else {
                ESP_LOGW(TAG, "❌ Comando não reconhecido");
                ESP_LOGI(TAG, "Comandos disponíveis:");
                ESP_LOGI(TAG, "  - {\"command\":\"solenoid_on\"}");
                ESP_LOGI(TAG, "  - {\"command\":\"solenoid_off\"}");
                ESP_LOGI(TAG, "  - {\"command\":\"publish_all\"}");
                ESP_LOGI(TAG, "  - {\"command\":\"set_read_period\",\"minutes\":10}");
                ESP_LOGI(TAG, "  - {\"command\":\"get_status\"}");
                ESP_LOGI(TAG, "  - {\"command\":\"restart\"}");
            }
            
            ESP_LOGI(TAG, "═══════════════════════════════════════");
        }
    }
}
