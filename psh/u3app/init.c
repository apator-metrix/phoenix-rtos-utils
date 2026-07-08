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

#include <stddef.h>
#include <unistd.h>

#include <sys/msg.h>

#include "u3app.h"


u3app_common_t u3app_common;


void __attribute__((constructor)) u3app_init(void)
{
	u3app_common.multi.id = -1;
	u3app_common.multi.port = -1;
	while (lookup("/dev/multi", NULL, &u3app_common.multi) < 0) {
		usleep(100000);
	}
}
