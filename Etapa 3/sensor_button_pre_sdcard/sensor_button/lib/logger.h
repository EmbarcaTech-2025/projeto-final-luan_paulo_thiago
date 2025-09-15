/**
 * @file logger.h
 * @brief Declarações do logger.
 */

#ifndef LOGGER_H
#define LOGGER_H

void logger_init(void);
void logger_log(const char *tag, const char *fmt, ...);

/** @brief Macro prática para logs com tag constante. */
#define LOG(TAG, fmt, ...) logger_log((TAG), (fmt), ##__VA_ARGS__)

#endif /* LOGGER_H */
