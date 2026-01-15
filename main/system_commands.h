#ifndef SYSTEM_COMMANDS_H
#define SYSTEM_COMMANDS_H

#include "mqtt_client.h"
#include "esp_event.h"

// Tópico para comandos do sistema
#define TOPIC_SYSTEM_COMMANDS "esp32/commands"

/**
 * Estrutura de configuração do sistema
 */
typedef struct {
    int read_period_minutes;  // Período de leitura em minutos
    bool solenoid_enabled;    // Solenoide habilitado ou não
} system_config_t;

/**
 * Inicializa o módulo de comandos do sistema
 */
void system_commands_init(void);

/**
 * Handler MQTT para processar comandos do sistema
 */
void system_commands_mqtt_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 * Publica todos os dados dos sensores imediatamente
 */
void system_commands_publish_all_data(esp_mqtt_client_handle_t client);

/**
 * Obtém o período de leitura atual em milissegundos
 */
int system_commands_get_read_period_ms(void);

/**
 * Define o período de leitura em minutos
 */
void system_commands_set_read_period_minutes(int minutes);

/**
 * Publica status do sistema
 */
void system_commands_publish_status(esp_mqtt_client_handle_t client);

#endif // SYSTEM_COMMANDS_H
