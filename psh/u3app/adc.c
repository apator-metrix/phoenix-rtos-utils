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


#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../psh.h"
#include "u3app.h"


typedef struct {
	int adc;
	int channel;
	int port;
	char pin;
} adcpin_t;

typedef struct {
	int channel;
	char name[8];
} namedch_t;

static const adcpin_t ADC_PIN[] = {
	{ adc1, 3, gpioa, 0 },
	{ adc1, 4, gpioa, 1 },
	{ adc1, 5, gpioa, 2 },
	{ adc1, 6, gpioa, 3 },
	{ adc1, 7, gpioa, 4 },
	{ adc1, 8, gpioa, 5 },
	{ adc1, 9, gpioa, 6 },
	{ adc1, 10, gpioa, 7 },
	{ adc1, 13, gpiob, 0 },
	{ adc1, 14, gpiob, 1 },
	{ adc1, 15, gpiob, 2 },
	{ adc1, 1, gpioc, 0 },
	{ adc1, 2, gpioc, 1 },
	{ adc2, 3, gpioa, 4 },
	{ adc2, 4, gpioa, 5 },
	{ adc2, 5, gpioa, 6 },
	{ adc2, 6, gpioa, 7 },
	{ adc2, 9, gpiob, 0 },
	{ adc2, 10, gpiob, 1 },
	{ adc2, 1, gpioc, 2 },
	{ adc2, 2, gpioc, 3 },
	{ adc2, 11, gpiod, 11 },
	{ adc2, 12, gpiod, 12 },
	{ adc2, 13, gpiod, 13 },
};

static const namedch_t NAMED_CH[] = {
	{ 0, "vref" },
	{ 16, "vbat" },
	{ 17, "vsense" },
	{ 18, "vcore" },
};


void u3adc_info(void)
{
	printf("read STM32U3 analog-digital converter");
}


static int _adc_read(unsigned *val, int adcno, int channel)
{
	int err;
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, adc_get);
	imsg->adc_get.adcno = adcno;
	imsg->adc_get.channel = channel;


	err = u3_send_msg(&msg);
	if (err == 0) {
		multi_o_t *omsg = (multi_o_t *)msg.o.raw;
		*val = omsg->adc_valmv;
	}
	return err;
}


static const adcpin_t *_match_pin(const u3pin_t *pin)
{
	for (int i = 0; i < NELEMS(ADC_PIN); ++i) {
		if ((pin->port == ADC_PIN[i].port) && (pin->pin == ADC_PIN[i].pin)) {
			return ADC_PIN + i;
		}
	}

	return NULL;
}


static int _match_namedch(const char *name)
{
	for (int i = 0; i < NELEMS(NAMED_CH); ++i) {
		if (strcmp(name, NAMED_CH[i].name) == 0) {
			return NAMED_CH[i].channel;
		}
	}

	return -ENOENT;
}


static int _read_internal(const char *name)
{
	int ret = _match_namedch(name);
	if (ret < 0) {
		return ret;
	}


	unsigned val = 0;
	if ((ret = _adc_read(&val, adc1, ret)) < 0) {
		fprintf(stderr, "\nu3adc: read failed on adc1 channel %d!\n", ret);
		return ret;
	}

	return (val <= (unsigned)INT_MAX) ? (int)val : -ERANGE;
}


static int _read_external(const char *name)
{
	int err;

	u3pin_t input;
	if (u3pin_parse(name, &input) < 0) {
		fprintf(stderr, "u3adc: invalid pin %s for input!\n", name);
		return -EINVAL;
	}

	if ((err = u3pin_config(&input, gpio_mode_analog, 0, gpio_otype_pp, gpio_ospeed_low, gpio_pupd_nopull)) < 0) {
		fprintf(stderr, "u3adc: input pin configuration failed!\n");
		return err;
	}

	const adcpin_t *adcpin = _match_pin(&input);
	if (adcpin == NULL) {
		fprintf(stderr, "u3adc: invalid analog input %s!\n", name);
		return -ENOENT;
	}

	unsigned val = 0;
	if ((err = _adc_read(&val, adcpin->adc, adcpin->channel)) < 0) {
		fprintf(stderr, "u3adc: read failed on adc%d channel %d!\n", adcpin->adc + 1, adcpin->channel);
		return err;
	}

	return (val <= (unsigned)INT_MAX) ? (int)val : -ERANGE;
}


int u3adc_run(int argc, char **argv)
{
	int ret;

	if (argc < 2) {
		printf("usage: u3adc input");
		for (int i = 0; i < NELEMS(NAMED_CH); ++i) {
			printf("|%s", NAMED_CH[i].name);
		}
		printf("\n");
		return EXIT_SUCCESS;
	}

	if ((ret = _read_internal(argv[1])) < 0) {
		if (ret != -ENOENT) {
			fprintf(stderr, "u3adc: internal input failed!\n");
			return EXIT_FAILURE;
		}
	}

	if (ret == -ENOENT) {
		if ((ret = _read_external(argv[1])) < 0) {
			fprintf(stderr, "u3adc: external input failed!\n");
			return EXIT_FAILURE;
		}
	}

	printf("%dmV\n", ret);
	return EXIT_SUCCESS;
}


void __attribute__((constructor)) u3adc_registerapp(void)
{
	static psh_appentry_t app_u3adc = { .name = "u3adc", .run = u3adc_run, .info = u3adc_info };

	psh_registerapp(&app_u3adc);
}
