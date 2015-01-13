/* START KEEPKEY LICENSE */
/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2014 KeepKey LLC
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


//================================ INCLUDES ===================================

#include <stddef.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/f2/nvic.h>
#include <libopencm3/stm32/syscfg.h>

#include "keepkey_button.h"
#include "keepkey_leds.h"


//====================== CONSTANTS, TYPES, AND MACROS =========================


//=============================== VARIABLES ===================================


static Handler on_press_handler     = NULL;
static Handler on_release_handler   = NULL;

static void* on_release_handler_context = NULL;
static void* on_press_handler_context   = NULL;


static const uint16_t BUTTON_PIN    = GPIO7;
static const uint32_t BUTTON_PORT   = GPIOB;
static const uint8_t BUTTON_IRQN    = NVIC_EXTI9_5_IRQ;
static const uint32_t BUTTON_EXTI   = EXTI7;


//====================== PRIVATE FUNCTION DECLARATIONS ========================



//=============================== FUNCTIONS ===================================

void keepkey_button_init(void) {

    on_press_handler = NULL;
    on_press_handler_context = NULL;

    on_release_handler = NULL;
    on_release_handler_context = NULL;

    nvic_enable_irq( BUTTON_IRQN );

    gpio_mode_setup(
            BUTTON_PORT, 
            GPIO_MODE_INPUT, 
            GPIO_PUPD_NONE,
            BUTTON_PIN );

    /* Configure the EXTI subsystem. */
    exti_select_source(
            BUTTON_EXTI, 
            GPIOB );

    exti_set_trigger(
            BUTTON_EXTI, 
            EXTI_TRIGGER_BOTH );

    exti_enable_request( BUTTON_EXTI );
}

void keepkey_button_set_on_press_handler(Handler handler, void* context)
{
    on_press_handler = handler;
    on_press_handler_context = context;
}

void keepkey_button_set_on_release_handler(Handler handler, void* context)
{
    on_release_handler = handler;
    on_release_handler_context = context;
}

bool keepkey_button_up(void)
{
    uint16_t port = gpio_port_read(BUTTON_PORT);
    return port & BUTTON_PIN;
}

bool keepkey_button_down(void)
{
    return !keepkey_button_up();
}

void exti9_5_isr(void)
{
    exti_reset_request( BUTTON_EXTI );

    if( gpio_get( BUTTON_PORT, BUTTON_PIN ) & BUTTON_PIN )
    {
        if( on_release_handler )
        {
            on_release_handler( on_release_handler_context );
        }
    }
    else
    {
        if( on_press_handler )
        {
            on_press_handler( on_press_handler_context );
        }
    }
}

