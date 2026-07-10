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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/platform.h>
#include <phoenix/arch/armv8m/stm32/u3/stm32u3.h>

#include "../psh.h"

#if defined(PCTL_HAS_ITRACE)
static const char *const IRQS[] = {
	[0] = "InitialSP",
	"Reset",
	"NMI",
	"HardFault",
	"MemMgtFault",
	"BusFault",
	"UsageFault",
	[11] = "SVC",
	"Debug",
	[14] = "PendSV",
	"SysTick",
	"wwdg",
	"pvd_pvm",
	"rtc",
	"rtc_s",
	"tamp",
	"ramcfg",
	"flash",
	"flash_s",
	"gtzc",
	"rcc",
	"rcc_s",
	"exti0",
	"exti1",
	"exti2",
	"exti3",
	"exti4",
	"exti5",
	"exti6",
	"exti7",
	"exti8",
	"exti9",
	"exti10",
	"exti11",
	"exti12",
	"exti13",
	"exti14",
	"exti15",
	"iwdg",
	"saes",
	"gpdma1_ch0",
	"gpdma1_ch1",
	"gpdma1_ch2",
	"gpdma1_ch3",
	"gpdma1_ch4",
	"gpdma1_ch5",
	"gpdma1_ch6",
	"gpdma1_ch7",
	"adc1",
	"dac1",
	"fdcan1_it0",
	"fdcan1_it1",
	"tim1_brk_terr_ierr",
	"tim1_up",
	"tim1_trg_com_dir_idx",
	"tim1_cc",
	"tim2",
	"tim3",
	"tim4",
	[65] = "tim6",
	"tim7",
	"tim12",
	[69] = "i3c1_ev",
	"i3c1_er",
	"i2c1_ev",
	"i2c1_er",
	"i2c2_ev",
	"i2c2_er",
	"spi1",
	"spi2",
	"usart1",
	"usart2",
	"usart3",
	"uart4",
	"uart5",
	"lpuart1",
	"lptim1",
	"lptim2",
	"tim15",
	"tim16",
	"tim17",
	"comp",
	"usb_fs",
	"crs",
	[92] = "octospi1",
	"hsp1",
	"sdmmc1",
	[96] = "gpdma1_ch8",
	"gpdma1_ch9",
	"gpdma1_ch10",
	"gpdma1_ch11",
	[104] = "i2c3_ev",
	"i2c3_er",
	"sai1",
	[108] = "tsc",
	"aes",
	"rng",
	"fpu",
	"hash",
	"pka",
	"lptim3",
	"spi3",
	"i3c2_ev",
	"i3c2_er",
	"tim8_brk_terr_ierr",
	"tim8_up",
	"tim8_trg_com_dir_idx",
	"tim8_cc",
	[123] = "icache",
	[126] = "lptim4",
	[128] = "adf1",
	"adc2",
	"fdcan2_it0",
	"fdcan2_it1",
	"i2c4_ev",
	"i2c4_er",
	[135] = "spi4",
	[139] = "pwr",
	"pwr_s",
};


static struct {
	bool stop;
} u3imon_common;


static void u3imon_signalint(int sig)
{
	u3imon_common.stop = true;
}
#endif


void u3imon_info(void)
{
	printf("monitor STM32U3 interrupts");
}


int u3imon_run(int argc, char **argv)
{
#if defined(PCTL_HAS_ITRACE)
	u3imon_common.stop = false;
	signal(SIGINT, u3imon_signalint);

	while (!u3imon_common.stop) {
		int status = EOK;
		platformctl_t pctl = {
			.action = pctl_get,
			.type = pctl_iTrace,
		};

		if (EOK != (status = platformctl(&pctl))) {
			fprintf(stderr, "u3imon: pctl_iTrace failed (%d)\n", status);
			return EXIT_FAILURE;
		}

		printf("\033[2J\033[30;47m%-20s  %6s  %6s\033[0m\n", "IRQ", "Count", "Ticks");
		for (unsigned i = 0; i < pctl.iTrace.sz; i++) {
			if (pctl.iTrace.counters[i]) {
				printf("%-20s  %6u  %6lld\n", IRQS[i], pctl.iTrace.counters[i], pctl.iTrace.ticks[i]);
			}
		}
		printf("\033[1m%-20s  %6u  %6lld\033[0m\n", "TOTAL", pctl.iTrace.counters[pctl.iTrace.sz], pctl.iTrace.ticks[pctl.iTrace.sz]);

		printf("\n\n\033[30;47m 0123456789ABCDEF\033[0m\n");
		for (unsigned row = 0, last = (pctl.iTrace.sz - 16 + 15) / 16; row < last; row++) {
			printf("\033[30;47m%X\033[0m", 1 + row);
			for (unsigned col = 0; col < 16; col++) {
				unsigned long reg = pctl.iTrace.enabled[row / 2];
				reg >>= (row % 2) ? col + 16 : col;

				if ((row == (last - 1)) && (col >= (pctl.iTrace.sz % 16))) {
					break;
				}
				printf(pctl.iTrace.counters[(row + 1) * 16 + col] ? "\033[32m%c\033[0m" : "%c", (reg & 1) ? 'x' : '.');
			}
			printf("\n");
		}

		for (unsigned i = 0; i < pctl.iTrace.sz; i++) {
			if (pctl.iTrace.counters[i]) {
				pctl.iTrace.counters[i] = 0;
				pctl.iTrace.ticks[i] = 0;
			}
		}

		pctl.iTrace.counters[pctl.iTrace.sz] = 0;
		pctl.iTrace.ticks[pctl.iTrace.sz] = 0;

		sleep(1);
	}

	return EXIT_SUCCESS;
#else
	fprintf(stderr, "\nu3imon: built without PCTL_HAS_ITRACE!\n");
	return EXIT_FAILURE;
#endif
}


void __attribute__((constructor)) u3imon_registerapp(void)
{
	static psh_appentry_t app_u3imon = { .name = "u3imon", .run = u3imon_run, .info = u3imon_info };

	psh_registerapp(&app_u3imon);
}
