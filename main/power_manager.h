#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Modos de economia de energia
 */
typedef enum {
    POWER_MODE_NORMAL,      // Modo normal (sem economia)
    POWER_MODE_LIGHT_SLEEP, // Light sleep (WiFi ativo, acorda por timer/MQTT)
    POWER_MODE_AUTO         // Automático (decide baseado no período de leitura)
} power_mode_t;

/**
 * Configuração de power management
 */
typedef struct {
    power_mode_t mode;           // Modo atual
    bool enabled;                // Power management habilitado
    uint32_t sleep_threshold_ms; // Mínimo de tempo entre leituras para ativar sleep (padrão: 60s)
} power_config_t;

/**
 * Inicializa o power management
 */
void power_manager_init(void);

/**
 * Habilita/desabilita economia de energia
 */
void power_manager_set_enabled(bool enabled);

/**
 * Define o modo de economia de energia
 */
void power_manager_set_mode(power_mode_t mode);

/**
 * Obtém configuração atual
 */
power_config_t power_manager_get_config(void);

/**
 * Entra em sleep por um período (ms)
 * Retorna quando acordar (por timer ou evento externo)
 */
void power_manager_sleep(uint32_t duration_ms);

/**
 * Verifica se deve entrar em sleep baseado no período de leitura
 */
bool power_manager_should_sleep(uint32_t next_read_period_ms);

/**
 * Marca que um sensor publicou seus dados
 * @param sensor_name Nome do sensor ("dht11", "uv", "soil")
 */
void power_manager_mark_sensor_published(const char *sensor_name);

/**
 * Verifica se todos os sensores já publicaram neste ciclo
 */
bool power_manager_all_sensors_published(void);

/**
 * Reseta os flags de publicação para o próximo ciclo
 */
void power_manager_reset_publish_flags(void);

/**
 * Reporta estatísticas de economia de energia
 */
void power_manager_report_stats(void);

#endif // POWER_MANAGER_H
