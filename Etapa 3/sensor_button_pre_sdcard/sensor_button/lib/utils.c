/**
 * @file utils.c
 * @brief Utilitários diversos (DNS com lwIP).
 */

#include "utils.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "lib/logger.h"

#define TAG "utils"

/**
 * @brief Callback interno de resolução DNS (lwIP).
 * @param name Host solicitado.
 * @param ipaddr Resultado (se sucesso).
 * @param arg Contexto com semáforo e destino de IP.
 */
static void dns_cb(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)name;

    struct
    {
        SemaphoreHandle_t sem;
        ip_addr_t *out;
        err_t result;
    } *ctx = (typeof(ctx))arg;

    if (ipaddr)
    {
        *(ctx->out) = *ipaddr;
        ctx->result = ERR_OK;
    }
    else
    {
        ctx->result = ERR_VAL;
    }

    xSemaphoreGive(ctx->sem);
}

/**
 * @brief Resolve um hostname para endereço IPv4 usando lwIP DNS.
 * @param hostname Nome do host (ex.: "api.thingspeak.com").
 * @param[out] out_ip Endereço IPv4 de saída.
 * @param timeout_ms Tempo máximo de espera (ms).
 * @return true em sucesso; false em timeout/erro.
 * @note Define o DNS primário como 8.8.8.8 antes de resolver.
 */
bool utils_resolve_dns(const char *hostname, ip_addr_t *out_ip, uint32_t timeout_ms)
{
    if (!hostname || !out_ip)
    {
        return false;
    }

    ip_addr_t dns_server;
    IP4_ADDR(&dns_server, 8, 8, 8, 8);
    dns_setserver(0, &dns_server);
    LOG(TAG, "Servidor DNS primário: 8.8.8.8");
    LOG(TAG, "Resolvendo hostname: %s", hostname);

    struct
    {
        SemaphoreHandle_t sem;
        ip_addr_t *out;
        err_t result;
    } ctx = {0};

    ctx.out = out_ip;
    ctx.result = ERR_INPROGRESS;
    ctx.sem = xSemaphoreCreateBinary();

    if (!ctx.sem)
    {
        LOG(TAG, "Falha ao criar semáforo DNS.");
        return false;
    }

    err_t err = dns_gethostbyname(hostname, out_ip, dns_cb, &ctx);

    if (err == ERR_OK)
    {
        LOG(TAG, "Hostname resolvido imediatamente: %s", ip4addr_ntoa(out_ip));
        vSemaphoreDelete(ctx.sem);
        return true;
    }

    if (err != ERR_INPROGRESS)
    {
        LOG(TAG, "dns_gethostbyname falhou (err=%d)", (int)err);
        vSemaphoreDelete(ctx.sem);
        return false;
    }

    if (xSemaphoreTake(ctx.sem, pdMS_TO_TICKS(timeout_ms)) == pdTRUE && ctx.result == ERR_OK)
    {
        LOG(TAG, "Hostname resolvido: %s", ip4addr_ntoa(out_ip));
        vSemaphoreDelete(ctx.sem);
        return true;
    }

    LOG(TAG, "Timeout/erro resolvendo %s", hostname);
    vSemaphoreDelete(ctx.sem);
    return false;
}
