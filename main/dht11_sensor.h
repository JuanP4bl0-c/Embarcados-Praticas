#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include "esp_err.h"
#include "mqtt_client.h"

// Configurações do sensor DHT11
#define DHT11_GPIO 27  // GPIO digital - Lado direito da placa
#define TOPIC_DHT11 "esp32/dht11"

/**
 * @brief Inicializa o sensor DHT11
 * @return ESP_OK em caso de sucesso
 */
esp_err_t dht11_sensor_init(void);

/**
 * @brief Lê temperatura e umidade do sensor DHT11
 * @param humidity Ponteiro para armazenar a umidade (%)
 * @param temperature Ponteiro para armazenar a temperatura (°C)
 * @return ESP_OK em caso de sucesso
 */
esp_err_t dht11_sensor_read(int16_t *humidity, int16_t *temperature);

/**
 * @brief Task para publicar dados do sensor DHT11 periodicamente
 * @param pvParameters Parâmetros da task (esp_mqtt_client_handle_t)
 */
void dht11_sensor_task(void *pvParameters);

#endif // DHT11_SENSOR_H