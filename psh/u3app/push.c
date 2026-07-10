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

#include "../psh.h"
#include "u3app.h"


static struct {
	pthread_cond_t icond;
	pthread_mutex_t imutex;
} u3push_common;


static int _exti_map(int exti, int port)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, exti_map);
	imsg->exti_map.line = exti;
	imsg->exti_map.port = port;
	return u3_send_msg(&msg);
}


static int _exti_config(int exti, unsigned char mode, unsigned char edge)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, exti_def);
	imsg->exti_def.line = exti;
	imsg->exti_def.mode = mode;
	imsg->exti_def.edge = edge;
	return u3_send_msg(&msg);
}


static int _exti_handler(unsigned int n, void *arg)
{
	return 1;
}


void u3push_info(void)
{
	printf("wait until USER button is pushed");
}


int u3push_run(int argc, char **argv)
{
	const u3pin_t USER_BUTTON = { gpioc, 13 };

	int err;

	if ((err = _exti_map(USER_BUTTON.pin, USER_BUTTON.port)) < 0) {
		fprintf(stderr, "\nu3push: _exti_map failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = u3pin_config(&USER_BUTTON, gpio_mode_gpi, 0, gpio_otype_pp, gpio_ospeed_low, gpio_pupd_pulldn)) < 0) {
		fprintf(stderr, "\nu3push: u3pin_config failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = _exti_config(USER_BUTTON.pin, exti_irq, exti_falling)) < 0) {
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
	interrupt(exti0_irq + USER_BUTTON.pin, _exti_handler, NULL, u3push_common.icond.condh, NULL);

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
