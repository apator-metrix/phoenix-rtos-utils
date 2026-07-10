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


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../psh.h"
#include "u3app.h"

typedef int u3pin_command_handler_t(const u3pin_t *pin, int nargs, char *const args[]);

typedef struct {
	const char *cmd;
	const char *usage;
	u3pin_command_handler_t *handler;
} u3pin_command_t;

typedef struct {
	const char *arg;
	int value;
} u3pin_arg_mapping_t;

static u3pin_command_handler_t _cmd_config;
static u3pin_command_handler_t _cmd_get;
static u3pin_command_handler_t _cmd_set;

static const u3pin_command_t COMMANDS[] = {
	{ "config", "(in|out|af0-af7|a) [down|up]", _cmd_config },
	{ "get", "", _cmd_get },
	{ "set", "(0|1)", _cmd_set },
};

static const u3pin_arg_mapping_t MODES[] = {
	{ "in", gpio_mode_gpi },
	{ "out", gpio_mode_gpo },
	{ "af", gpio_mode_af },
	{ "a", gpio_mode_analog },
};

static const u3pin_arg_mapping_t PUPDS[] = {
	{ "down", gpio_pupd_pulldn },
	{ "up", gpio_pupd_pullup },
};

int u3pin_config(const u3pin_t *pin, char mode, char af, char otype, char ospeed, char pupd)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, gpio_def);
	imsg->gpio_def.port = pin->port;
	imsg->gpio_def.pin = pin->pin;
	imsg->gpio_def.mode = mode;
	imsg->gpio_def.af = af;
	imsg->gpio_def.ospeed = ospeed;
	imsg->gpio_def.otype = otype;
	imsg->gpio_def.pupd = pupd;
	return u3_send_msg(&msg);
}


int u3pin_get(const u3pin_t *pin)
{
	msg_t msg;
	int err;

	multi_i_t *imsg = u3_prepare_msg(&msg, gpio_get);
	imsg->gpio_get.port = pin->port;
	err = u3_send_msg(&msg);
	return (err < 0) ? err : ((((multi_o_t *)msg.o.raw)->gpio_get & (1 << pin->pin)) != 0);
}


int u3pin_set(const u3pin_t *pin, int state)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, gpio_set);
	imsg->gpio_set.port = pin->port;
	imsg->gpio_set.mask = 1 << pin->pin;
	imsg->gpio_set.state = state ? 0xffff : 0;
	return u3_send_msg(&msg);
}


int u3pin_parse(const char *name, u3pin_t *pin)
{
	int pinid;

	char portid = toupper(*name);
	if (portid == 'P') {
		name++;
		portid = toupper(*name);
	}

	if (portid < 'A' || portid > U3_MAX_PORT) {
		return -EINVAL;
	}

	name++;
	pinid = atoi(name);
	if (pinid < 0 || pinid > U3_MAX_PIN) {
		return -EINVAL;
	}

	pin->port = gpioa + (portid - 'A');
	pin->pin = (char)pinid;
	return EOK;
}


void u3pin_info(void)
{
	printf("control STM32U3 GPIO pins");
}


static int _map_arg(const u3pin_arg_mapping_t *mapping, size_t count, const char *arg)
{
	size_t i;
	for (i = 0; i < count; ++i) {
		if (strncasecmp(mapping[i].arg, arg, strlen(mapping[i].arg)) == 0) {
			return mapping[i].value;
		}
	}

	return -ENOENT;
}


static void _print_usage(void)
{
	int i;
	for (i = 0; i < NELEMS(COMMANDS); ++i) {
		printf("usage: u3pin %s port %s\n", COMMANDS[i].cmd, COMMANDS[i].usage);
	}
}


static int _cmd_config(const u3pin_t *pin, int nargs, char *const args[])
{
	int mode, af = 0, pupd = gpio_pupd_nopull, err;

	if (nargs < 1) {
		fprintf(stderr, "\nu3pin: missing mode!\n");
		return EXIT_FAILURE;
	}

	if ((mode = _map_arg(MODES, NELEMS(MODES), args[0])) < 0) {
		fprintf(stderr, "\nu3pin: unknown mode %s!\n", args[0]);
		return EXIT_FAILURE;
	}

	if (mode == gpio_mode_af) {
		af = atoi(args[0] + 2);  // afX
		if (af < 0 || af > 7) {
			fprintf(stderr, "\nu3pin: unknown function %s!\n", args[0]);
			return EXIT_FAILURE;
		}
	}

	if ((nargs > 1) && ((pupd = _map_arg(PUPDS, NELEMS(PUPDS), args[1])) < 0)) {
		fprintf(stderr, "\nu3pin: unknown pull mode %s!\n", args[1]);
		return EXIT_FAILURE;
	}

	if ((err = u3pin_config(pin, mode, af, gpio_otype_pp, gpio_ospeed_low, pupd)) < 0) {
		fprintf(stderr, "\nu3pin: u3pin_config failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


static int _cmd_get(const u3pin_t *pin, int nargs, char *const args[])
{
	int err;

	if ((err = u3pin_get(pin)) < 0) {
		fprintf(stderr, "\nu3pin: u3pin_get failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	printf("%d\n", err);
	return EXIT_SUCCESS;
}


static int _cmd_set(const u3pin_t *pin, int nargs, char *const args[])
{
	int err, state;

	if (nargs < 1) {
		fprintf(stderr, "\nu3pin: missing state!\n");
		return EXIT_FAILURE;
	}

	state = atoi(args[1]);
	if (state < 0 || state > 1) {
		fprintf(stderr, "\nu3pin: invalid state %s!\n", args[1]);
		return EXIT_FAILURE;
	}

	if ((err = u3pin_set(pin, state)) < 0) {
		fprintf(stderr, "\nu3pin: u3pin_set failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


int u3pin_run(int argc, char **argv)
{
	u3pin_t pin;
	int i, nargs = argc - 3;
	char **args = argv + 3;

	if (argc < 3) {
		_print_usage();
		return EXIT_SUCCESS;
	}

	if (u3pin_parse(argv[2], &pin) < 0) {
		fprintf(stderr, "\nu3pin: %s is not a valid pin!\n", argv[2]);
	}

	for (i = 0; i < NELEMS(COMMANDS); ++i) {
		if (strcmp(argv[1], COMMANDS[i].cmd) == 0) {
			return COMMANDS[i].handler(&pin, nargs, args);
		}
	}

	_print_usage();
	return EXIT_FAILURE;
}


void __attribute__((constructor)) u3pin_registerapp(void)
{
	static psh_appentry_t app_u3pin = { .name = "u3pin", .run = u3pin_run, .info = u3pin_info };

	psh_registerapp(&app_u3pin);
}
