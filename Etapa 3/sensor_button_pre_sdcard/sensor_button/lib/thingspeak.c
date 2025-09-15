#include "thingspeak.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "wifi_task.h"
#include <string.h>
#include <stdio.h>
#include "buffer_manager.h"

static int dns_fail_count = 0;   // contador de falhas
bool thingspeak_dns_failed = false; // flag de falha (visível no main)
QueueHandle_t thingspeak_queue = NULL;

// Função para fechar a conexão TCP e limpar o estado
static void http_request_close(HTTP_REQUEST_STATE *state) {
    if (state->tcp_pcb) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        tcp_close(state->tcp_pcb);
        state->tcp_pcb = NULL;
    }
    state->complete = true;
    free(state);
}

// Callback: chamado quando os dados são recebidos do servidor
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    HTTP_REQUEST_STATE *state = (HTTP_REQUEST_STATE *)arg;
    if (!p) {
        // Se p for NULL, a conexão foi fechada pelo servidor
        http_request_close(state);
        return ERR_OK;
    }

    // Indica que recebemos os dados e libera o buffer
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    
    // Para o ThingSpeak, não precisamos processar a resposta, apenas fechar
    http_request_close(state);
    return ERR_OK;
}

// Callback: chamado após o envio dos dados ser confirmado (ACK)
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    // Podemos ignorar, pois a lógica principal está no recebimento da resposta
    return ERR_OK;
}

// Callback: chamado em caso de erro na conexão TCP
static void http_err_callback(void *arg, err_t err) {
    HTTP_REQUEST_STATE *state = (HTTP_REQUEST_STATE *)arg;
    printf("TCP error callback: %d\n", err);
    http_request_close(state);
}

// Callback: chamado quando a conexão TCP é estabelecida com sucesso
static err_t http_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    HTTP_REQUEST_STATE *state = (HTTP_REQUEST_STATE *)arg;
    if (err != ERR_OK) {
        printf("Falha na conexao: %d\n", err);
        http_request_close(state);
        return err;
    }

    printf("Conectado ao servidor. Enviando requisicao HTTP...\n");
    
    // Configura os callbacks para a conexão
    tcp_recv(tpcb, http_recv_callback);
    tcp_sent(tpcb, http_sent_callback);
    tcp_err(tpcb, http_err_callback);

    // Escreve a requisição HTTP no buffer de envio
    err = tcp_write(tpcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Falha ao escrever no buffer TCP: %d\n", err);
        http_request_close(state);
        return err;
    }

    // Envia os dados
    err = tcp_output(tpcb);
    if (err != ERR_OK) {
        printf("Falha ao enviar dados TCP: %d\n", err);
        http_request_close(state);
        return err;
    }

    return ERR_OK;
}

// Inicia a conexão TCP
static bool http_open_connection(HTTP_REQUEST_STATE *state) {
    cyw43_arch_lwip_begin();
    
    // 
    state->tcp_pcb = tcp_new();
    
    cyw43_arch_lwip_end();

    if (!state->tcp_pcb) {
        printf("Falha ao criar PCB TCP\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    
    // Tenta conectar ao servidor na porta 80 (HTTP)
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, 80, http_connect_callback);
    
    return err == ERR_OK;
}

// 
static void thingspeak_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    HTTP_REQUEST_STATE *state = (HTTP_REQUEST_STATE *)callback_arg;
    if (ipaddr) {
        state->remote_addr = *ipaddr;
        dns_fail_count = 0;  // zera contador ao resolver com sucesso
        thingspeak_dns_failed = false;
        printf("IP do servidor encontrado: %s\n", ipaddr_ntoa(ipaddr));
        if (!http_open_connection(state)) {
            http_request_close(state);
        }
    } else {
        dns_fail_count++;
        printf("Falha na resolucao do DNS (tentativa %d)\n", dns_fail_count);
        
        if (dns_fail_count >= 3) {
            printf("DNS falhou 3 vezes. Ativando flag de falha.\n");
            thingspeak_dns_failed = true;
        }
        
        http_request_close(state);
    }
}

// Prepara e inicia o processo de envio dos dados
void thingspeak_send(temp_record_t record) {
    HTTP_REQUEST_STATE *state = calloc(1, sizeof(HTTP_REQUEST_STATE));
    if (!state) {
        printf("Falha ao alocar estado\n");
        return;
    }

char timestamp_str[25]; 
snprintf(timestamp_str, sizeof(timestamp_str),
         "%04d-%02d-%02dT%02d:%02d:%02dZ",  // Formato ISO8601 com 'T' e 'Z'
         record.timestamp.year, record.timestamp.month, record.timestamp.day,
         record.timestamp.hour, record.timestamp.min, record.timestamp.sec);

snprintf(state->http_request, sizeof(state->http_request),
         "GET /update?api_key=%s&field1=%.2f&created_at=%s HTTP/1.1\r\n"
         "Host: %s\r\n"
         "Connection: close\r\n\r\n",
         THINGSPEAK_API_KEY, record.temperature, timestamp_str, THINGSPEAK_HOST);
    
    cyw43_arch_lwip_begin();
    
    err_t err = dns_gethostbyname(THINGSPEAK_HOST, &state->remote_addr, thingspeak_dns_found_cb, state);
    
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        if (!http_open_connection(state)) {
            http_request_close(state);
        }
    } else if (err != ERR_INPROGRESS) {
        printf("Erro imediato no DNS: %d\n", err);
        http_request_close(state);
    }
}

// Task que processa envios ao ThingSpeak
static void thingspeak_task(void *pvParameters) {
    temp_record_t record_to_send;

    for (;;) {
        // Espera por um registro completo na fila
        if (xQueueReceive(thingspeak_queue, &record_to_send, portMAX_DELAY)) {
            if (wifi_connected && !thingspeak_dns_failed) {
                
                /*******************************************************************/
                /* INÍCIO DA ALTERAÇÃO                                             */
                /*******************************************************************/

                // Log agora inclui os segundos e usa a hora correta do RTC, sem subtrair.
                printf("[ThingSpeak] Enviando registro: %.2f C de %02d/%02d %02d:%02d:%02d\n", 
                       record_to_send.temperature, record_to_send.timestamp.day, 
                       record_to_send.timestamp.month, record_to_send.timestamp.hour,
                       record_to_send.timestamp.min, record_to_send.timestamp.sec);

                /*******************************************************************/
                /* FIM DA ALTERAÇÃO                                                */
                /*******************************************************************/
                
                thingspeak_send(record_to_send);
            } else {
                printf("[ThingSpeak] WiFi ou DNS indisponivel, nao enviando\n");
            }
        }
    }
}

// Função para enviar múltiplos dados ao ThingSpeak (em lote)
void thingspeak_send_batch(temp_record_t* records, int count) {
    if (count == 0) return;
    
    printf("[ThingSpeak] Enviando lote de %d registros (1 a cada 15s)\n", count);
    
    for (int i = 0; i < count; i++) {
        if (wifi_connected && !thingspeak_dns_failed) {
            printf("[ThingSpeak] Enviando registro %d/%d: %.2f C\n", 
                   i + 1, count, records[i].temperature);
            
            thingspeak_send(records[i]);
            
            // Respeita limite de 15 segundos do ThingSpeak free
            vTaskDelay(pdMS_TO_TICKS(15000));
            
        } else {
            printf("[ThingSpeak] WiFi ou DNS indisponivel, quebrando o lote.\n");
            break;
        }
    }
    
    printf("[ThingSpeak] Lote enviado completo\n");
}

// Inicializa fila + task
void thingspeak_task_init(void) {
    thingspeak_queue = xQueueCreate(10, sizeof(temp_record_t));
    if (!thingspeak_queue) {
        printf("Falha ao criar fila do ThingSpeak!\n");
        return;
    }

    xTaskCreate(
        thingspeak_task,
        "ThingSpeakTask",
        4096,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );
}