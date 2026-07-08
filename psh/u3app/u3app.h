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

#include <phoenix/types.h>

typedef struct {
	oid_t multi;
} u3app_common_t;

extern u3app_common_t u3app_common;

#endif
