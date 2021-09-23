#include <linux/fb.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

static inline
uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo *vinfo)
{
	return (r<<vinfo->red.offset) |
		(g<<vinfo->green.offset) |
		(b<<vinfo->blue.offset);
}

int main()
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;
	long screensize;
	uint8_t *fbp;
	int fb_fd;
	int x,y;

	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) {
		printf("Failed to open fbdev 0\n");
		return -1;
	}

	/* Get variable screen info */
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	vinfo.grayscale=0;
	vinfo.bits_per_pixel=32;

	/* Set variable screen info for new BPP */
	ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);

	/* Get new variable info */
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);

	screensize = vinfo.yres_virtual * finfo.line_length;
	if (!screensize) {
		printf("Zero screen size, pitch=%d y=%d\n", finfo.line_length, vinfo.yres_virtual);
		close(fb_fd);
		return -2;
	}

	fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	if (!fbp) {
		printf("mmap failed\n");
		close(fb_fd);
		return -3;
	}

	/* Draw the tricolor */
	for (x = 0; x < vinfo.xres; x++) {

		for (y = 0; y < vinfo.yres/3; y++) {
			long location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
					(y+vinfo.yoffset) * finfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel_color(0xFF, 0x00, 0x00, &vinfo);
		}

		for (y = vinfo.yres/3; y < (2 * vinfo.yres)/3; y++) {
			long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
					(y+vinfo.yoffset) * finfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel_color(0x00, 0xFF, 0x00, &vinfo);
		}

		for (y = (2 * vinfo.yres)/3; y < vinfo.yres; y++) {
			long location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
					(y+vinfo.yoffset) * finfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel_color(0x00, 0x00, 0xFF, &vinfo);
		}

	}

	munmap(fbp, screensize);
	close(fb_fd);
	printf("Fbdev done\n");
	return 0;
}
