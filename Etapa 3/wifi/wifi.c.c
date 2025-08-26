#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "pico/multicore.h"

#define WIFI_SSID "HBR Guest"     // Substitua pelo nome da sua rede
#define WIFI_PASS "Visit@8523"    // Substitua pela senha da sua rede

// Buffer para resposta HTTP
char http_response[1024];

// ----- Início Módulo Wifi 

// Função para criar uma resposta HTTP simples
void create_http_response() {
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n\r\n"
        "<!DOCTYPE html>"
        "<html>"
        "<head><meta charset=\"UTF-8\"><title>Página Pico W</title></head>"
        "<body>"
        "<h1>Bem-vindo à página da Pico W!</h1>"
        "<p>Esta é uma página simples servida pela sua placa.</p>"
        "</body>"
        "</html>\r\n");
}


static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    // Gerar a resposta HTML simples
    create_http_response();

    // Enviar a resposta ao cliente
    tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);

    pbuf_free(p);

    return ERR_OK;
}



// Callback de conexão: associa o http_callback à conexão
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

// Função para iniciar o servidor HTTP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);

    printf("Servidor HTTP rodando na porta 80...\n");
}


// Função para rodar o Wi-Fi em paralelo no Core 1
void wifi_task() {
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    while (true) {
        printf("Tentando conectar ao Wi-Fi...\n");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 5000) == 0) {
            printf("Wi-Fi conectado!\n");
            
            uint8_t *ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            printf("Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

            start_http_server(); // *NOVO: Inicia o servidor HTTP assim que conectar

            break;
        }

        printf("Falha na conexão, tentando novamente...\n");
        sleep_ms(2000); // Pequena espera antes de tentar de novo
    }

    while (true) {
        cyw43_arch_poll();  // Mantém a conexão Wi-Fi ativa

        static uint64_t last_print_time = 0;
        uint64_t now = time_us_64();

        // Print do IP a cada 30 segundos
        if (now - last_print_time >= 5 * 1000000) { 
            uint8_t *ip = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
            printf("[Wi-Fi] Endereço IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            last_print_time = now;
        }

        sleep_ms(100);      // Pequena espera para evitar uso excessivo da CPU
    }
}

int main()
{
    stdio_init_all();

    // Inicializar wifi segundo núcleo
    multicore_launch_core1(wifi_task);

    while (true) {
    }
}
