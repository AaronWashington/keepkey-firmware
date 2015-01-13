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

//============================= CONDITIONALS ==================================


#ifndef timer_H
#define timer_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
	

/******************** #defines *************************************/
#define ONE_SEC 1000 
#define HALF_SEC 500
#define MAX_RUNNABLES 3

/******************** Typedefs and enums ***************************/
typedef void (*Runnable)(void* context);
typedef struct RunnableNode RunnableNode;
struct RunnableNode
{
    uint32_t    remaining;
    Runnable    runnable;
    void        *context;
    uint32_t    period;
    bool        repeating;
    RunnableNode* next;
};

typedef struct
{
    RunnableNode*   head;
    int             size;
} RunnableQueue;



/******************** Function Declarations ***********************/
void timer_init(void);
void delay_ms(uint32_t ms);
void post_delayed(Runnable runnable, void *context, uint32_t ms_delay);
void post_periodic(Runnable runnable, void *context, uint32_t period_ms, uint32_t delay_ms);
void remove_runnable(Runnable runnable);
void clear_runnables(void);

#ifdef __cplusplus
}
#endif

#endif // timer_H

