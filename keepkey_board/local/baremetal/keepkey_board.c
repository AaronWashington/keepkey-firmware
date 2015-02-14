/* START KEEPKEY LICENSE */
/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2015 KeepKey LLC
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/* END KEEPKEY LICENSE */

#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f2/rng.h>
#include <keepkey_board.h>

/* Static and Global variables */
/* stack smashing protector (SSP) canary value storage */
uintptr_t __stack_chk_guard;

/*
 * __stack_chk_fail() - stack smashing protector (SSP) call back funcation for -fstack-protector-all GCC option
 *
 * INPUT  - none
 * OUTPUT - none
 */
__attribute__((noreturn)) void __stack_chk_fail(void)
{
    int cnt = 0;
	layout_warning("Error Dectected.  Reboot Device!");
    display_refresh();
	do{
        if(cnt % 5 == 0) {
            animate();
            display_refresh();
        }
    }while(1); //loop forever
}

/*
 * board_reset() - Request board reset
 *
 * INPUT  - none
 * OUTPUT - none
 */
void board_reset(void)
{
    scb_reset_system();
}

/*
 * void clock_init() - Initialize clocks to enable peripherals
 *
 * INPUT - none
 * OUTPUT - none
 */
static void clock_init(void)
{
    // setup clock
    clock_scale_t clock = hse_8mhz_3v3[CLOCK_3V3_120MHZ];
    rcc_clock_setup_hse_3v3(&clock);


    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_OTGFS);
    rcc_periph_clock_enable(RCC_SYSCFG);
    rcc_periph_clock_enable(RCC_TIM4);
    rcc_periph_clock_enable(RCC_RNG);
    
}
 
/*
 * board_init() - Initialize board
 *
 * INPUT - none
 * OUTPUT - none
 */
void board_init(void)
{
    clock_init();

    /*
     * Enable random
     */
    RNG_CR |= RNG_CR_IE | RNG_CR_RNGEN;

    timer_init();
    keepkey_leds_init();
    keepkey_button_init();
    display_init();
    layout_init(display_canvas());
}
