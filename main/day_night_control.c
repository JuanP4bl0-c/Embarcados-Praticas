#include "day_night_control.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "DAY_NIGHT";

void day_night_control_init(void)
{
    ESP_LOGI(TAG, "Controle Diurno/Noturno inicializado");
    ESP_LOGI(TAG, "Noite: %02d:00 - %02d:00 (Sensor UV desativado)", 
             NIGHT_START_HOUR, NIGHT_END_HOUR);
    ESP_LOGI(TAG, "Dia: %02d:00 - %02d:00 (Sensor UV ativo)", 
             NIGHT_END_HOUR, NIGHT_START_HOUR);
}

int get_current_hour(void)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Retorna a hora (0-23)
    return timeinfo.tm_hour;
}

bool is_night_time(void)
{
    int hour = get_current_hour();
    
    if (hour < 0) {
        // Hora não disponível, considera como dia por segurança
        return false;
    }
    
    // Noite: 18:00 até 05:00
    // Se hour >= 18 OR hour < 5, é noite
    bool is_night = (hour >= NIGHT_START_HOUR || hour < NIGHT_END_HOUR);
    
    return is_night;
}
