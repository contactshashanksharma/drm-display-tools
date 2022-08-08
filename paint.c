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

enum color {
	black = 0,
	red,
	green,
	blue,
	white,
};

uint32_t clr_val[] = {
	0, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFFFFFFFF
};

static void paint_buffer_single_color(char *buf, int bsz, int bpp, uint32_t val)
{
	int *ib = (int *)buf;
	int n_pixels = bsz/bpp;

	wmemset(ib, val, n_pixels);
}

static void paint_a_line(char *buf, int pitch, uint32_t val)
{
	int n_pixels = pitch/4;
	int *ib = (int *)buf;

	wmemset(ib, val, n_pixels);
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
	for (j = 0; j < v; j++)
		paint_a_line(sb + j * pitch, sb_pitch, clr_val[black]);
}

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

void paint_a_buffer_white(char *fb, int X, int Y, int bpp)
{
	paint_a_buffer_region_solid(fb, X, Y, 0, 0, X, Y, bpp, clr_val[white]);
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

	for (j = 0; j < v/3; j++)
		paint_a_line(sb + j * pitch, sb_pitch, clr_val[red]);
	for (j = v/3; j < (2 * v)/3; j++)
		paint_a_line(sb + j * pitch, sb_pitch, clr_val[green]);
	for (j = (2 * v)/ 3; j < v; j++)
		paint_a_line(sb + j * pitch, sb_pitch, clr_val[blue]);
}

void paint_buffer_tricolor(char *fb, int xres, int yres, int bytes_pp)
{
	int ret = 0;
	int pitch = xres * bytes_pp;
	int buf_sz = yres * pitch;
	int granularity = yres/3;

	paint_buffer_single_color(fb, buf_sz/3, bytes_pp, clr_val[red]);
	paint_buffer_single_color(fb + buf_sz/3, buf_sz/3, bytes_pp, clr_val[green]);
	paint_buffer_single_color(fb + 2 * buf_sz/3, buf_sz/3, bytes_pp, clr_val[blue]);

}