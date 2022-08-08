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
/* open*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/* close */
#include <unistd.h>

/* Map */
#include <sys/mman.h>

/* DRM */
#include <xf86drmMode.h>
#include <drm/drm_fourcc.h>

#define CARD_0 "/dev/dri/card0"

struct drm_format_name {
        char a;
        char b;
        char c;
        char d;
};

static void drm_print_format_name(uint32_t code)
{
        struct drm_format_name name;

	if (!code)
		return;

    name.a = code & 0xff;
    name.b = ((code & (0xff << 8)) >> 8);
    name.c = ((code & (0xff << 16)) >> 16);
    name.d = ((code & (0xff << 24)) >> 24);

	if (name.a == 'X' && name.b == 'R') {
		printf("Format name XBGR%c%c\n", name.c, name.d);
		return;
	}

    if (name.a == 'R' && name.b == 'X') {
		printf("Format name RGBX%c%c\n", name.c, name.d);
		return;
	}

    if (name.a == 'A' && name.b == 'R') {
		printf("Format name ABGR%c%c\n", name.c, name.d);
		return;
	}

    if (name.a == 'R' && name.b == 'A') {
		printf("Format name RGBA%c%c\n", name.c, name.d);
		return;
	}

    printf("Format name %c%c%c%c\n", name.a, name.b, name.c, name.d);
}

int main()
{
    int fd;
    int count;
    drmModeResPtr res;

    fd = open(CARD_0, O_RDWR);
    if (fd < 0) {
        printf("Error open\n");
        return -1;
    }

    res = drmModeGetResources(fd);
    if (!res) {
        printf("Error get res\n");
        close (fd);
        return -1;
    }

    printf("Get Res: CRTCs: %d Connectors: %d Enc: %d FBs: %d\n",
            res->count_crtcs, res->count_connectors,
            res->count_encoders, res->count_fbs);

    printf("\n============== CRTCs =================\n");
    for (count = 0; count < res->count_crtcs; count++) {
        drmModeCrtcPtr crtc;
        
        crtc = drmModeGetCrtc(fd, res->crtcs[count]);
        if (crtc) {
            printf("CRTC: id:0x%x, w:%d h:%d x:%d y:%d\n",
            crtc->crtc_id, crtc->width, crtc->height,
            crtc->x, crtc->y);
        }
        drmModeFreeCrtc(crtc);
    }
    printf("==========================================\n");

    printf("\n============== Connectors =================\n");
    for (count = 0; count < res->count_connectors; count++) {
        drmModeConnectorPtr conn;
        
        conn = drmModeGetConnector(fd, res->connectors[count]);
        if (conn) {
            printf("Conn: id:0x%x, wxh(mm):%dx%d, status:%d props: %d modes:%d\n",
                    conn->connector_id, conn->mmWidth, conn->mmHeight,
                    conn->connection, conn->count_props,
                    conn->count_modes);  
        }

        if (!count) {
            int i;
            drmModePropertyPtr prop;

            printf("\n\t============== Connector props =================\n");
            for (i= 0; i < conn->count_props; i++) {
                prop = drmModeGetProperty(fd, conn->props[i]);
                if (prop) {
                    printf("\tConn Prop: id: 0x%x, name: %s\n", 
                    prop->prop_id, prop->name);
                }
                drmModeFreeProperty(prop);
            }
            printf("\t==========================================\n");
        }

        drmModeFreeConnector(conn);
    }
    printf("==========================================\n");

    printf("\n============== Encs =================\n");
    for (count = 0; count < res->count_encoders; count++) {
        drmModeEncoderPtr enc;
        
        enc = drmModeGetEncoder(fd, res->encoders[count]);
        if (enc) {
            printf("ENC: id:0x%x, type: %d\n", enc->encoder_id, enc->encoder_type);
        }
        drmModeFreeEncoder(enc);
    }
    printf("==========================================\n");

    printf("\n============== Planes =================\n");
    drmModePlaneResPtr pres = drmModeGetPlaneResources(fd);
    for (count = 0; count < pres->count_planes; count++) {
        drmModePlanePtr p;

        p = drmModeGetPlane(fd, pres->planes[count]);
        if (p) {
            int fmt;

            printf("\nPlane: CRTC id:0x%x, plane id: 0x%x num_formats: %d\n",
                    p->crtc_id, p->plane_id, p->count_formats);
            printf("Supported Formats:\n");
            for (fmt = 0; fmt < p->count_formats; fmt++) {
                printf("0x%x ", p->formats[fmt]);
		drm_print_format_name(p->formats[fmt]);
                if (fmt && fmt%10 == 0)
                    printf("\n");
            }
            printf("\n");
        }
        drmModeFreePlane(p);
    }
    printf("\n==========================================\n");

    drmModeFreePlaneResources(pres);
    drmModeFreeResources(res);
    close(fd);
    return 0;
}
