/******************************************************************************
    Copyright (c) __20xx __Client_Name. All rights reserved.
    Developed for __Client_Name by Carbon Design Group.
******************************************************************************/

/// @file keepkey_display.c
/// __One_line_description_of_file.
///
/// __Detailed_description_of_file.


//================================ INCLUDES ===================================

#include <stddef.h>
#include "draw.h"
#include "font.h"


//====================== CONSTANTS, TYPES, AND MACROS =========================


//=============================== VARIABLES ===================================


//=============================== FUNCTIONS ===================================


static bool draw_char_with_shift(Canvas* canvas, char c, DrawableParams* p,
		int* x_shift, int* y_shift, const CharacterImage* img)
{
    bool success = false;

    int start_index = ( p->y * canvas->width ) + p->x;
    uint8_t* canvas_pixel = &canvas->buffer[ start_index ];

    // Check that this was a character that we have in the font.
    if( img != NULL )
    {

        // Check that it's within bounds.
        if( ( ( img->width + p->x ) <= canvas->width ) &&
            ( ( img->height + p->y ) <= canvas->height ) )
        {
            const uint8_t* img_pixel = &img->data[ 0 ]; 

            int y;
            for( y = 0; y < img->height; y++ )
            {
                int x;
                for( x = 0; x < img->width; x++ )
                {
                    *canvas_pixel = ( *img_pixel == 0x00 ) ? p->color : *canvas_pixel;
                    canvas_pixel++;
                    img_pixel++;
                }
                canvas_pixel += ( canvas->width - img->width );
            }

            if( x_shift != NULL )
            {
                *x_shift += img->width;
            }

            if( y_shift != NULL )
            {
                *y_shift += img->height;
            }

            success = true;
        }
    }

    canvas->dirty = true;

    return success;
}


bool draw_body_char(Canvas* canvas, char c, DrawableParams* params)
{
	const CharacterImage* img = body_font_get_char(c);
    return draw_char_with_shift(canvas, c, params, NULL, NULL, img);
}

bool draw_title_char(Canvas* canvas, char c, DrawableParams* params)
{
	const CharacterImage* img = body_font_get_char(c);
    return draw_char_with_shift(canvas, c, params, NULL, NULL, img);
}

bool draw_title_string(Canvas* canvas, const char* c, DrawableParams* p, int width, int line_height)
{
	return draw_string_helper(canvas, c, p, width, line_height, &title_font_get_char);
}

bool draw_body_string(Canvas* canvas, const char* c, DrawableParams* p, int width, int line_height)
{
	return draw_string_helper(canvas, c, p, width, line_height, &body_font_get_char);
}

bool draw_string_helper(Canvas* canvas, const char* c, DrawableParams* p, int width, int line_height,
		const CharacterImage* (*font_get_char)(char))
{
    bool have_space = true;

    int x_offset = 0;

    DrawableParams char_params = *p;

    while( *c && have_space )
    {
    	/*
    	 * Determine if we need a line break
    	 */
    	if((width != 0) && (x_offset + p->x > width))
    	{
    		char_params.y += line_height;
    		x_offset = 0;
    	}

    	/*
    	 * Remove spaces that start new lines
    	 */
    	if(x_offset == 0 && &c == ' ')
    		continue;

    	/*
    	 * Draw Character
    	 */
    	const CharacterImage* img = (*font_get_char)( *c );
        char_params.x = x_offset + p->x;
        have_space = draw_char_with_shift(
                canvas,
                *c,
                &char_params,
                &x_offset,
                NULL,
                img);
        c++;
    }

    canvas->dirty = true;

    return have_space;
}


bool draw_box(Canvas* canvas, BoxDrawableParams* p)
{
    int start_row = p->base.y;
    int end_row = start_row + p->height;
    end_row = ( end_row >= canvas->height ) ? canvas->height - 1 : end_row; 
    
    int start_col = p->base.x;
    int end_col = p->base.x + p->width;
    end_col = ( end_col >= canvas->width ) ? canvas->width - 1 : end_col; 

    int start_index = ( start_row * canvas->width ) + start_col;
    uint8_t* canvas_pixel = &canvas->buffer[ start_index ];

    int height = end_row - start_row;
    int width = end_col - start_col;

    int y;
    for( y = 0; y < height; y++ )
    {
        int x;
        for( x = 0; x < width; x++ )
        {
            *canvas_pixel = p->base.color;
            canvas_pixel++;
        }
        canvas_pixel += ( canvas->width - width );
    }

    canvas->dirty = true;
    
    return true;
}
