/* START KEEPKEY LICENSE */
/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2014 Carbon Design Group <tom@carbondesign.com>
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

/*
 * @brief General confirmation state machine.
 */

//================================ INCLUDES =================================== 
#include <assert.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <libopencm3/cm3/cortex.h>

#include <keepkey_display.h>
#include <keepkey_button.h>
#include <timer.h>
#include <layout.h>

#include <confirm_sm.h>

//====================== CONSTANTS, TYPES, AND MACROS =========================
typedef enum
{
    HOME,
    CONFIRM_WAIT,
    ABORTED,
    CONFIRMED,
    FINISHED
} DisplayState;

typedef enum
{
    LAYOUT_REQUEST,
    LAYOUT_CONFIRM_ANIMATION,
    LAYOUT_ABORTED,
    LAYOUT_CONFIRMED,
    LAYOUT_FINISHED,
    LAYOUT_NUM_LAYOUTS,
    LAYOUT_INVALID
} ActiveLayout;

/**
 * Define the given layout dialog texts for each screen
 */
#define NUM_LINES 4
typedef struct
{
    const char* str;
    uint8_t color;
} ScreenLine;
typedef ScreenLine ScreenLines[NUM_LINES];
typedef ScreenLines DialogLines[LAYOUT_NUM_LAYOUTS];

typedef struct 
{
    DialogLines lines;
    DisplayState display_state;
    ActiveLayout active_layout;
} StateInfo;

/*
 * The number of milliseconds to wait for a confirmation
 */
#define CONFIRM_TIMEOUT_MS (2000)

//=============================== VARIABLES ===================================

//====================== PRIVATE FUNCTION DECLARATIONS ========================
static void handle_screen_press( void* context);
static void handle_screen_release( void* context);
static void handle_confirm_timeout( void* context );

//=============================== FUNCTIONS ===================================

/**
 * Handles user key-down event.
 */
static void handle_screen_press( void* context)
{
    assert(context != NULL);

    StateInfo *si = (StateInfo*)context;

    switch( si->display_state )
    {
        case HOME:
            si->active_layout = LAYOUT_CONFIRM_ANIMATION;
            si->display_state = CONFIRM_WAIT;
            break;

        default:
            break;
    }
}

/**
 * Handles the scenario in which a button is released.  During the confirmation period, this
 * indicates the user stopped their confirmation/aborted.
 *
 * @param context unused
 */
static void handle_screen_release( void* context)
{
    assert(context != NULL);

    StateInfo *si = (StateInfo*)context;

    switch( si->display_state )
    {
        case CONFIRM_WAIT:
            si->active_layout = LAYOUT_ABORTED;
            si->display_state = ABORTED;
            break;

        case CONFIRMED:
            si->active_layout = LAYOUT_FINISHED;
            si->display_state = FINISHED;
            break;

        default:
            break;
    }
}

/**
 * This is the success path for confirmation, and indicates that the confirmation timer
 * has expired.
 *
 * @param context The state info used to apss info back to the user.
 */

static void handle_confirm_timeout( void* context )
{
    assert(context != NULL);

    StateInfo *si = (StateInfo*)context;
    si->display_state = CONFIRMED;
    si->active_layout = LAYOUT_CONFIRMED;
}

void swap_layout(ActiveLayout active_layout, volatile StateInfo* si)
{
    switch(active_layout)
    {
    	case LAYOUT_REQUEST:
    		break;
    	case LAYOUT_CONFIRM_ANIMATION:
    		post_delayed( &handle_confirm_timeout, (void*)si, CONFIRM_TIMEOUT_MS );
    		break;
    	case LAYOUT_ABORTED:
    		remove_runnable( &handle_confirm_timeout );
    		break;
    	case LAYOUT_CONFIRMED:
    		remove_runnable( &handle_confirm_timeout );
    		break;
    	case LAYOUT_FINISHED:
    		break;
    	default:
    		assert(0);
    };
    layout_standard_notification(si->lines[active_layout][0].str, si->lines[active_layout][1].str);
}

bool confirm(const char *request, ...)
{
    bool ret=false;

    va_list vl;
    va_start(vl, request);
    char strbuf[layout_char_width()+1];
    vsnprintf(strbuf, sizeof(strbuf), request, vl);
    va_end(vl);

    volatile StateInfo state_info;
    memset((void*)&state_info, 0, sizeof(state_info));
    state_info.display_state = HOME;
    state_info.active_layout = LAYOUT_REQUEST;
    state_info.lines[LAYOUT_REQUEST][0].str = request;
    state_info.lines[LAYOUT_REQUEST][0].color = DATA_COLOR;
    state_info.lines[LAYOUT_REQUEST][1].str = "Press and hold button to confirm...";
    state_info.lines[LAYOUT_REQUEST][1].color = LABEL_COLOR;
    state_info.lines[LAYOUT_CONFIRMED][0].str = request;
    state_info.lines[LAYOUT_CONFIRMED][0].color = DATA_COLOR;
    state_info.lines[LAYOUT_CONFIRMED][1].str = "CONFIRMED";
    state_info.lines[LAYOUT_CONFIRMED][1].color = LABEL_COLOR;
    state_info.lines[LAYOUT_ABORTED][0].str = request;
    state_info.lines[LAYOUT_ABORTED][0].color = DATA_COLOR;
    state_info.lines[LAYOUT_ABORTED][1].str = "ABORTED";
    state_info.lines[LAYOUT_ABORTED][1].color = LABEL_COLOR;

    keepkey_button_set_on_press_handler( &handle_screen_press, (void*)&state_info );
    keepkey_button_set_on_release_handler( &handle_screen_release, (void*)&state_info );

    ActiveLayout cur_layout = LAYOUT_INVALID;
    while(1)
    {
        cm_disable_interrupts();
    		ActiveLayout new_layout = state_info.active_layout;
    		DisplayState new_ds = state_info.display_state;
        cm_enable_interrupts();

        if(cur_layout != new_layout)
        {
            swap_layout(new_layout, &state_info);
            cur_layout = new_layout;
        }

        display_refresh();
        animate();

        if(new_ds == FINISHED ||  new_ds == ABORTED)
        {
            ret =new_ds == FINISHED;
	    break;
        }
    }

    keepkey_button_set_on_press_handler( NULL, NULL );
    keepkey_button_set_on_release_handler( NULL, NULL );

    return ret;
}


