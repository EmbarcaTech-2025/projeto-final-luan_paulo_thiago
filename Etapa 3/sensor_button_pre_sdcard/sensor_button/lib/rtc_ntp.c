/**
 * @file rtc_ntp.c
 * @brief Sincronização do RTC interno via NTP usando lwIP (UDP).
 * @details
 * Envia um pacote NTP, aguarda resposta e ajusta o RTC do RP2040.
 * Converte o tempo NTP (base 1900) para `datetime_t` em UTC.
 */

#include "lib/rtc_ntp.h"
#include <string.h>
#include <inttypes.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/ip4_addr.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "utils.h"
#include "lib/logger.h"

#define TAG "rtc_ntp"

#define NTP_PORT                    123U            /**< Porta UDP do NTP. */
#define NTP_PKT_LEN                 48U             /**< Tamanho do pacote NTP. */

/************************************************************/
/* LINHA ADICIONADA - Define o fuso horário como 0 (UTC)    */
/************************************************************/
#define RTC_NTP_TZ_OFFSET_SECONDS   0               /**< Offset de fuso (0 para UTC). */
/************************************************************/


/**
 * @brief Testa se o ano é bissexto.
 * @param y Ano (ex.: 2025).
 * @return true se bissexto; false caso contrário.
 */
static inline bool is_leap(int y)
{
    return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

/**
 * @brief Dias no mês para o ano dado (considera bissexto).
 * @param y Ano.
 * @param m Mês [1..12].
 * @return Dias no mês.
 */
static int dim(int y, int m)
{
    static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int x = d[m - 1];
    return (m == 2 && is_leap(y)) ? 29 : x;
}

/**
 * @brief Converte segundos NTP (base 1900) para `datetime_t` local com fuso.
 * @param ntp_sec Segundos desde 1900-01-01 00:00:00.
 * @param tz_offset_seconds Offset de fuso em segundos (pode ser negativo).
 * @param[out] out Estrutura de saída preenchida.
 * @return true se conversão bem-sucedida; false caso contrário.
 */
static bool ntpsec_to_datetime_local(uint32_t ntp_sec, int32_t tz_offset_seconds, datetime_t *out)
{
    if (!out)
    {
        return false;
    }

    int64_t local = (int64_t)ntp_sec + (int64_t)tz_offset_seconds;

    if (local < 0)
    {
        local = 0;
    }

    uint64_t secs = (uint64_t)local;
    uint64_t days = secs / 86400ULL;
    uint32_t sod = (uint32_t)(secs % 86400ULL);

    int year = 1900;

    while (1)
    {
        int dy = is_leap(year) ? 366 : 365;

        if (days >= (uint64_t)dy)
        {
            days -= dy;
            year++;
        }
        else
            break;
    }

    int month = 1;

    while (1)
    {
        int dm = dim(year, month);

        if (days >= (uint64_t)dm)
        {
            days -= dm;
            month++;
        }
        else
            break;
    }

    int day = (int)days + 1;
    int hour = (int)(sod / 3600U);
    int min = (int)((sod % 3600U) / 60U);
    int sec = (int)(sod % 60U);
    int dotw = (int)((secs / 86400ULL + 1ULL) % 7ULL);

    out->year = year;
    out->month = month;
    out->day = day;
    out->hour = hour;
    out->min = min;
    out->sec = sec;
    out->dotw = dotw;
    return true;
}

/**
 * @brief Lê um u32 big-endian de buffer de bytes.
 * @param b Ponteiro para 4 bytes.
 * @return Valor de 32 bits.
 */
static uint32_t rd_be_u32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | ((uint32_t)b[3]);
}

/** @brief Contexto interno para operação NTP assíncrona (UDP). */
typedef struct
{
    SemaphoreHandle_t sem;  /**< Semáforo para sinalização de resposta. */
    bool ok;                /**< Flag de sucesso ao receber pacote válido. */
    uint32_t ntp_sec;       /**< Segundos NTP recebidos (campo transmit time). */
} ntp_ctx_t;

/**
 * @brief Callback de recepção UDP do NTP.
 */
static void ntp_recv_cb(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                        const ip_addr_t *addr, u16_t port)
{
    (void)upcb;
    (void)addr;
    (void)port;
    ntp_ctx_t *ctx = (ntp_ctx_t *)arg;

    if (!p || p->len < NTP_PKT_LEN)
    {
        if (p)
        {
            pbuf_free(p);
        }
        return;
    }

    const uint8_t *pkt = (const uint8_t *)p->payload;
    uint32_t sec = rd_be_u32(&pkt[40]);

    ctx->ntp_sec = sec;
    ctx->ok = true;

    pbuf_free(p);
    xSemaphoreGive(ctx->sem);
}

/**
 * @brief Inicializa o RTC com base 1900-01-01 (base do NTP).
 */
void rtc_ntp_init(void)
{
    rtc_init();

    datetime_t base1900 = {
        .year = 1900, .month = 1, .day = 1, .dotw = 1, .hour = 0, .min = 0, .sec = 0};
    rtc_set_datetime(&base1900);

    LOG(TAG, "RTC inicializado para 1900/01/01 00:00:00.000 (base NTP)");
}

/**
 * @brief Sincroniza o RTC consultando um servidor NTP (UDP).
 * @param server_host Hostname do servidor (ex.: "pool.ntp.org").
 * @param timeout_ms Timeout total para aguardar resposta.
 * @return true em caso de sucesso; false em erros/timeout.
 */
bool rtc_ntp_sync(const char *server_host, uint32_t timeout_ms)
{
    if (!server_host)
    {
        return false;
    }

    ip_addr_t server_ip;

    if (!utils_resolve_dns(server_host, &server_ip, 5000))
    {
        LOG(TAG, "DNS falhou para %s", server_host);
        return false;
    }

    struct udp_pcb *pcb = udp_new();

    if (!pcb)
    {
        LOG(TAG, "Falha ao criar UDP PCB.");
        return false;
    }

    ntp_ctx_t ctx = {0};
    ctx.sem = xSemaphoreCreateBinary();

    if (!ctx.sem)
    {
        LOG(TAG, "Falha ao criar semáforo NTP.");
        udp_remove(pcb);
        return false;
    }
    udp_recv(pcb, ntp_recv_cb, &ctx);

    uint8_t ntp[NTP_PKT_LEN] = {0};
    ntp[0] = 0x23;

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_PKT_LEN, PBUF_RAM);
    if (!p)
    {
        LOG(TAG, "Falha ao alocar pbuf NTP.");
        vSemaphoreDelete(ctx.sem);
        udp_remove(pcb);
        return false;
    }

    memcpy(p->payload, ntp, NTP_PKT_LEN);

    err_t err = udp_sendto(pcb, p, &server_ip, NTP_PORT);
    pbuf_free(p);

    if (err != ERR_OK)
    {
        LOG(TAG, "Erro udp_sendto NTP (err=%d).", (int)err);
        vSemaphoreDelete(ctx.sem);
        udp_remove(pcb);
        return false;
    }

    LOG(TAG, "Solicitação NTP enviada para %s:%d", ip4addr_ntoa(&server_ip), (int)NTP_PORT);

    bool ok = (xSemaphoreTake(ctx.sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) && ctx.ok;

    udp_remove(pcb);
    vSemaphoreDelete(ctx.sem);

    if (!ok)
    {
        LOG(TAG, "Timeout/erro aguardando resposta NTP.");
        return false;
    }

    datetime_t dt;

    if (!ntpsec_to_datetime_local(ctx.ntp_sec, RTC_NTP_TZ_OFFSET_SECONDS, &dt))
    {
        LOG(TAG, "Conversão NTP->datetime falhou.");
        return false;
    }

    if (!rtc_set_datetime(&dt))
    {
        LOG(TAG, "rtc_set_datetime falhou.");
        return false;
    }

    uint32_t ms = (uint32_t)((time_us_64() / 1000ULL) % 1000ULL);
    
    // LOG ATUALIZADO para mostrar que agora é UTC
    LOG(TAG, "RTC sincronizado: %04d/%02d/%02d %02d:%02d:%02d.%03u (UTC)",
        dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, ms);
        
    return true;
}