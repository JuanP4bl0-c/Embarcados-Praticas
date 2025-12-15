#ifndef PLANT_CONFIG_H
#define PLANT_CONFIG_H

#include "esp_err.h"
#include "mqtt_client.h"
#include <stdbool.h>

// Tópico para receber configurações
#define TOPIC_PLANT_CONFIG "esp32/config"
#define TOPIC_ALERTS "esp32/alerts"

// Estrutura de parâmetros ideais da planta
typedef struct {
    int temperature_min;      // Temperatura mínima ideal (°C)
    int temperature_max;      // Temperatura máxima ideal (°C)
    int humidity_min;         // Umidade do ar mínima ideal (%)
    int humidity_max;         // Umidade do ar máxima ideal (%)
    int soil_moisture_min;    // Umidade do solo mínima ideal (%)
    int soil_moisture_max;    // Umidade do solo máxima ideal (%)
    int uv_min;               // Exposição UV mínima ideal (0-100)
    int uv_max;               // Exposição UV máxima ideal (0-100)
    int irrigation_threshold; // Limiar para irrigação automática (% abaixo do ideal)
    bool auto_irrigation;     // Irrigação automática habilitada
} plant_config_t;

/**
 * @brief Inicializa configuração da planta com valores padrão
 */
void plant_config_init(void);

/**
 * @brief Obtém a configuração atual da planta
 * @return Ponteiro para a configuração
 */
plant_config_t* plant_config_get(void);

/**
 * @brief Atualiza a configuração da planta via JSON
 * @param json_data String JSON com novos parâmetros
 * @return ESP_OK em caso de sucesso
 */
esp_err_t plant_config_update_from_json(const char *json_data);

/**
 * @brief Verifica se é necessário irrigar baseado na umidade do solo
 * @param current_moisture Umidade atual do solo (%)
 * @return true se deve irrigar, false caso contrário
 */
bool plant_config_should_irrigate(int current_moisture);

/**
 * @brief Verifica se os parâmetros estão dentro dos limites ideais
 * @param temp Temperatura atual (°C)
 * @param humidity Umidade do ar atual (%)
 * @param soil_moisture Umidade do solo atual (%)
 * @param uv Exposição UV atual (0-100)
 * @param alert_msg Buffer para mensagem de alerta (256 bytes)
 * @return true se tudo está OK, false se há problemas
 */
bool plant_config_check_parameters(int temp, int humidity, int soil_moisture, 
                                   int uv, char *alert_msg);

/**
 * @brief Handler MQTT para receber atualizações de configuração
 */
void plant_config_mqtt_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data);

/**
 * @brief Publica configuração atual no MQTT
 * @param client Handle do cliente MQTT
 */
void plant_config_publish(esp_mqtt_client_handle_t client);

#endif // PLANT_CONFIG_H
