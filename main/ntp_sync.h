#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <time.h>
#include "esp_sntp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa e sincroniza o relógio via NTP
 * 
 * @note Necessário para TLS funcionar corretamente
 */
void ntp_sync_init(void);

/**
 * @brief Aguarda a sincronização NTP completar
 * 
 * @param timeout_ms Timeout em milissegundos (0 = aguarda indefinidamente)
 * @return true se sincronizou, false se timeout
 */
bool ntp_wait_sync(uint32_t timeout_ms);

/**
 * @brief Obtém a hora atual formatada
 * 
 * @param buffer Buffer para armazenar a string
 * @param size Tamanho do buffer
 */
void ntp_get_time_string(char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // NTP_SYNC_H
