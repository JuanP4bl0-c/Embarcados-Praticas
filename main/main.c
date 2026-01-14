#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "esp_adc/adc_oneshot.h"
#include "day_night_control.h"

// Módulos dos sensores e atuadores
#include "uv_sensor.h"
#include "soil_moisture.h"
#include "dht11_sensor.h"
#include "solenoid.h"
#include "plant_config.h"
#include "ntp_sync.h"
#include "system_commands.h"
#include "power_manager.h"


// #define WIFI_SSID "UFC_QUIXADA"

#define WIFI_SSID "UFC_B4_SL3_1"
#define WIFI_PASS ""

// #define WIFI_SSID "brisa-2504280"
// #define WIFI_PASS "eubcidpn"

#define AWS_IOT_ENDPOINT "a1gqpq2oiyi1r1-ats.iot.us-east-1.amazonaws.com"
#define AWS_IOT_CLIENT_ID "ESP32_Client"


// LED embutido da ESP32
#define BUILTIN_LED_GPIO GPIO_NUM_2  // GPIO2 - Pisca a cada log
// LED externo - Status do sistema
#define EXTERNAL_LED_GPIO GPIO_NUM_4  // GPIO4 - Aceso = Sistema pronto (WiFi + AWS + Internet)

static const char *TAG = "APP_MAIN";

// Função de callback customizado para vprintf (intercepta todos os logs)
static int custom_vprintf(const char *fmt, va_list args)
{
    // Chama o printf original
    int ret = vprintf(fmt, args);
    
    // Pisca APENAS LED embutido a cada log
    gpio_set_level(BUILTIN_LED_GPIO, 1);   // Liga LED embutido
    vTaskDelay(pdMS_TO_TICKS(25));         // Mantém por 25ms
    gpio_set_level(BUILTIN_LED_GPIO, 0);   // Desliga LED embutido
    
    // LED externo permanece no estado de status (não pisca)
    
    return ret;
}

adc_oneshot_unit_handle_t adc1_handle = NULL;

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t device_certificate_pem_crt_start[] asm("_binary_376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f_certificate_pem_crt_start");
extern const uint8_t device_private_pem_key_start[] asm("_binary_376f19f7d489fd831039a918bc7a9ec29a363566a92e0c10b4fc5b0f69aa345f_private_pem_key_start");

esp_mqtt_client_handle_t client = NULL;
extern bool mqtt_connected;

void app_main(void)
{
    // Configura LED embutido
    gpio_reset_pin(BUILTIN_LED_GPIO);
    gpio_set_direction(BUILTIN_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUILTIN_LED_GPIO, 0);  // Inicia apagado
    
    // Configura LED externo (D4 / GPIO4)
    gpio_reset_pin(EXTERNAL_LED_GPIO);
    gpio_set_direction(EXTERNAL_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(EXTERNAL_LED_GPIO, 0);  // Inicia apagado
    
    // Registra função customizada para piscar LEDs a cada log
    esp_log_set_vprintf(custom_vprintf);
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash inicializado");

    // LED externo piscando = conectando WiFi
    ESP_LOGI(TAG, "Conectando ao WiFi: %s", WIFI_SSID);
    gpio_set_level(EXTERNAL_LED_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(EXTERNAL_LED_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    wifi_manager_init(WIFI_SSID, WIFI_PASS, WIFI_AUTH_OPEN);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Inicializa e sincroniza o NTP para obter horário real
    ESP_LOGI(TAG, "Inicializando sincronização de horário via NTP...");
    ntp_sync_init();
    if (ntp_wait_sync(10000)) {
        ESP_LOGI(TAG, "Horário sincronizado com sucesso!");
    } else {
        ESP_LOGW(TAG, "Falha ao sincronizar horário, continuando sem NTP");
    }

    // LED externo piscando = conectando AWS IoT
    for (int i = 0; i < 3; i++) {
        gpio_set_level(EXTERNAL_LED_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
        gpio_set_level(EXTERNAL_LED_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    // Registra handlers MQTT customizados
    mqtt_manager_set_custom_handler(solenoid_mqtt_handler);
    mqtt_manager_set_custom_handler(system_commands_mqtt_handler);
    
    mqtt_manager_start(AWS_IOT_ENDPOINT, AWS_IOT_CLIENT_ID, 
                       aws_root_ca_pem_start, 
                       device_certificate_pem_crt_start, 
                       device_private_pem_key_start, 
                       &client);
    vTaskDelay(pdMS_TO_TICKS(3000));

    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc1_handle));
    ESP_LOGI(TAG, "ADC Start");
    
    // Inicializa controle diurno/noturno
    day_night_control_init();
    
    // Inicializa sistema de comandos
    system_commands_init();
    
    // Inicializa gerenciamento de energia
    power_manager_init();
    
    plant_config_init();
    
    uv_sensor_init();
    soil_moisture_init();
    dht11_sensor_init();
    solenoid_init();
    
    ESP_LOGI(TAG, "Todos os dispositivos inicializados");

    if (mqtt_connected && client != NULL) {
        ESP_LOGI(TAG, "subscribe nos tópicos.");
        int msg_id1 = esp_mqtt_client_subscribe(client, TOPIC_SOLENOID, 1);
        int msg_id2 = esp_mqtt_client_subscribe(client, TOPIC_PLANT_CONFIG, 1);
        int msg_id3 = esp_mqtt_client_subscribe(client, TOPIC_SYSTEM_COMMANDS, 1);
        
        ESP_LOGI(TAG, "Subscribed: %s (msg_id=%d)", TOPIC_SOLENOID, msg_id1);
        ESP_LOGI(TAG, "Subscribed: %s (msg_id=%d)", TOPIC_PLANT_CONFIG, msg_id2);
        ESP_LOGI(TAG, "Subscribed: %s (msg_id=%d)", TOPIC_SYSTEM_COMMANDS, msg_id3);
        
        // Publica configuração atual
        plant_config_publish(client);
        
        // Publica status inicial do sistema
        system_commands_publish_status(client);
        
        // SISTEMA PRONTO! Liga LED externo fixo
        ESP_LOGI(TAG, "Sistema 100%% operacional - WiFi + AWS IoT conectados");
        gpio_set_level(EXTERNAL_LED_GPIO, 1);  // LED externo ACESO = Sistema pronto
    } else {
        ESP_LOGW(TAG, "MQTT não conectado, não foi possível fazer subscribe");
        // LED externo piscando rápido = ERRO
        for (int i = 0; i < 10; i++) {
            gpio_set_level(EXTERNAL_LED_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(EXTERNAL_LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }


    // DHT11 no Core 1 (isolado do WiFi/MQTT) com maior prioridade
    xTaskCreatePinnedToCore(dht11_sensor_task, "dht11_task", 4096, (void*)client, 6, NULL, 1);
    // Outras tasks no Core 0 (com WiFi/MQTT)
    xTaskCreatePinnedToCore(uv_sensor_task, "uv_task", 4096, (void*)client, 3, NULL, 0);
    xTaskCreatePinnedToCore(soil_moisture_task, "soil_task", 4096, (void*)client, 3, NULL, 0);

    ESP_LOGI(TAG, "Sistema iniciado com sucesso!");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


