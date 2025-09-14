#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "pico/cyw43_arch.h"

// --- CONFIGURAÇÕES DO THINGSPEAK ---
#define THINGSPEAK_API_KEY "F1M0J5Y2F0D7GZNA"
#define THINGSPEAK_HOST "api.thingspeak.com"

typedef struct HTTP_REQUEST_STATE_T {
    char http_request[256];
    ip_addr_t remote_addr;
    struct tcp_pcb *tcp_pcb;
    int sent_len;
    bool complete;
} HTTP_REQUEST_STATE;

// Variável global para indicar falhas de DNS
extern bool thingspeak_dns_failed;

// Função para enviar dados ao ThingSpeak
void thingspeak_send(float temperature);

#endif // THINGSPEAK_H