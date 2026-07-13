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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/pwman.h>

#include "../psh.h"
#include "u3app.h"


typedef struct {
	int spi;
	u3pin_t cs;
	u3pin_t sck;
	u3pin_t miso;
	u3pin_t mosi;
	u3pin_t wp_hold;
} spiflash_t;


#define FLASH_CMD_SFDP    0x5a
#define FLASH_CMD_JEDECID 0x9f

#define MAX_SFDP_READ 16


static int _spi_enable(int spi)
{
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, spi_def);
	imsg->spi_def.spi = spi;
	imsg->spi_def.enable = 1;
	imsg->spi_def.mode = 0;
	imsg->spi_def.bdiv = 0;
	return u3_send_msg(&msg);
}


static int _spi_read(int spi, unsigned char cmd, unsigned int addr, unsigned char flags, void *buff, size_t bufflen)
{
	int err;
	msg_t msg;

	multi_i_t *imsg = u3_prepare_msg(&msg, spi_get);
	msg.o.data = buff;
	msg.o.size = bufflen;
	imsg->spi_rw.spi = spi;
	imsg->spi_rw.cmd = cmd;
	imsg->spi_rw.addr = addr;
	imsg->spi_rw.flags = flags;
	err = u3_send_msg(&msg);

	return err;
}


static int _flash_init(const spiflash_t *flash)
{
	u3pin_set(&flash->cs, 0);
	u3pin_set(&flash->sck, 0);
	u3pin_set(&flash->miso, 0);
	u3pin_set(&flash->mosi, 0);
	u3pin_set(&flash->wp_hold, 0);

	u3pin_config(&flash->cs, gpio_mode_gpo, 0, 0, 0, 0);
	u3pin_config(&flash->sck, gpio_mode_gpo, 0, 0, 0, 0);
	u3pin_config(&flash->miso, gpio_mode_gpo, 0, 0, 0, 0);
	u3pin_config(&flash->mosi, gpio_mode_gpo, 0, 0, 0, 0);
	u3pin_config(&flash->wp_hold, gpio_mode_gpo, 0, 0, 0, 0);

	return _spi_enable(flash->spi);
}


static int _get_sck_miso_af(const spiflash_t *flash)
{
	return (flash->spi < spi3) ? 5 : 6;
}


static int _get_mosi_af(const spiflash_t *flash)
{
	return ((flash->spi < spi3) || ((flash->spi == spi3) && (flash->cs.port == gpiod) && (flash->cs.pin == 6))) ? 5 : 6;
}


static int _flash_enable(const spiflash_t *flash)
{
	int sck_miso_af = _get_sck_miso_af(flash);
	int mosi_af = _get_mosi_af(flash);

	keepidle(1);
	u3pin_config(&flash->cs, gpio_mode_gpi, 0, 0, 0, 0);
	u3pin_set(&flash->cs, 1);
	u3pin_config(&flash->cs, gpio_mode_gpo, 0, 0, 0, 0);

	u3pin_set(&flash->wp_hold, 1);

	u3pin_config(&flash->sck, gpio_mode_af, sck_miso_af, 0, 0, 0);
	u3pin_config(&flash->mosi, gpio_mode_af, mosi_af, 0, 0, 0);
	u3pin_config(&flash->miso, gpio_mode_af, sck_miso_af, 0, 0, 0);

	usleep(10000);
	return 0;
}


static int _flash_disable(const spiflash_t *flash)
{
	int sck_miso_af = _get_sck_miso_af(flash);
	int mosi_af = _get_mosi_af(flash);

	u3pin_config(&flash->sck, gpio_mode_gpo, sck_miso_af, 0, 0, 0);
	u3pin_config(&flash->mosi, gpio_mode_gpo, mosi_af, 0, 0, 0);
	u3pin_config(&flash->miso, gpio_mode_gpo, sck_miso_af, 0, 0, 0);

	u3pin_set(&flash->wp_hold, 0);

	u3pin_config(&flash->cs, gpio_mode_gpi, 0, 0, 0, 0);
	u3pin_set(&flash->cs, 0);
	u3pin_config(&flash->cs, gpio_mode_gpo, 0, 0, 0, 0);

	keepidle(0);
	return 0;
}


static int _flash_read(const spiflash_t *flash, unsigned char cmd, unsigned int addr, unsigned char flags, void *buff, size_t bufflen)
{
	int err;
	u3pin_set(&flash->cs, 0);
	err = _spi_read(flash->spi, cmd, addr, flags, buff, bufflen);
	u3pin_set(&flash->cs, 1);
	return err;
}


static int _flash_detect(const spiflash_t *flash)
{
	int err;
	uint8_t jedec[3];

	if ((err = _flash_enable(flash)) < 0) {
		_flash_disable(flash);
		return err;
	}

	err = _flash_read(flash, FLASH_CMD_JEDECID, 0, spi_cmd, jedec, sizeof(jedec));
	_flash_disable(flash);
	return (err < 0) ? err : ((jedec[0] << 16) | (jedec[1] << 8) | jedec[2]);
}


static int _flash_read_sfdp(const spiflash_t *flash, unsigned int addr, void *buff, size_t bufflen)
{
	int err;
	char sfdp[4 + MAX_SFDP_READ];

	if ((bufflen > MAX_SFDP_READ) || (addr > 0xffffff)) {
		return -EINVAL;
	}

	err = _flash_read(flash, FLASH_CMD_SFDP, addr, spi_cmd, &sfdp, 4 + bufflen);
	if (err >= 0) {
		memcpy(buff, sfdp + 4, bufflen);
	}

	return err;
}


static int _flash_dump_sfdp(const spiflash_t *flash)
{
	int err = EOK;
	uint32_t header[2];

	if ((err = _flash_enable(flash)) < 0) {
		fprintf(stderr, "\nu3nordet: cannot enable the Flash chip, failed with %d!\n", err);
		goto exit;
	}

	if ((err = _flash_read_sfdp(flash, 0x0, header, sizeof(header))) < 0) {
		fprintf(stderr, "\nu3nordet: cannot read SFDP header, failed with %d!\n", err);
		goto exit;
	}

	if (header[0] != 0x50444653UL) {
		printf("u3nordet: SFDP not supported\n");
		goto exit;
	}
	printf("u3nordet: supports SFDP %u.%u\n", (header[1] >> 8) & 0xff, header[1] & 0xff);

exit:
	_flash_disable(flash);
	return err;
}


void u3nordet_info(void)
{
	printf("detect SPI NOR Flash chips");
}

#define PARSE_PIN_OR_FAIL(name, out) \
	if (u3pin_parse((name), &(out)) < 0) { \
		fprintf(stderr, "\nu3nordet: invalid pin %s for " #out "!\n", name); \
		return EXIT_FAILURE; \
	}


int u3nordet_run(int argc, char **argv)
{
	int err;
	spiflash_t flash;

	if (argc != 7) {
		printf("usage: u3nordet spi cs sck miso mosi wphold\n");
		return (argc < 2) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	flash.spi = atoi(argv[1]) - 1;
	if ((flash.spi < spi1) || (flash.spi > spi3)) {
		fprintf(stderr, "\nu3nordet: invalid SPI %s!\n", argv[1]);
		return EXIT_FAILURE;
	}

	PARSE_PIN_OR_FAIL(argv[2], flash.cs);
	PARSE_PIN_OR_FAIL(argv[3], flash.sck);
	PARSE_PIN_OR_FAIL(argv[4], flash.miso);
	PARSE_PIN_OR_FAIL(argv[5], flash.mosi);
	PARSE_PIN_OR_FAIL(argv[6], flash.wp_hold);

	if ((err = _flash_init(&flash)) < 0) {
		fprintf(stderr, "\nu3nordet: _flash_init failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	if ((err = _flash_detect(&flash)) < 0) {
		fprintf(stderr, "\nu3nordet: _flash_detect failed with %d!\n", err);
		return EXIT_FAILURE;
	}
	printf("u3nordet: %06x\n", err);

	if ((err = _flash_dump_sfdp(&flash)) < 0) {
		fprintf(stderr, "\nu3nordet: _flash_dump_sfdp failed with %d!\n", err);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void __attribute__((constructor)) u3nordet_registerapp(void)
{
	static psh_appentry_t app_u3nordet = { .name = "u3nordet", .run = u3nordet_run, .info = u3nordet_info };

	psh_registerapp(&app_u3nordet);
}
