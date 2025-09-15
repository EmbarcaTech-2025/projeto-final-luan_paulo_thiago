/**
 * @file rtc_ntp.h
 * @brief API de sincronização do RTC via NTP.
 */

#ifndef RTC_NTP_H
#define RTC_NTP_H

#include <stdbool.h>
#include <stdint.h>

void rtc_ntp_init(void);
bool rtc_ntp_sync(const char *server_host, uint32_t timeout_ms);

#endif /* RTC_NTP_H */
