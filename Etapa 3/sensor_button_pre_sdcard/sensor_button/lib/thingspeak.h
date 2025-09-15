#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "buffer_manager.h" // Já inclui temp_record_t

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

// --- Fila e Task ---
extern QueueHandle_t thingspeak_queue;
void thingspeak_task_init(void);

// MUDANÇA: A função agora aceita a estrutura completa
void thingspeak_send(temp_record_t record);

void thingspeak_send_batch(temp_record_t* records, int count);
#endif // THINGSPEAK_H