/**
 * @file utils.h
 * @brief Declarações de utilitários de rede.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include "lwip/ip_addr.h"

bool utils_resolve_dns(const char *hostname, ip_addr_t *out_ip, uint32_t timeout_ms);

#endif /* UTILS_H */
