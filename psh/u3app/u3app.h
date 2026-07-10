/*
 * Phoenix-RTOS
 *
 * Phoenix-RTOS SHell
 *
 * u3app - STM32U3 test utilities
 *
 * Copyright 2026 Apator Metrix
 * Author: Mateusz Karcz
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef PSH_U3APP_H
#define PSH_U3APP_H

#include <stm32l4-multi.h>
#include <sys/msg.h>


typedef struct {
	int port;
	char pin;
} u3pin_t;


#define NELEMS(x) (sizeof(x) / sizeof(x[0]))


#define U3_MAX_PORT 'H'
#define U3_MAX_PIN  15


extern multi_i_t *u3_prepare_msg(msg_t *msg, int type);
extern int u3_send_msg(msg_t *msg);

extern int u3pin_config(const u3pin_t *pin, char mode, char af, char otype, char ospeed, char pupd);
extern int u3pin_get(const u3pin_t *pin);
extern int u3pin_set(const u3pin_t *pin, int state);

extern int u3pin_parse(const char *name, u3pin_t *pin);


#endif
