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
#include <stdlib.h>

#include "../psh.h"
#include "u3app.h"


#define PARSE_PIN_OR_FAIL(name, out) \
	if (u3pin_parse((name), &(out)) < 0) { \
		fprintf(stderr, "\nu3iicdet: invalid pin %s for " #out "!\n", name); \
		return EXIT_FAILURE; \
	}


typedef struct {
	int i2c;
	u3pin_t scl;
	u3pin_t sda;
} i2cbus_t;


typedef struct {
	int i2c;
	int port;
	char pin;
	uint8_t af;
} pinctrl_t;

static const pinctrl_t SCL_AF[] = {
	{ i2c1, gpiob, 6, 4 },
	{ i2c1, gpiob, 8, 4 },
	{ i2c2, gpiob, 2, 3 },
	{ i2c2, gpiob, 10, 4 },
	{ i2c2, gpiob, 13, 4 },
	{ i2c3, gpioa, 7, 4 },
	{ i2c3, gpioc, 0, 4 },
	{ i2c3, gpiod, 12, 4 },
	{ i2c4, gpiob, 6, 12 },
	{ i2c4, gpiob, 10, 11 },
	{ i2c4, gpiod, 11, 5 },
};

static const pinctrl_t SDA_AF[] = {
	{ i2c1, gpiob, 3, 4 },
	{ i2c1, gpiob, 7, 4 },
	{ i2c1, gpiob, 9, 4 },
	{ i2c2, gpioa, 6, 4 },
	{ i2c2, gpiob, 11, 4 },
	{ i2c2, gpiob, 14, 4 },
	{ i2c3, gpiob, 4, 4 },
	{ i2c3, gpioc, 1, 4 },
	{ i2c3, gpiod, 13, 4 },
	{ i2c4, gpiob, 7, 12 },
	{ i2c4, gpiob, 11, 11 },
	{ i2c4, gpiod, 12, 5 },
};


static int _get_af(int i2c, const u3pin_t *pin, const pinctrl_t *pinctrl, size_t count)
{
	const pinctrl_t *end = pinctrl + count;

	while (pinctrl < end) {
		if ((pinctrl->i2c == i2c) && (pinctrl->port == pin->port) && (pinctrl->pin == pin->pin)) {
			return pinctrl->af;
		}

		++pinctrl;
	}

	return -ENOENT;
}


static int _get_scl_af(const i2cbus_t *bus)
{
	return _get_af(bus->i2c, &bus->scl, SCL_AF, NELEMS(SCL_AF));
}


static int _get_sda_af(const i2cbus_t *bus)
{
	return _get_af(bus->i2c, &bus->sda, SDA_AF, NELEMS(SDA_AF));
}


static int _i2c_read(int i2c, unsigned char addr, void *buff, size_t len)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, i2c_get);
	imsg->i2c_msg.i2c = i2c;
	imsg->i2c_msg.addr = addr;
	msg.o.data = buff;
	msg.o.size = len;
	return u3_send_msg(&msg);
}


static int _i2c_init(const i2cbus_t *bus)
{
	int scl_af = _get_scl_af(bus), sda_af = _get_sda_af(bus), err;

	if (scl_af < 0) {
		fprintf(stderr, "\nu3iicdet: invalid pin p%c%d for i2c%d scl!\n", 'a' + bus->scl.port, bus->scl.pin, bus->i2c + 1);
		return scl_af;
	}

	if (sda_af < 0) {
		fprintf(stderr, "\nu3iicdet: invalid pin p%c%d for i2c%d sda!\n", 'a' + bus->sda.port, bus->sda.pin, bus->i2c + 1);
		return sda_af;
	}

	if ((err = u3pin_config(&bus->scl, gpio_mode_af, scl_af, gpio_otype_od, gpio_ospeed_low, gpio_pupd_pullup)) < 0) {
		fprintf(stderr, "\nu3iicdet: scl pin configuration failed!\n");
		return err;
	}

	if ((err = u3pin_config(&bus->sda, gpio_mode_af, sda_af, gpio_otype_od, gpio_ospeed_low, gpio_pupd_pullup)) < 0) {
		fprintf(stderr, "\nu3iicdet: sda pin configuration failed!\n");
		return err;
	}

	return EOK;
}


static void _i2c_detect(const i2cbus_t *bus, uint8_t first, uint8_t last)
{
	uint8_t line, column;

	printf("   ");
	for (column = 0; column < 16; ++column) {
		printf("%3x", column);
	}
	printf("\n");

	for (line = first >> 4; line <= (last >> 4); ++line) {
		printf("%x0:", line);
		for (column = 0; column < 16; ++column) {
			uint8_t data, addr = (line << 4) | column;

			if ((addr < first) || (addr > last)) {
				printf("   ");
			}
			else if (_i2c_read(bus->i2c, addr, &data, sizeof(data)) < 0) {
				printf(" --");
			}
			else {
				printf(" %02x", addr);
			}
			fflush(stdout);
		}
		printf("\n");
	}
}


void u3iicdet_info(void)
{
	printf("detect I2C devices");
}


int u3iicdet_run(int argc, char **argv)
{
	int err, first = 0x03, last = 0x77;
	i2cbus_t bus;

	if ((argc != 4) && (argc != 6)) {
		printf("usage: u3iicdet i2c scl sda [first last]\n");
		return (argc < 2) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	if (argc == 6) {
		if ((first = atoi(argv[4])) < 0x00) {
			fprintf(stderr, "\nu3iicdet: invalid address %s!\n", argv[4]);
			return EXIT_FAILURE;
		}

		if ((last = atoi(argv[5])) > 0xff) {
			fprintf(stderr, "\nu3iicdet: invalid address %s!\n", argv[5]);
			return EXIT_FAILURE;
		}
	}

	bus.i2c = atoi(argv[1]) - 1;
	if ((bus.i2c < i2c1) || (bus.i2c > i2c4)) {
		fprintf(stderr, "\nu3iicdet: invalid I2C %s!\n", argv[1]);
		return EXIT_FAILURE;
	}

	PARSE_PIN_OR_FAIL(argv[2], bus.scl);
	PARSE_PIN_OR_FAIL(argv[3], bus.sda);

	if ((err = _i2c_init(&bus)) < 0) {
		fprintf(stderr, "\nu3iicdet: _i2c_init failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	_i2c_detect(&bus, (uint8_t)first, (uint8_t)last);
	return EXIT_SUCCESS;
}


void __attribute__((constructor)) u3iicdet_registerapp(void)
{
	static psh_appentry_t app_u3iicdet = { .name = "u3iicdet", .run = u3iicdet_run, .info = u3iicdet_info };

	psh_registerapp(&app_u3iicdet);
}
