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