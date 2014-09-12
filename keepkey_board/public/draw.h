/******************************************************************************
    Copyright (c) __20xx __Client_Name. All rights reserved.
    Developed for __Client_Name by Carbon Design Group.
******************************************************************************/

/// @file __TemplateFile.h
/// __One_line_description_of_file
///
/// @page __Page_label __Page_title (__TemplateFile.h)
///
/// @section __1st_section_label __1st_section_title
///
/// __Section_body
///
/// @section __2nd_section_label __2nd_section_title
///
/// __Section_body
///
/// @see __Insert_cross_reference_here




//============================= CONDITIONALS ==================================


#ifndef draw_H
#define draw_H

#ifdef __cplusplus
extern "C" {
#endif


//=============================== INCLUDES ====================================


#include <stddef.h>
#include <stdbool.h>
#include "canvas.h"
#include "font.h"


//====================== CONSTANTS, TYPES, AND MACROS =========================


typedef struct
{
    uint8_t color;
    int     x;
    int     y;

} DrawableParams;


typedef struct
{
    DrawableParams  base;
    int             height;
    int             width;
} BoxDrawableParams;


//=============================== VARIABLES ===================================


//=============================== FUNCTIONS ===================================


//-----------------------------------------------------------------------------
/// Draw a character on the display.
///
//-----------------------------------------------------------------------------
bool
draw_char(
        Canvas*         canvas,
        char            c,
        DrawableParams* params
);


//-----------------------------------------------------------------------------
/// Draw a string on the display.
///
//-----------------------------------------------------------------------------
bool draw_title_string(Canvas* canvas, const char* c, DrawableParams* p, int width,
		int line_height);
bool draw_body_string(Canvas* canvas, const char* c, DrawableParams* p, int width,
		int line_height);
bool draw_string_helper(Canvas* canvas, const char* c, DrawableParams* p, int width,
		int line_height, const CharacterImage* (*font_get_char)(char));


//-----------------------------------------------------------------------------
/// Draw a box on the display.
///
//-----------------------------------------------------------------------------
bool
draw_box(Canvas* canvas, BoxDrawableParams*  params);


#ifdef __cplusplus
}
#endif

#endif // draw_H

