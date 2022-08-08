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

char *get_a_subbuffer_copy(char *fb, int X, int Y, int xoff, int yoff, int h, int v, int bpp);
void blank_a_buffer_region(char *fb, int X, int Y, int x_off, int y_off, int h, int v, int bpp);
void paint_a_buffer_region_tricolor(char *fb, int X, int Y, int x_off, int y_off, int h, int v, int bpp);
void paint_a_buffer_white(char *fb, int X, int Y, int bpp);
void paint_buffer_tricolor(char *fb, int xres, int yres, int bytes_pp);