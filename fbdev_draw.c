#include <linux/fb.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define pixel(r, g, b, varinfo) \
	(r << varinfo.red.offset) | \
	(g << varinfo.green.offset) | \
	(b << varinfo.blue.offset)

int main()
{
	struct fb_fix_screeninfo fixinfo;
	struct fb_var_screeninfo varinfo;
	long screensize;
	uint8_t *fbp;
	int fb_fd, ret;
	int x,y;

	fb_fd = open("/dev/fb0", O_RDWR);
	if (fb_fd < 0) {
		printf("Failed to open fbdev 0\n");
		return -1;
	}

	/* Get variable screen info */
	ret = ioctl(fb_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		printf("Failed to get fbdev varinfo\n");
		close(fb_fd);
		return -2;
	}

	varinfo.grayscale=0;
	varinfo.bits_per_pixel=32;

	/* Set variable screen info for new BPP */
	ret = ioctl(fb_fd, FBIOPUT_VSCREENINFO, &varinfo);
	if (ret < 0) {
		printf("Failed to set bpp\n");
		close(fb_fd);
		return -3;
	}

	/* Get new variable info */
	ret = ioctl(fb_fd, FBIOGET_VSCREENINFO, &varinfo);
	if (ret < 0) {
		printf("Failed to get new variable info\n");
		close(fb_fd);
		return -4;
	}

	ret = ioctl(fb_fd, FBIOGET_FSCREENINFO, &fixinfo);
	if (ret < 0) {
		printf("Failed to get fb fix info\n");
		close(fb_fd);
		return -5;
	}

	screensize = varinfo.yres_virtual * fixinfo.line_length;
	if (!screensize) {
		printf("Zero screen size, pitch=%d y=%d\n", fixinfo.line_length, varinfo.yres_virtual);
		close(fb_fd);
		return -6;
	}

	/* Map framebuffer memory in this app's space */
	fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
	if (!fbp) {
		printf("mmap failed\n");
		close(fb_fd);
		return -7;
	}

	/* Draw the tricolor on the screen */
	for (x = 0; x < varinfo.xres; x++) {

		/* Red */
		for (y = 0; y < varinfo.yres/3; y++) {
			long location = (x + varinfo.xoffset) * (varinfo.bits_per_pixel / 8) +
					(y+varinfo.yoffset) * fixinfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel(0xFF, 0x00, 0x00, varinfo);
		}

		/* Green */
		for (y = varinfo.yres/3; y < (2 * varinfo.yres)/3; y++) {
			long location = (x+varinfo.xoffset) * (varinfo.bits_per_pixel/8) +
					(y+varinfo.yoffset) * fixinfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel(0x00, 0xFF, 0x00, varinfo);
		}

		/* Blue */
		for (y = (2 * varinfo.yres)/3; y < varinfo.yres; y++) {
			long location = (x+varinfo.xoffset) * (varinfo.bits_per_pixel/8) +
					(y+varinfo.yoffset) * fixinfo.line_length;
			*((uint32_t*)(fbp + location)) = pixel(0x00, 0x00, 0xFF, varinfo);
		}

	}

	munmap(fbp, screensize);
	close(fb_fd);
	printf("Fbdev draw done\n");
	return 0;
}
