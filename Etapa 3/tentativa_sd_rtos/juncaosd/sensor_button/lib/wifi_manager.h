/**
 * @file wifi_manager.h
 * @brief API de gerenciamento de conexão Wi‑Fi (cyw43).
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

void wifi_manager_init(const char *ssid, const char *pass);
bool wifi_manager_is_connected(void);
bool wifi_manager_wait_connected(uint32_t timeout_ms);
void wifi_manager_force_reconnect(void);
void wifi_manager_task(void *params);
const char *wifi_manager_ip_str(void);
void wifi_manager_init_t(void);

// Variáveis globais de status
extern bool wifi_connected;

#endif /* WIFI_MANAGER_H */
