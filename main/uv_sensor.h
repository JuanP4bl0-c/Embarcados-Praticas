#ifndef UV_SENSOR_H
#define UV_SENSOR_H

#include "esp_err.h"
#include "mqtt_client.h"

// Configurações do sensor UV
#define UV_SENSOR_GPIO 32  // ADC1_CHANNEL_4 - Lado direito da placa
#define TOPIC_UV_SENSOR "esp32/uv"

/**
 * @brief Inicializa o sensor UV
 * @return ESP_OK em caso de sucesso
 */
esp_err_t uv_sensor_init(void);

/**
 * @brief Lê o valor do sensor UV (0-4095)
 * @param value Ponteiro para armazenar o valor lido
 * @return ESP_OK em caso de sucesso
 */
esp_err_t uv_sensor_read(int *value);

/**
 * @brief Task para publicar dados do sensor UV periodicamente
 * @param pvParameters Parâmetros da task (esp_mqtt_client_handle_t)
 */
void uv_sensor_task(void *pvParameters);

#endif // UV_SENSOR_H
