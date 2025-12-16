#include "ntp_sync.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/time.h>

static const char *TAG = "NTP_SYNC";

void ntp_sync_init(void)
{
    ESP_LOGI(TAG, "Inicializando sincronização NTP...");
    
    // Configura timezone para Brasília (UTC-3)
    setenv("TZ", "BRT3", 1);
    tzset();
    
    // Configura SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "time.cloudflare.com");
    esp_sntp_init();
    
    ESP_LOGI(TAG, "Servidores NTP configurados");
}

bool ntp_wait_sync(uint32_t timeout_ms)
{
    ESP_LOGI(TAG, "Aguardando sincronização NTP...");
    
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int max_retries = timeout_ms / 500;
    
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < max_retries) {
        ESP_LOGI(TAG, "Tentativa %d/%d...", retry + 1, max_retries);
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
    }
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Verifica se o ano está correto (>2020)
    if (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGE(TAG, "❌ Falha na sincronização NTP (ano=%d)", timeinfo.tm_year + 1900);
        return false;
    }
    
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "✅ NTP sincronizado: %s", strftime_buf);
    
    return true;
}

void ntp_get_time_string(char *buffer, size_t size)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}
