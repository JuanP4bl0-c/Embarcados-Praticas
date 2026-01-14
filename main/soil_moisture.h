#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include "esp_err.h"
#include "mqtt_client.h"

// Configurações do sensor de umidade do solo
#define SOIL_MOISTURE_GPIO 33  // ADC1_CHANNEL_5 - Lado direito da placa
#define TOPIC_SOIL_MOISTURE "esp32/soil_moisture"

/**
 * @brief Inicializa o sensor de umidade do solo
 * @return ESP_OK em caso de sucesso
 */
esp_err_t soil_moisture_init(void);

/**
 * @brief Lê o valor do sensor de umidade do solo (0-4095)
 * @param value Ponteiro para armazenar o valor lido
 * @return ESP_OK em caso de sucesso
 */
esp_err_t soil_moisture_read(int *value);

/**
 * @brief Task para publicar dados do sensor de umidade do solo periodicamente
 * @param pvParameters Parâmetros da task (esp_mqtt_client_handle_t)
 */
void soil_moisture_task(void *pvParameters);

/**
 * @brief Força a publicação imediata dos dados do sensor de umidade do solo
 * @param client Cliente MQTT
 */
void soil_moisture_force_publish(esp_mqtt_client_handle_t client);

#endif // SOIL_MOISTURE_H
