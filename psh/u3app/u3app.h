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


extern multi_i_t *u3_prepare_msg(msg_t *msg, int type);
extern int u3_send_msg(msg_t *msg);


#endif
