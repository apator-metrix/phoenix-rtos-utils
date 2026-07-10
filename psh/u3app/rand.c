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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../psh.h"
#include "u3app.h"


void u3rand_info(void)
{
	printf("read STM32U3 random number generator");
}


static int _rng_read(void *buff, size_t len)
{
	msg_t msg;

	u3_prepare_msg(&msg, rng_get);
	msg.o.data = buff;
	msg.o.size = len;
	return u3_send_msg(&msg);
}


static int _random_int(void)
{
	int err = EOK;
	unsigned num = 0;

	if ((err = _rng_read(&num, sizeof(num))) < 0) {
		fprintf(stderr, "\nu3rand: _rng_read failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	printf("%u\n", num);
	return EXIT_SUCCESS;
}


static int _random_data(const char *param)
{
	int err = EOK, size;

	for (size = atoi(param); size > 0; --size) {
		uint8_t c;
		if ((err = _rng_read(&c, sizeof(c))) < 0) {
			fprintf(stderr, "\nu3rand: _rng_read failed with %d!\n", err);
			return EXIT_FAILURE;
		}

		putchar(' ' + (c % ('~' - ' ' + 1)));
	}

	return EXIT_SUCCESS;
}


int u3rand_run(int argc, char **argv)
{
	if (argc < 2) {
		return _random_int();
	}

	return _random_data(argv[1]);
}


void __attribute__((constructor)) u3rand_registerapp(void)
{
	static psh_appentry_t app_u3rand = { .name = "u3rand", .run = u3rand_run, .info = u3rand_info };

	psh_registerapp(&app_u3rand);
}
