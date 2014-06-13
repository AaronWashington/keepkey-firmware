/******************************************************************************
    Copyright (c) __20xx __Client_Name. All rights reserved.
    Developed for __Client_Name by Carbon Design Group.
******************************************************************************/

/// @file keepkey_display.c
/// __One_line_description_of_file.
///
/// __Detailed_description_of_file.


//================================ INCLUDES ===================================


#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "keepkey_display.h"
#include "pin.h"
#include "timer.h"


//====================== CONSTANTS, TYPES, AND MACROS =========================


#define START_COL ((uint8_t)0x1C)
#define START_ROW ((uint8_t)0x00)


//=============================== VARIABLES ===================================



static const Pin DATA_0_PIN    = { GPIOA, GPIO0 };
static const Pin DATA_1_PIN    = { GPIOA, GPIO1 };
static const Pin DATA_2_PIN    = { GPIOA, GPIO2 };
static const Pin DATA_3_PIN    = { GPIOA, GPIO3 };
static const Pin DATA_4_PIN    = { GPIOA, GPIO4 };
static const Pin DATA_5_PIN    = { GPIOA, GPIO5 };
static const Pin DATA_6_PIN    = { GPIOA, GPIO6 };
static const Pin DATA_7_PIN    = { GPIOA, GPIO7 };

static const Pin nOE_PIN       = { GPIOA, GPIO8 };
static const Pin nWE_PIN       = { GPIOA, GPIO9 };
static const Pin nDC_PIN       = { GPIOB, GPIO1 };

static const Pin nSEL_PIN      = { GPIOA, GPIO10 };
static const Pin nRESET_PIN    = { GPIOB, GPIO5 };

static const Pin BACKLIGHT_PWR_PIN = { GPIOB, GPIO0 };


static uint8_t canvas_buffer[ KEEPKEY_DISPLAY_HEIGHT * KEEPKEY_DISPLAY_WIDTH ];
static Canvas canvas;


//====================== PRIVATE FUNCTION DECLARATIONS ========================



//-----------------------------------------------------------------------------
/// Write a frame buffer to the display.
/// 
//-----------------------------------------------------------------------------
static void
display_configure_io (
        void
);


//-----------------------------------------------------------------------------
/// Reset IO to do nothing.
/// 
//-----------------------------------------------------------------------------
static void
display_reset_io(
        void
);


//-----------------------------------------------------------------------------
/// Reset the display.
/// 
//-----------------------------------------------------------------------------
static void
display_reset(
        void
);


//-----------------------------------------------------------------------------
/// Write to display ram
///
//-----------------------------------------------------------------------------
static void
display_prepare_gram_write(
        void
);


//-----------------------------------------------------------------------------
/// Write to a display reg
///
//-----------------------------------------------------------------------------
static void
display_write_reg (
        uint8_t reg
);


//-----------------------------------------------------------------------------
/// Write to display ram
///
//-----------------------------------------------------------------------------
static void
display_write_ram(
        uint8_t val 
);


//=============================== FUNCTIONS ===================================


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
Canvas*
display_init(
        void
)
{
    // Prepare the canvas
    canvas.buffer   = canvas_buffer;
    canvas.width    = KEEPKEY_DISPLAY_WIDTH;
    canvas.height   = KEEPKEY_DISPLAY_HEIGHT;
    canvas.dirty    = false;

    display_configure_io();

    CLEAR_PIN( BACKLIGHT_PWR_PIN );

    display_reset();

    display_write_reg( (uint8_t)0xFD ); 
    display_write_ram( (uint8_t)0x12 ); 

    display_turn_off();

    // Divide DIVSET by 2?
    display_write_reg( (uint8_t)0xB3 ); 
    display_write_ram( (uint8_t)0x91 ); 

    display_write_reg( (uint8_t)0xCA ); 
    display_write_ram( (uint8_t)0x3F ); 

    display_write_reg( (uint8_t)0xA2 ); 
    display_write_ram( (uint8_t)0x00 ); 

    display_write_reg( (uint8_t)0xA1 ); 
    display_write_ram( (uint8_t)0x00 ); 


    uint8_t row_start = START_ROW;
    uint8_t row_end = row_start + 64 - 1;

    // Width is in units of 4 pixels/column (2 bytes at 4 bits/pixel)
    int width = ( 256 / 4 );
    uint8_t col_start = START_COL;
    uint8_t col_end = col_start + width - 1;

    display_write_reg( (uint8_t)0x75 );
    display_write_ram( row_start );
    display_write_ram( row_end );
    display_write_reg( (uint8_t)0x15 );
    display_write_ram( col_start );
    display_write_ram( col_end );

    // Horizontal address increment
    // Disable colum address re-map
    // Disable nibble re-map
    // Scan from COM0 to COM[n-1]
    // Disable dual COM mode
    display_write_reg( (uint8_t)0xA0 ); 
    //display_write_ram( (uint8_t)0x00;
    //display_write_ram( (uint8_t)0x01;
    display_write_ram( (uint8_t)0x14 ); 
    display_write_ram( (uint8_t)0x11 ); 

    // GPIO0: pin HiZ, Input disabled
    // GPIO1: pin HiZ, Input disabled
    display_write_reg( (uint8_t)0xB5 ); 
    display_write_ram( (uint8_t)0x00 ); 

    // Enable internal Vdd regulator?
    display_write_reg( (uint8_t)0xAB ); 
    display_write_ram( (uint8_t)0x01 ); 

    // 
    display_write_reg( (uint8_t)0xB4 ); 
    display_write_ram( (uint8_t)0xA0 ); 
    display_write_ram( (uint8_t)0xFD ); 

    display_set_brightness( DEFAULT_DISPLAY_BRIGHTNESS );

    display_write_reg( (uint8_t)0xC7 ); 
    display_write_ram( (uint8_t)0x0F ); 

    display_write_reg( (uint8_t)0xB9 ); 

    display_write_reg( (uint8_t)0xB1 ); 
    display_write_ram( (uint8_t)0xE2 ); 

    display_write_reg( (uint8_t)0xD1 ); 
    display_write_ram( (uint8_t)0x82 ); 
    display_write_ram( (uint8_t)0x20 ); 

    display_write_reg( (uint8_t)0xBB ); 
    display_write_ram( (uint8_t)0x1F ); 

    display_write_reg( (uint8_t)0xB6 ); 
    display_write_ram( (uint8_t)0x08 ); 

    display_write_reg( (uint8_t)0xBE ); 
    display_write_ram( (uint8_t)0x07 ); 

    display_write_reg( (uint8_t)0xA6 ); 

    delay( 10 );


    // Set the screen to display-writing mode
    display_prepare_gram_write();

    delay( 10 );

    // Make the display blank
    int end = 64  * 256;
    int i;
    for( i = 0; i < end; i += 2 )
    {
        display_write_ram( (uint8_t)0x00 );
    }

    // Turn on 12V
    SET_PIN( BACKLIGHT_PWR_PIN );

    delay( 100 );

    display_turn_on();

    return &canvas;
}


//-----------------------------------------------------------------------------
/// See keepkey_display.h
///
//-----------------------------------------------------------------------------
Canvas*
display_canvas(
        void
)
{
    return &canvas;
}


//-----------------------------------------------------------------------------
/// Initialize the GPIO necessary for the display and show a blank screen.
///
//-----------------------------------------------------------------------------
void
display_refresh(
        void
)
{
    display_prepare_gram_write();

    int num_writes = canvas.width * canvas.height;

    int i;
    for( i = 0; i < num_writes; i += 2 )
    {
        uint8_t v = ( 0xF0 & canvas.buffer[ i ] ) | ( canvas.buffer[ i + 1 ] >> 4 );
        display_write_ram( v );
    }

    canvas.dirty = false;
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
void
display_set_brightness(
        int percentage
)
{
    // Set the brightness..
    int v = percentage;

    // Clip to be 0 <= value <= 100
    v = ( v >= 0 ) ? v : 0;
    v = ( v > 100 ) ? 100 : v;

    v = ( 0xFF * v ) / 100;

    uint8_t reg_value = (uint8_t)v;

    display_write_reg( (uint8_t)0xC1 );
    display_write_ram( reg_value );
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
void
display_turn_on(
        void
)
{
    display_write_reg( (uint8_t)0xAF );
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
void
display_turn_off(
        void
)
{
    display_write_reg( (uint8_t)0xAE );
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_reset(
        void
)
{
    CLEAR_PIN( nRESET_PIN );

    delay( 10 );

    SET_PIN( nRESET_PIN );

    delay( 50 );
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_reset_io(
        void
)
{
    SET_PIN( nRESET_PIN );
    CLEAR_PIN( BACKLIGHT_PWR_PIN );
    SET_PIN( nWE_PIN );
    SET_PIN( nOE_PIN );
    SET_PIN( nDC_PIN );
    SET_PIN( nSEL_PIN );

    GPIO_BSRR( GPIOA ) = 0x00FF0000;
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_configure_io(
        void
)
{
    // Set up port A
    gpio_mode_setup( 
            GPIOA, 
            GPIO_MODE_OUTPUT, 
            GPIO_PUPD_NONE, 
            GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 | GPIO10 );

    gpio_set_output_options(
            GPIOA,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_100MHZ,
            GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9 | GPIO10 );

    // Set up port B
    gpio_mode_setup( 
            GPIOB, 
            GPIO_MODE_OUTPUT, 
            GPIO_PUPD_NONE, 
            GPIO0 | GPIO1 | GPIO5 );

    gpio_set_output_options(
            GPIOB,
            GPIO_OTYPE_PP,
            GPIO_OSPEED_100MHZ,
            GPIO0 | GPIO1 | GPIO5 );

    // Set to defaults
    display_reset_io();
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_prepare_gram_write(
        void
)
{
    display_write_reg( (uint8_t)0x5C );
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_write_reg (
        uint8_t reg
)
{
    // Unsure about nDC

    // Set up the data
    GPIO_BSRR( GPIOA ) = 0x000000FF & (uint32_t)reg;

    // Set nOLED_SEL low, nMEM_OE high, and nMEM_WE high.
    CLEAR_PIN( nSEL_PIN );
    SET_PIN( nOE_PIN );
    SET_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");

    // Set nDC low?
    CLEAR_PIN( nDC_PIN );

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");

    // Set nMEM_WE low
    CLEAR_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");

    // Set nMEM_WE high
    SET_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");

    // Set nOLED_SEL high
    SET_PIN( nSEL_PIN );
    GPIO_BSRR( GPIOA ) = 0x00FF0000;

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
}


//-----------------------------------------------------------------------------
// See keepkey_display.h for public interface.
//
static void
display_write_ram(
        uint8_t val 
)
{
    // Set up the data
    GPIO_BSRR( GPIOA ) = 0x000000FF & (uint32_t)val;

    // Set nOLED_SEL low, nMEM_OE high, and nMEM_WE high.
    CLEAR_PIN( nSEL_PIN );
    SET_PIN( nOE_PIN );
    SET_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");

    // Set nDC high?
    SET_PIN( nDC_PIN );

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");

    // Set nMEM_WE low
    CLEAR_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");

    // Set nMEM_WE high
    SET_PIN( nWE_PIN );

    __asm__("nop");
    __asm__("nop");

    // Set nOLED_SEL high
    SET_PIN( nSEL_PIN );
    GPIO_BSRR( GPIOA ) = 0x00FF0000;

    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
    __asm__("nop");
}
