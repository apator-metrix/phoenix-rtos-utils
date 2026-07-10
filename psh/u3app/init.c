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

#include "u3app.h"


static struct {
	oid_t multi;
} u3app_common;


void __attribute__((constructor)) u3app_init(void)
{
	u3app_common.multi.id = -1;
	u3app_common.multi.port = -1;
	while (lookup("/dev/multi", NULL, &u3app_common.multi) < 0) {
		usleep(100000);
	}
}


multi_i_t *u3_prepare_msg(msg_t *msg, int type)
{
	msg->type = mtDevCtl;
	msg->oid = u3app_common.multi;
	msg->i.data = NULL;
	msg->i.size = 0;
	msg->o.data = NULL;
	msg->o.size = 0;

	multi_i_t *imsg = (multi_i_t *)msg->i.raw;
	imsg->type = type;
	return imsg;
}


int u3_send_msg(msg_t *msg)
{
	int ret = msgSend(u3app_common.multi.port, msg);
	return (ret < 0) ? ret : msg->o.err;
}
