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

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include <phoenix/arch/armv8m/stm32/u3/stm32u3.h>
#include <stm32l4-multi.h>
#include <sys/msg.h>

#include "../psh.h"
#include "u3app.h"


static struct {
	pthread_cond_t icond;
	pthread_mutex_t imutex;
} u3push_common;


static multi_i_t *_make_devctl(msg_t *msg, int type)
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


static int _exti_map(int exti, int port)
{
	msg_t msg;
	int err;

	multi_i_t *imsg = _make_devctl(&msg, exti_map);
	imsg->exti_map.line = exti;
	imsg->exti_map.port = port;

	err = msgSend(u3app_common.multi.port, &msg);
	if (err < 0) {
		return err;
	}

	return msg.o.err;
}


static int _exti_config(int exti, unsigned char mode, unsigned char edge)
{
	msg_t msg;
	int err;

	multi_i_t *imsg = _make_devctl(&msg, exti_def);
	imsg->exti_def.line = exti;
	imsg->exti_def.mode = mode;
	imsg->exti_def.edge = edge;

	err = msgSend(u3app_common.multi.port, &msg);
	if (err < 0) {
		return err;
	}

	return msg.o.err;
}


static int _exti_handler(unsigned int n, void *arg)
{
	return 1;
}


static int _gpio_config(int port, char pin, char mode, char af, char otype, char ospeed, char pupd)
{
	msg_t msg;

	multi_i_t *imsg = _make_devctl(&msg, gpio_def);
	imsg->gpio_def.port = port;
	imsg->gpio_def.pin = pin;
	imsg->gpio_def.mode = mode;
	imsg->gpio_def.af = af;
	imsg->gpio_def.ospeed = ospeed;
	imsg->gpio_def.otype = otype;
	imsg->gpio_def.pupd = pupd;

	int ret = msgSend(u3app_common.multi.port, &msg);
	if (ret < 0) {
		return ret;
	}

	return msg.o.err;
}


void u3push_info(void)
{
	printf("wait until USER button is pushed");
}


int u3push_run(int argc, char **argv)
{
	int err;

	if ((err = _exti_map(13, gpioc)) < 0) {
		fprintf(stderr, "\nu3push: _exti_map failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = _gpio_config(gpioc, 13, gpio_mode_gpi, 0, gpio_otype_pp, gpio_ospeed_low, gpio_pupd_pulldn)) < 0) {
		fprintf(stderr, "\nu3push: _gpio_config failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = _exti_config(13, exti_irq, exti_falling)) < 0) {
		fprintf(stderr, "\nu3push: _exti_config failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	pthread_mutex_init(&u3push_common.imutex, NULL);

	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	if ((err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) < 0) {
		fprintf(stderr, "\nu3push: pthread_condattr_setclock failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = pthread_cond_init(&u3push_common.icond, &attr)) < 0) {
		fprintf(stderr, "\nu3push: pthread_cond_init failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	pthread_condattr_destroy(&attr);
	interrupt(exti13_irq, _exti_handler, NULL, u3push_common.icond.condh, NULL);

	printf("u3push: push USER to continue...\n");
	pthread_mutex_lock(&u3push_common.imutex);
	pthread_cond_wait(&u3push_common.icond, &u3push_common.imutex);
	pthread_mutex_unlock(&u3push_common.imutex);

	return EXIT_SUCCESS;
}


void __attribute__((constructor)) u3push_registerapp(void)
{
	static psh_appentry_t app_u3push = { .name = "u3push", .run = u3push_run, .info = u3push_info };

	psh_registerapp(&app_u3push);
}
