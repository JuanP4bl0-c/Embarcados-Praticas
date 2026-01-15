#include "power_manager.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "POWER_MGR";

// Configuração global
static power_config_t power_config = {
    .mode = POWER_MODE_AUTO,
    .enabled = false,
    .sleep_threshold_ms = 600000  // 600 segundos (10 minutos)
};

// Estatísticas
static struct {
    uint32_t total_sleep_count;
    uint64_t total_sleep_time_ms;
    uint32_t wake_by_timer_count;
} stats = {0};

// Flags de sincronização de publicação dos sensores
static struct {
    bool dht11_published;
    bool uv_published;
    bool soil_published;
} publish_flags = {false, false, false};

void power_manager_init(void)
{
    ESP_LOGI(TAG, "Inicializando Power Management...");
    
    // Configura Power Management para permitir light sleep automático
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,      // Frequência máxima
        .min_freq_mhz = 80,       // Frequência mínima em idle
        .light_sleep_enable = true // Habilita light sleep automático
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Power Management configurado (Light Sleep habilitado)");
    } else {
        ESP_LOGW(TAG, "Falha ao configurar Power Management: %d", ret);
    }
    
    // Configura WiFi para manter conexão durante light sleep
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    
    ESP_LOGI(TAG, "Modo: %s", 
             power_config.mode == POWER_MODE_AUTO ? "AUTO" :
             power_config.mode == POWER_MODE_LIGHT_SLEEP ? "LIGHT_SLEEP" : "NORMAL");
    ESP_LOGI(TAG, "Sleep threshold: %lu ms", power_config.sleep_threshold_ms);
    ESP_LOGI(TAG, "Estado: %s", power_config.enabled ? "HABILITADO" : "DESABILITADO");
}

void power_manager_set_enabled(bool enabled)
{
    power_config.enabled = enabled;
    ESP_LOGI(TAG, "Power management %s", enabled ? "HABILITADO" : "DESABILITADO");
}

void power_manager_set_mode(power_mode_t mode)
{
    power_config.mode = mode;
    ESP_LOGI(TAG, "Modo alterado para: %s",
             mode == POWER_MODE_AUTO ? "AUTO" :
             mode == POWER_MODE_LIGHT_SLEEP ? "LIGHT_SLEEP" : "NORMAL");
}

power_config_t power_manager_get_config(void)
{
    return power_config;
}

bool power_manager_should_sleep(uint32_t next_read_period_ms)
{
    if (!power_config.enabled) {
        return false;
    }
    
    if (power_config.mode == POWER_MODE_NORMAL) {
        return false;
    }
    
    if (power_config.mode == POWER_MODE_LIGHT_SLEEP) {
        return true;
    }
    
    // Modo AUTO: decide baseado no período
    return next_read_period_ms >= power_config.sleep_threshold_ms;
}

void power_manager_mark_sensor_published(const char *sensor_name)
{
    if (strcmp(sensor_name, "dht11") == 0) {
        publish_flags.dht11_published = true;
        ESP_LOGD(TAG, "DHT11 marcado como publicado");
    } else if (strcmp(sensor_name, "uv") == 0) {
        publish_flags.uv_published = true;
        ESP_LOGD(TAG, "UV marcado como publicado");
    } else if (strcmp(sensor_name, "soil") == 0) {
        publish_flags.soil_published = true;
        ESP_LOGD(TAG, "Soil marcado como publicado");
    }
    
    // Log se todos já publicaram
    if (power_manager_all_sensors_published()) {
        ESP_LOGI(TAG, "Todos os sensores publicaram - pronto para sleep");
    }
}

bool power_manager_all_sensors_published(void)
{
    return publish_flags.dht11_published && 
           publish_flags.uv_published && 
           publish_flags.soil_published;
}

void power_manager_reset_publish_flags(void)
{
    publish_flags.dht11_published = false;
    publish_flags.uv_published = false;
    publish_flags.soil_published = false;
    ESP_LOGD(TAG, "Flags de publicação resetados");
}

void power_manager_sleep(uint32_t duration_ms)
{
    if (!power_config.enabled || duration_ms < 1000) {
        // Se desabilitado ou sleep muito curto, usa delay normal
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }
    
    // Aguarda até que todos os sensores tenham publicado
    int wait_count = 0;
    const int max_wait = 30; // Máximo 15 segundos (30 * 500ms)
    
    while (!power_manager_all_sensors_published() && wait_count < max_wait) {
        ESP_LOGI(TAG, "Aguardando sensores publicarem... (DHT11:%d UV:%d Soil:%d)",
                 publish_flags.dht11_published,
                 publish_flags.uv_published,
                 publish_flags.soil_published);
        vTaskDelay(pdMS_TO_TICKS(500));
        wait_count++;
    }
    
    if (!power_manager_all_sensors_published()) {
        ESP_LOGW(TAG, "Timeout aguardando sensores - entrando em sleep mesmo assim");
    } else {
        ESP_LOGI(TAG, "Todos sensores prontos - iniciando sleep");
    }
    
    // Aguarda um pouco para garantir que as mensagens MQTT foram enviadas
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Reseta flags para o próximo ciclo ANTES de dormir
    power_manager_reset_publish_flags();
    
    // Ajusta duração para compensar o delay de 1s
    uint32_t adjusted_duration_ms = duration_ms > 1000 ? duration_ms - 1000 : duration_ms;
    
    // Calcula tempo de sleep em microssegundos
    uint64_t sleep_time_us = (uint64_t)adjusted_duration_ms * 1000;
    
    ESP_LOGI(TAG, "Entrando em Light Sleep por %lu ms...", adjusted_duration_ms);
    
    // Configura timer para acordar
    esp_sleep_enable_timer_wakeup(sleep_time_us);
    
    // IMPORTANTE: Light sleep profundo bloqueia WiFi
    // Para receber MQTT, usa vTaskDelay em vez de esp_light_sleep_start
    // O ESP-IDF já gerencia economia com esp_pm_configure
    
    // Registra início do sleep
    int64_t sleep_start = esp_timer_get_time();
    
    // USA DELAY COM POWER MANAGEMENT AUTOMÁTICO
    // O WiFi permanece ativo e pode receber MQTT
    vTaskDelay(pdMS_TO_TICKS(adjusted_duration_ms));
    
    // Calcula tempo real em idle
    int64_t sleep_end = esp_timer_get_time();
    uint32_t actual_sleep_ms = (sleep_end - sleep_start) / 1000;
    
    // Atualiza estatísticas
    stats.total_sleep_count++;
    stats.total_sleep_time_ms += actual_sleep_ms;
    stats.wake_by_timer_count++; // Com vTaskDelay sempre acorda por timer
    
    ESP_LOGI(TAG, "Periodo de economia concluido (%lu ms) - WiFi ativo durante todo tempo", actual_sleep_ms);
}

void power_manager_report_stats(void)
{
    ESP_LOGI(TAG, "════════════════════════════════════════");
    ESP_LOGI(TAG, "     ESTATÍSTICAS DE ECONOMIA DE ENERGIA");
    ESP_LOGI(TAG, "════════════════════════════════════════");
    ESP_LOGI(TAG, "Total de sleeps: %lu", stats.total_sleep_count);
    ESP_LOGI(TAG, "Tempo total dormindo: %llu ms (%.2f min)", 
             stats.total_sleep_time_ms, 
             stats.total_sleep_time_ms / 60000.0);
    ESP_LOGI(TAG, "Wake-ups por timer: %lu", stats.wake_by_timer_count);
    ESP_LOGI(TAG, "Wake-ups por evento: %lu", 
             stats.total_sleep_count - stats.wake_by_timer_count);
    
    if (stats.total_sleep_count > 0) {
        uint32_t avg_sleep_ms = stats.total_sleep_time_ms / stats.total_sleep_count;
        ESP_LOGI(TAG, "Média de sleep: %lu ms", avg_sleep_ms);
    }
    
    ESP_LOGI(TAG, "Modo atual: %s", 
             power_config.mode == POWER_MODE_AUTO ? "AUTO" :
             power_config.mode == POWER_MODE_LIGHT_SLEEP ? "LIGHT_SLEEP" : "NORMAL");
    ESP_LOGI(TAG, "Estado: %s", power_config.enabled ? "HABILITADO" : "DESABILITADO");
    ESP_LOGI(TAG, "════════════════════════════════════════");
}
