#ifndef DAY_NIGHT_CONTROL_H
#define DAY_NIGHT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Horários de dia/noite (em horas, 0-23)
#define NIGHT_START_HOUR 18  // 18:00 (6 PM)
#define NIGHT_END_HOUR 5     // 05:00 (5 AM)

/**
 * @brief Verifica se é horário noturno
 * 
 * @return true se é noite (18:00 - 05:00), false se é dia
 */
bool is_night_time(void);

/**
 * @brief Obtém a hora atual (0-23)
 * 
 * @return Hora atual ou -1 se não disponível
 */
int get_current_hour(void);

/**
 * @brief Inicializa o controle diurno/noturno
 */
void day_night_control_init(void);

#ifdef __cplusplus
}
#endif

#endif // DAY_NIGHT_CONTROL_H
