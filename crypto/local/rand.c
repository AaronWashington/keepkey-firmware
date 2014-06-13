/**
 * Copyright (c) 2013 Tomas Dzetkulic
 * Copyright (c) 2013 Pavol Rusnak
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdio.h>

#include "rand.h"

static FILE *f = NULL;

void init_rand(void)
{
	f = fopen("/dev/urandom", "r");
}

uint32_t random32(void)
{
	uint32_t r;

        if(f == NULL) {
            init_rand();
        }

        assert(f != NULL);
	fread(&r, 1, sizeof(r), f);
	return r;
}

void random_buffer(uint8_t *buf, uint32_t len)
{
        if(f == NULL) {
            init_rand();
        }

	fread(buf, 1, len, f);
}
