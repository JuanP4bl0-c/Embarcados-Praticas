#include "plant_config.h"
#include "esp_log.h"
#include "solenoid.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "PLANT_CONFIG";

static plant_config_t plant_config = {
    .temperature_min = 18,        // 18°C mínimo
    .temperature_max = 28,        // 28°C máximo
    .humidity_min = 60,           // 60% umidade do ar mínima
    .humidity_max = 80,           // 80% umidade do ar máxima
    .soil_moisture_min = 60,      // 60% umidade do solo mínima
    .soil_moisture_max = 80,      // 80% umidade do solo máxima
    .uv_min = 30,                 // 30% exposição mínima
    .uv_max = 70,                 // 70% exposição máxima
    .irrigation_threshold = 25,   // Irriga se 25% abaixo do mínimo
    .auto_irrigation = true       // Irrigação automática habilitada
};

void plant_config_init(void){

    ESP_LOGI(TAG, "═══════════════════════════════════════════════");
    ESP_LOGI(TAG, "   🌱 Configuração Ideal da Planta (Tomate) 🌱 ");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════");
    ESP_LOGI(TAG, "🌡️  Temperatura: %d°C - %d°C",plant_config.temperature_min, plant_config.temperature_max);
    ESP_LOGI(TAG, "💨 Umidade Ar:  %d%% - %d%%",plant_config.humidity_min, plant_config.humidity_max);
    ESP_LOGI(TAG, "💧 Umidade Solo: %d%% - %d%%",plant_config.soil_moisture_min, plant_config.soil_moisture_max);
    ESP_LOGI(TAG, "☀️  Exposição UV: %d%% - %d%%",plant_config.uv_min, plant_config.uv_max);
    ESP_LOGI(TAG, "🚰 Irrigação automática: %s (limiar: -%d%%)",plant_config.auto_irrigation ? "ATIVADA" : "DESATIVADA",plant_config.irrigation_threshold);
    ESP_LOGI(TAG, "═══════════════════════════════════════════════");
}

plant_config_t* plant_config_get(void){return &plant_config;}

bool plant_config_should_irrigate(int current_moisture){
    if (!plant_config.auto_irrigation) {
        return false;
    }
    
    // Calcula o limiar de irrigação
    int threshold = plant_config.soil_moisture_min - plant_config.irrigation_threshold;
    
    if (current_moisture < threshold) {
        ESP_LOGW(TAG, "   IRRIGAÇÃO NECESSÁRIA!");
        ESP_LOGW(TAG, "   Umidade atual: %d%%", current_moisture);
        ESP_LOGW(TAG, "   Limiar: %d%% (ideal: %d%% - %d%%)", 
                 threshold, plant_config.soil_moisture_min, plant_config.irrigation_threshold);
        return true;
    }
    
    return false;
}

bool plant_config_check_parameters(int temp, int humidity, int soil_moisture,int uv, char *alert_msg){
    bool all_ok = true;
    char temp_msg[256] = {0};
    
    strcpy(alert_msg, "{\"alerts\":[");
    
    // Verifica temperatura
    if (temp < plant_config.temperature_min) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"temp_low\",\"value\":%d,\"min\":%d},", 
                 temp, plant_config.temperature_min);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    } else if (temp > plant_config.temperature_max) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"temp_high\",\"value\":%d,\"max\":%d},", 
                 temp, plant_config.temperature_max);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    }
    
    // Verifica umidade do ar
    if (humidity < plant_config.humidity_min) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"humidity_low\",\"value\":%d,\"min\":%d},", 
                 humidity, plant_config.humidity_min);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    } else if (humidity > plant_config.humidity_max) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"humidity_high\",\"value\":%d,\"max\":%d},", 
                 humidity, plant_config.humidity_max);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    }
    
    // Verifica umidade do solo
    if (soil_moisture < plant_config.soil_moisture_min) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"soil_low\",\"value\":%d,\"min\":%d},", 
                 soil_moisture, plant_config.soil_moisture_min);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    } else if (soil_moisture > plant_config.soil_moisture_max) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"soil_high\",\"value\":%d,\"max\":%d},", 
                 soil_moisture, plant_config.soil_moisture_max);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    }
    
    // Verifica UV
    if (uv < plant_config.uv_min) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"uv_low\",\"value\":%d,\"min\":%d},", 
                 uv, plant_config.uv_min);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    } else if (uv > plant_config.uv_max) {
        snprintf(temp_msg, sizeof(temp_msg), 
                 "{\"type\":\"uv_high\",\"value\":%d,\"max\":%d},", 
                 uv, plant_config.uv_max);
        strcat(alert_msg, temp_msg);
        all_ok = false;
    }
    
    // Remove última vírgula e fecha JSON
    int len = strlen(alert_msg);
    if (alert_msg[len-1] == ',') {
        alert_msg[len-1] = ']';
        strcat(alert_msg, "}");
    } else {
        strcat(alert_msg, "]}");
    }
    
    return all_ok;
}

esp_err_t plant_config_update_from_json(const char *json_data){
    ESP_LOGI(TAG, "Atualizando configuração da planta...");
    ESP_LOGI(TAG, "JSON: %s", json_data);
    
    // Parse simples de JSON
    int temp_min, temp_max, hum_min, hum_max;
    int soil_min, soil_max, uv_min, uv_max;
    int threshold;
    
    // Tenta extrair os valores
    bool updated = false;
    const char *ptr = NULL;
    
    // Verifica auto_irrigation primeiro (booleano)
    if (strstr(json_data, "\"auto_irrigation\":true") != NULL || 
        strstr(json_data, "\"auto_irrigation\": true") != NULL) {
        plant_config.auto_irrigation = true;
        updated = true;
        ESP_LOGI(TAG, "auto_irrigation = true");
    } else if (strstr(json_data, "\"auto_irrigation\":false") != NULL ||
               strstr(json_data, "\"auto_irrigation\": false") != NULL) {
        plant_config.auto_irrigation = false;
        updated = true;
        ESP_LOGI(TAG, "auto_irrigation = false");
    }
    
    // Temperatura mínima
    ptr = strstr(json_data, "\"temperature_min\":");
    if (ptr != NULL && sscanf(ptr, "\"temperature_min\":%d", &temp_min) == 1) {
        plant_config.temperature_min = temp_min;
        updated = true;
        ESP_LOGI(TAG, "temperature_min = %d", temp_min);
    }
    
    // Temperatura máxima
    ptr = strstr(json_data, "\"temperature_max\":");
    if (ptr != NULL && sscanf(ptr, "\"temperature_max\":%d", &temp_max) == 1) {
        plant_config.temperature_max = temp_max;
        updated = true;
        ESP_LOGI(TAG, "temperature_max = %d", temp_max);
    }
    
    // Umidade mínima
    ptr = strstr(json_data, "\"humidity_min\":");
    if (ptr != NULL && sscanf(ptr, "\"humidity_min\":%d", &hum_min) == 1) {
        plant_config.humidity_min = hum_min;
        updated = true;
        ESP_LOGI(TAG, "humidity_min = %d", hum_min);
    }
    
    // Umidade máxima
    ptr = strstr(json_data, "\"humidity_max\":");
    if (ptr != NULL && sscanf(ptr, "\"humidity_max\":%d", &hum_max) == 1) {
        plant_config.humidity_max = hum_max;
        updated = true;
        ESP_LOGI(TAG, "humidity_max = %d", hum_max);
    }
    
    // Umidade do solo mínima
    ptr = strstr(json_data, "\"soil_moisture_min\":");
    if (ptr != NULL && sscanf(ptr, "\"soil_moisture_min\":%d", &soil_min) == 1) {
        plant_config.soil_moisture_min = soil_min;
        updated = true;
        ESP_LOGI(TAG, "soil_moisture_min = %d", soil_min);
    }
    
    // Umidade do solo máxima
    ptr = strstr(json_data, "\"soil_moisture_max\":");
    if (ptr != NULL && sscanf(ptr, "\"soil_moisture_max\":%d", &soil_max) == 1) {
        plant_config.soil_moisture_max = soil_max;
        updated = true;
        ESP_LOGI(TAG, "soil_moisture_max = %d", soil_max);
    }
    
    // UV mínimo
    ptr = strstr(json_data, "\"uv_min\":");
    if (ptr != NULL && sscanf(ptr, "\"uv_min\":%d", &uv_min) == 1) {
        plant_config.uv_min = uv_min;
        updated = true;
        ESP_LOGI(TAG, "uv_min = %d", uv_min);
    }
    
    // UV máximo
    ptr = strstr(json_data, "\"uv_max\":");
    if (ptr != NULL && sscanf(ptr, "\"uv_max\":%d", &uv_max) == 1) {
        plant_config.uv_max = uv_max;
        updated = true;
        ESP_LOGI(TAG, "uv_max = %d", uv_max);
    }
    
    // Limiar de irrigação
    ptr = strstr(json_data, "\"irrigation_threshold\":");
    if (ptr != NULL && sscanf(ptr, "\"irrigation_threshold\":%d", &threshold) == 1) {
        plant_config.irrigation_threshold = threshold;
        updated = true;
        ESP_LOGI(TAG, "irrigation_threshold = %d", threshold);
    }
    
    if (updated) {
        ESP_LOGI(TAG, "Configuração atualizada com sucesso!");
        plant_config_init(); // Mostra nova configuração
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Nenhum parâmetro válido encontrado no JSON");
        return ESP_FAIL;
    }
}

void plant_config_mqtt_handler(void *handler_args, esp_event_base_t base,int32_t event_id, void *event_data){
    
    esp_mqtt_event_handle_t event = event_data;
    
    if (event_id == MQTT_EVENT_DATA) {
        char topic[128] = {0};
        snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
        
        if (strncmp(topic, TOPIC_PLANT_CONFIG, strlen(TOPIC_PLANT_CONFIG)) == 0) {
            char data[512] = {0};
            snprintf(data, sizeof(data), "%.*s", event->data_len, event->data);
            
            plant_config_update_from_json(data);
        }
    }
}

void plant_config_publish(esp_mqtt_client_handle_t client){
    if (client == NULL) return;
    
    // Obtém timestamp Unix em segundos e converte para milissegundos
    time_t now;
    time(&now);
    int64_t timestamp_ms = (int64_t)now * 1000;
    
    // Obtém horário formatado
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    char message[768];
    snprintf(message, sizeof(message),
        "{\"device_id\":\"ESP32_Client\","
        "\"temperature_min\":%d,\"temperature_max\":%d,"
        "\"humidity_min\":%d,\"humidity_max\":%d,"
        "\"soil_moisture_min\":%d,\"soil_moisture_max\":%d,"
        "\"uv_min\":%d,\"uv_max\":%d,"
        "\"irrigation_threshold\":%d,\"auto_irrigation\":%s,"
        "\"timestamp\":%lld,\"datetime\":\"%s\",\"event\":\"system_init\"}",
        plant_config.temperature_min, plant_config.temperature_max,
        plant_config.humidity_min, plant_config.humidity_max,
        plant_config.soil_moisture_min, plant_config.soil_moisture_max,
        plant_config.uv_min, plant_config.uv_max,
        plant_config.irrigation_threshold,
        plant_config.auto_irrigation ? "true" : "false",
        timestamp_ms, time_str);
    
    esp_mqtt_client_publish(client, TOPIC_PLANT_CONFIG, message, 0, 1, 0);
    ESP_LOGI(TAG, "Configuração publicada [%s]", time_str);
}
