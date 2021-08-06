#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

/* Defaults to init framebuffer */
#define XRES 1920
#define YRES 1200
#define DEPTH_BYTES_PER_PIXEL 4

/* Graphic card nodes */
#define CARD_0 "/dev/dri/card0"
#define CARD_1 "/dev/dri/card1"

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

static void paint(struct fb *fb)
{
	int y, x;
	int pitch = fb->stride;
	char *buf = fb->mapped_fb;

	/* Paint it red solid */
	for (y = 0; y < fb->y/3; y++) {
		uint32_t *pixel = (uint32_t *)&(fb->mapped_fb[y * pitch]);

		for (x = 0; x < fb->x; x++)
		pixel[x] = (0xFF << 16);
	}

	/* Paint it green solid */
	for (y = fb->y/3; y < (2 * fb->y)/3; y++) {
		uint32_t *pixel = (uint32_t *)&(fb->mapped_fb[y * pitch]);

		for (x = 0; x < fb->x; x++)
		pixel[x] = (0xFF << 8);
	}

	/* Paint it blue solid */
	for (y = (2 * fb->y)/3; y < fb->y; y++) {
		uint32_t *pixel = (uint32_t *)&(fb->mapped_fb[y * pitch]);

		for (x = 0; x < fb->x; x++)
		pixel[x] = 0xFF;
	}
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

	/* Get the first connected connector */
	for (i = 0; i < res->count_connectors; i++) {
		conn = drmModeGetConnector(drm_fd, res->connectors[i]);
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

	/* Init buffer size */
	fb.x = XRES;
	fb.y = YRES;
	fb.d = DEPTH_BYTES_PER_PIXEL;

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
	ret = create_drm_buffer(drm_fd, &fb);
	if (ret) {
		printf("Failed to create a drm buffer\n");
		ret = -1;
		goto close;
	}

	/* Draw something on buffer */
	paint(&fb);

	/* Set the mode and fb on CRTC */
	ret = drmModeSetCrtc(drm_fd, display.crtc_id,
			fb.fb_fd, 0, 0,
			&display.conn_id, 1,
			&display.mode);
	if (ret < 0) {
		printf("Set CRTC fail, ret =%d\n", ret);
		ret = -1;
		goto release_buffer;
	}

	/* Keep the buffer on screen for a few seconds */
	sleep(3);

release_buffer:
	release_drm_buffer(drm_fd, &fb);

close:
	close(drm_fd);
	return 0;
}
