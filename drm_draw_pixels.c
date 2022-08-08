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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include "paint.h"

/* Defaults to init framebuffer */
#define XRES 1920
#define YRES 1200
#define DEPTH_BYTES_PER_PIXEL 4

/* Graphic card nodes */
#define CARD_0 "/dev/dri/card0"
#define CARD_1 "/dev/dri/card1"

/* Verbose */
uint8_t be_loud;

struct fb {
	int x;
	int y;
	int d;
	int handle;
	int fb_fd;
	uint32_t stride;
	uint32_t size;
	char *mapped_fb;
};

struct drm_display {
	uint32_t crtc_id;
	uint32_t conn_id;
	uint32_t enc_id;
	drmModeModeInfo mode;
};

static void dump_videomodes(drmModeConnector *conn)
{
	int i;
	drmModeModeInfo *mode;

	printf("\n\t================================\n");
	for (i = 0; i < conn->count_modes; i ++) {
		mode = &conn->modes[i];
		printf("\tMode:%s %dx%d clock %d\n", mode->name, mode->hdisplay, mode->vdisplay, mode->clock);
	}
	printf("\t================================\n");
}

static void dump_props(int fd, uint32_t *props, int prop_count)
{
	int i;
	drmModePropertyPtr prop;

	if (!prop_count || !props)
	return;

	printf("\n\t================================\n");
	for (i = 0; i < prop_count; i++) {
		prop = drmModeGetProperty(fd, props[i]);
		if (prop) {
			printf("\t %s:id %d\n", prop->name, prop->prop_id);
			drmModeFreeProperty(prop);
		}
	}
	printf("\t================================\n");
}

static void paint_white(struct fb *fb)
{
	paint_a_buffer_white(fb->mapped_fb, fb->x, fb->y, fb->d);
}

static void blank_subbuffer(struct fb *fb, int xres, int yres, int x_off, int y_off, int x, int y)
{
	blank_a_buffer_region(fb->mapped_fb, xres, yres, x_off, y_off, x, y, 4);
}

static void paint_subbuffer(struct fb *fb, int xres, int yres, int x_off, int y_off, int x, int y)
{
	paint_a_buffer_region_tricolor(fb->mapped_fb, xres, yres, x_off, y_off, x, y, 4);
}

static void paint_tricolor(struct fb *fb)
{
	paint_buffer_tricolor(fb->mapped_fb, fb->x, fb->y, 4);
}

static int display_drm_buffer(int drm_fd, struct fb *fb, struct drm_display *display)
{
	int ret; 

	if (!fb || !display || drm_fd < 0) {
		printf("Can't display, invalid inputs\n");
		return -1;
	}

	/* Set the mode and fb on CRTC */
	ret = drmModeSetCrtc(drm_fd, display->crtc_id,
			fb->fb_fd, 0, 0,
			&display->conn_id, 1,
			&display->mode);
	if (ret < 0) {
		printf("Set CRTC fail, ret =%d\n", ret);
		return -1;
	}

	/* Keep the buffer on screen for a few seconds */
	sleep(3);
	return 0;
}

static int create_drm_buffer(int drm_fd, struct fb *fb)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;
	struct drm_mode_destroy_dumb dreq;
	char *mapped_buffer;
	int ret;

	/* create dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = fb->x;
	creq.height = fb->y;
	creq.bpp = 32;
	ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		printf("cannot create dumb buffer (%d): %m\n", errno);
		return -errno;
	}

	fb->stride = creq.pitch;
	fb->size = creq.size;
	fb->handle = creq.handle;

	/* create framebuffer object for the dumb-buffer */
	ret = drmModeAddFB(drm_fd, fb->x, fb->y, 24, 32, fb->stride, fb->handle, &fb->fb_fd);
	if (ret) {
		printf("cannot create framebuffer (%d): %m\n", errno);
		ret = -errno;
		goto err_destroy;
	}

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = fb->handle;
	ret = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		printf("cannot map dumb buffer (%d): %m\n", errno);
		ret = -errno;
		goto err_fb;
	}

	/* perform actual memory mapping */
	fb->mapped_fb = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, mreq.offset);
	if (fb->mapped_fb == MAP_FAILED) {
		printf("cannot mmap dumb buffer (%d): %m\n", errno);
		ret = -errno;
		goto err_fb;
	}

	/* clear the framebuffer to 0 */
	memset(fb->mapped_fb, 0, fb->size);
	return 0;

err_fb:
	drmModeRmFB(drm_fd, fb->fb_fd);

err_destroy:
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = fb->handle;
	drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	return ret;
}

void release_drm_buffer(int drm_fd, struct fb *fb)
{
	struct drm_mode_destroy_dumb dreq;

	munmap(fb->mapped_fb, fb->size);
	drmModeRmFB(drm_fd, fb->fb_fd);

	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = fb->handle;

	drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
}

static int get_drm_display(int drm_fd, struct drm_display *display)
{
	int i, ret = 0;
	uint32_t active_crtc;
	drmModeRes *res;
	drmModeCrtc *crtc;
	drmModeEncoder *enc;
	drmModeModeInfo *mode;
	drmModeConnector *conn;

	res = drmModeGetResources(drm_fd);
	if (!res) {
		printf("Failed to get resources\n");
		ret = -1;
		goto fail_res;
	}

	if (be_loud ) {
		printf("Resources of card: CRTCs:%d Connectors:%d Encoders:%d FBs: %d\n",
			res->count_crtcs, res->count_connectors, res->count_encoders,
			res->count_fbs);
	}

	/* Get the first connected connector */
	for (i = 0; i < res->count_connectors; i++) {
		conn = drmModeGetConnector(drm_fd, res->connectors[i]);

		if (be_loud) {
			printf("Connector %d: properties: %d\n", conn->connector_id, conn->count_props);
			dump_props(drm_fd, conn->props, conn->count_props);
		}

		if (conn->connection == DRM_MODE_CONNECTED)
			break;

		drmModeFreeConnector(conn);
		conn = NULL;
	}

	if (!conn) {
		printf("No connected connector found\n");
		ret = -1;
		goto fail_conn;
	}

	printf("Picking Connector: id:%d \n", conn->connector_id);

	if (be_loud && conn->count_modes) {
		printf("Supported Videomodes on connector:%d\n", conn->count_modes);
		dump_videomodes(conn);
	}

	/* Get the preferred resolution */
	for (i = 0; i < conn->count_modes; i++) {
		mode = &conn->modes[i];

		if (mode->type & DRM_MODE_TYPE_PREFERRED)
			break;
		mode = NULL;
	}

	if (!mode) {
		printf("No preferred mode found\n");
		ret = -1;
		goto fail_conn;
	}

	printf("Picking Mode: %dx%d clk %d\n", mode->hdisplay, mode->vdisplay, mode->clock);

	/* Get the enc coupled with connector */
	enc = drmModeGetEncoder(drm_fd, conn->encoder_id);
	if (!enc) {
		printf("No encoder found\n");
		ret = -1;
		goto fail_enc;
	}

	printf("Picking encoder:%d\n", enc->encoder_id);

	/* Get the CRTC on which encoder is active */
	active_crtc = enc->crtc_id;
	crtc = drmModeGetCrtc(drm_fd, active_crtc);
	if (!crtc) {
		printf("No CRTC found\n");
		ret = -1;
		goto fail_crtc;
	}

	printf("Found CRTC: %d\n", crtc->crtc_id);

	/* Steal required info */
	display->crtc_id = crtc->crtc_id;
	display->conn_id = conn->connector_id;
	display->enc_id = enc->encoder_id;
	memcpy(&display->mode, mode, sizeof(*mode));
	ret = 0;

fail_crtc:
	drmModeFreeEncoder(enc);

fail_enc:
	drmModeFreeConnector(conn);

fail_conn:
	drmModeFreeResources(res);

fail_res:
	return ret;
}

int main(void)
{
	int drm_fd;
	int ret = 0;
	struct fb fb = {0,};
	struct drm_display display = {0, };
	char *sub;
	int sub_pitch;
	int sub_h = 600;
	int sub_v = 200;

	drm_fd = open(CARD_0, O_RDWR);
	if (drm_fd < 0) {
		printf("Failed to open graphic card\n");
		return -1;
	}

	ret = get_drm_display(drm_fd, &display);
	if (ret) {
		printf("Failed to get display\n");
		ret = -1;
		goto close;
	}

	/* Set the fb size as per the mode, and create a framebuffer */
	fb.x = display.mode.hdisplay;
	fb.y = display.mode.vdisplay;
	fb.d = DEPTH_BYTES_PER_PIXEL;
	ret = create_drm_buffer(drm_fd, &fb);
	if (ret) {
		printf("Failed to create a drm buffer\n");
		ret = -1;
		goto close;
	}

	/* Draw tricolor lines on buffer */
	paint_tricolor(&fb);

	ret = display_drm_buffer(drm_fd, &fb, &display);
	if (ret) {
		printf("Failed to display buffer of %dx%d\n", fb.x, fb.y);
		ret = -1;
		goto release_buffer;
	}

	/* paint something else */
	paint_subbuffer(&fb, fb.x, fb.y, 200, 200, 1280, 720);
	ret = display_drm_buffer(drm_fd, &fb, &display);
	if (ret) {
		printf("Failed to display buffer 1920x1080\n");
		ret = -1;
	}

	/* blank some pixels */
	blank_subbuffer(&fb, fb.x, fb.y, 400, 400, sub_h, sub_v);
	ret = display_drm_buffer(drm_fd, &fb, &display);
	if (ret) {
		printf("Failed to display buffer 1920x1080\n");
		ret = -1;
	}

	sub = get_a_subbuffer_copy(fb.mapped_fb, fb.x, fb.y, 400, 400, sub_h, sub_v, 4);
	if (!sub) {
		printf("Failed to get the subbuffer\n");
		ret = -1;
	}
	sub_pitch = sub_h * 4;

	/* White paint the buffer first */
	paint_white(&fb);
	ret = display_drm_buffer(drm_fd, &fb, &display);
	if (ret) {
		printf("Failed to display white-buffer\n");
		ret = -1;
	}

	/* Display the subbuffer now at 0,0 but maintain the pitch of small buffer */
	for (ret = 0; ret < sub_v; ret++)
		memcpy(fb.mapped_fb + ret * fb.stride, sub, sub_pitch);

	ret = display_drm_buffer(drm_fd, &fb, &display);
	if (ret) {
		printf("Failed to display sub-buffer\n");
		ret = -1;
	}

release_buffer:
	release_drm_buffer(drm_fd, &fb);

close:
	close(drm_fd);
	return ret;
}
