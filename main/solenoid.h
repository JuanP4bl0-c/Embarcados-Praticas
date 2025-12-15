#ifndef SOLENOID_H
#define SOLENOID_H

#include "esp_err.h"
#include "mqtt_client.h"
#include <stdbool.h>

// Configurações do solenoide
#define SOLENOID_GPIO 26  // GPIO digital - Lado direito da placa
#define TOPIC_SOLENOID "esp32/solenoid"

/**
 * @brief Inicializa o solenoide
 * @return ESP_OK em caso de sucesso
 */
esp_err_t solenoid_init(void);

/**
 * @brief Define o estado do solenoide
 * @param state true para ligar, false para desligar
 * @return ESP_OK em caso de sucesso
 */
esp_err_t solenoid_set_state(bool state);

/**
 * @brief Controla o solenoide (wrapper para set_state)
 * @param state true para ligar, false para desligar
 */
void solenoid_control(bool state);

/**
 * @brief Obtém o estado atual do solenoide
 * @return true se ligado, false se desligado
 */
bool solenoid_get_state(void);

/**
 * @brief Handler customizado para processar comandos MQTT do solenoide
 * @param handler_args Argumentos do handler
 * @param base Base do evento
 * @param event_id ID do evento
 * @param event_data Dados do evento
 */
void solenoid_mqtt_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif // SOLENOID_H
