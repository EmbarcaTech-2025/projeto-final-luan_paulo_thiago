#include "thingspeak.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include <string.h>
#include <stdio.h>

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
    
    // >> CORREÇÃO 1: A função tcp_new() não aceita argumentos nesta versão do LwIP.
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

// >> CORREÇÃO 2: Renomeamos a função para evitar conflito com o typedef da biblioteca.
static void thingspeak_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    HTTP_REQUEST_STATE *state = (HTTP_REQUEST_STATE *)callback_arg;
    if (ipaddr) {
        state->remote_addr = *ipaddr;
        printf("IP do servidor encontrado: %s\n", ipaddr_ntoa(ipaddr));
        if (!http_open_connection(state)) {
            http_request_close(state);
        }
    } else {
        printf("Falha na resolucao do DNS\n");
        http_request_close(state);
    }
}

// Prepara e inicia o processo de envio dos dados
void thingspeak_send(float temperature) {
    // Aloca memória para o estado da requisição
    // Usamos calloc para zerar a memória
    HTTP_REQUEST_STATE *state = calloc(1, sizeof(HTTP_REQUEST_STATE));
    if (!state) {
        printf("Falha ao alocar estado\n");
        return;
    }

    // Formata a requisição HTTP GET com a chave da API e o valor da temperatura
    snprintf(state->http_request, sizeof(state->http_request),
             "GET /update?api_key=%s&field1=%.2f HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             THINGSPEAK_API_KEY, temperature, THINGSPEAK_HOST);
    
    // Inicia a resolução de DNS para encontrar o IP do servidor ThingSpeak
    cyw43_arch_lwip_begin();
    
    // >> CORREÇÃO 2 (continuação): Usamos o novo nome da função aqui.
    err_t err = dns_gethostbyname(THINGSPEAK_HOST, &state->remote_addr, thingspeak_dns_found_cb, state);
    
    cyw43_arch_lwip_end();

    if (err == ERR_OK) {
        // O endereço já estava em cache, podemos conectar diretamente
        printf("IP do servidor em cache: %s\n", ipaddr_ntoa(&state->remote_addr));
        if (!http_open_connection(state)) {
            http_request_close(state);
        }
    } else if (err != ERR_INPROGRESS) {
        printf("Erro imediato no DNS: %d\n", err);
        http_request_close(state);
    }
    // Se err == ERR_INPROGRESS, o callback `thingspeak_dns_found_cb` será chamado quando terminar.
}