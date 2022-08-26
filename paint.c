/*
 * Copyright 2022 Shashank Sharma (contactshashanksharma@gmail.com)
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>	
#include <malloc.h>
#include <wchar.h>
#include "paint.h"

static struct clr_hash_table *table;

/* ============ Color hash table related functions =========== */

void init_clr_hash(int num, uint32_t *clr_val)
{
	int i;

	table = malloc(sizeof(struct clr_hash_table));
	for (i = 0; i < num; i++) {
		table->cval[i] = malloc(sizeof(struct clr_hash_data));
		table->cval[i]->clr = i; //enum color
		table->cval[i]->val = clr_val[i];		
	}
	table->entries = num;
}

uint64_t hash_get_clr_val(int key)
{
	if (!table || key < 0 || key > table->entries) {
		printf("Invalid input\n");
		return -1;
	}

	return table->cval[key]->val;
}

void delete_clr_hash(int key)
{
	int i = 0;

	for (i = 0; i < table->entries; i++)
		free(table->cval[i]);
	free(table);
}


/* ============ Drawing functions =========== */

static void paint_a_line(char *buf, int pitch, uint32_t val)
{
	int n_pixels = pitch/4;
	int *ib = (int *)buf;

	wmemset(ib, val, n_pixels);
}

void _paint_x(char *buf, int xres, int x, uint32_t val)
{
	uint32_t *pixel = buf;

	if (x < xres) {
		pixel[x++] = val;
		_paint_x(buf, xres, x, val);
	}
}

void _paint_y(char *fb, int xres, int yres, int y, int bpp, uint32_t val)
{
	int pitch = xres * bpp;

	if (y < yres) {
		_paint_x(fb + y++ * pitch, xres, 0, val);
		_paint_y(fb, xres, yres, y, bpp, val);
	}
}

void paint_buf_recursively(char *fb, int xres, int yres, int bpp, int val)
{
	if (!fb || !xres || !yres)
		return;

	_paint_y(fb, xres, yres, 0, bpp, val);
}

void blank_a_buffer_region(char *fb, int X, int Y, int x_off, int y_off, int h, int v, int bpp)
{
	int ret = 0;
	int pitch = X * bpp;
	int bsz = Y * pitch;
	int sb_pitch = h * bpp;
	int sb_sz = v * sb_pitch;
	int j;
	char *sb;

	sb = fb + y_off * pitch + x_off * bpp;

#ifdef LINE_BY_LINE
	for (j = 0; j < v; j++)
		paint_a_line(sb + j * pitch, sb_pitch, hash_get_clr_val(red));
#else
	for (j = 0; j < v; j++)
		_paint_x(sb + j * pitch, h, 0, hash_get_clr_val(black));
#endif
}

#ifdef LINE_BY_LINE
void paint_a_buffer_region_solid(char *fb, int X, int Y, int x_off, int y_off, int h, int v, int bpp, int clr_val)
{
	int ret = 0;
	int pitch = X * bpp;
	int bsz = Y * pitch;
	int sb_pitch = h * bpp;
	int sb_sz = v * sb_pitch;
	int sb_clr_sz = sb_sz/3;
	int j;
	char *sb;

	memset(fb, 0, bsz);

	sb = fb + y_off * pitch + x_off * bpp;
	for (j = 0; j < v; j++)
		paint_a_line(sb + j * pitch, sb_pitch, clr_val);
}
#endif

void paint_a_buffer_white(char *fb, int X, int Y, int bpp)
{
#ifdef LINE_BY_LINE
	paint_a_buffer_region_solid(fb, X, Y, 0, 0, X, Y, bpp, hash_get_clr_val(red));
#else
	paint_buf_recursively(fb, X, Y, bpp, hash_get_clr_val(white));
#endif
}

char *get_a_subbuffer_copy(char *fb, int X, int Y, int xoff, int yoff, int h, int v, int bpp)
{
	char *output;
	char *sub;
	int pitch = X * bpp;
	int sb_pitch = h * bpp;
	int i;

	if (!fb || !X || !Y) {
		printf("Invalid input, cant get the buffer\n");
		return NULL;
	}

	output = malloc(v * sb_pitch);
	sub = fb + yoff * pitch + xoff * bpp;

	for (i = 0; i < v; i++)
		memcpy(output, sub + i * pitch, sb_pitch);

	return output;
}

void paint_a_buffer_region_tricolor(char *fb, int X, int Y, int x_off, int y_off, int h, int v, int bpp)
{
	int ret = 0;
	int pitch = X * bpp;
	int bsz = Y * pitch;
	int sb_pitch = h * bpp;
	int sb_sz = v * sb_pitch;
	int sb_clr_sz = sb_sz/3;
	int j;
	char *sb;

	memset(fb, 0, bsz);
	sb = fb + y_off * pitch + x_off * bpp;

#ifdef LINE_BY_LINE
	for (j = 0; j < v/3; j++)
		paint_a_line(sb + j * pitch, sb_pitch, hash_get_clr_val(red));
	for (j = v/3; j < (2 * v)/3; j++)
		paint_a_line(sb + j * pitch, sb_pitch, hash_get_clr_val(red));
	for (j = (2 * v)/ 3; j < v; j++)
		paint_a_line(sb + j * pitch, sb_pitch, hash_get_clr_val(red));
#else
	paint_buf_recursively(sb, X, v/3, 4, hash_get_clr_val(red));
	paint_buf_recursively(sb + pitch * v/3, X, v/3, 4, hash_get_clr_val(green));
	paint_buf_recursively(sb + pitch * 2 * v/3, X, v/3, 4, hash_get_clr_val(blue));
#endif
}

#ifdef LINE_BY_LINE
static void paint_buffer_single_color(char *buf, int bsz, int bpp, uint32_t val)
{
	int *ib = (int *)buf;
	int n_pixels = bsz/bpp;

	wmemset(ib, val, n_pixels);
}
#endif

void paint_buffer_tricolor(char *fb, int xres, int yres, int bytes_pp)
{
	int ret = 0;
	int pitch = xres * bytes_pp;
	int buf_sz = yres * pitch;
	int granularity = yres/3;

#ifdef LINE_BY_LINE
	paint_buffer_single_color(fb, buf_sz/3, bytes_pp, hash_get_clr_val(red));
	paint_buffer_single_color(fb + buf_sz/3, buf_sz/3, bytes_pp, hash_get_clr_val(red));
	paint_buffer_single_color(fb + 2 * buf_sz/3, buf_sz/3, bytes_pp, hash_get_clr_val(red));
#else
	paint_buf_recursively(fb, xres, yres/3, 4, hash_get_clr_val(red));
	paint_buf_recursively(fb + pitch * yres/3, xres, yres/3, 4, hash_get_clr_val(green));
	paint_buf_recursively(fb + pitch * 2 * yres/3, xres, yres/3, 4, hash_get_clr_val(blue));
#endif

}