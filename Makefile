all:
	gcc -o drm_draw_pixels drm_draw_pixels.c -g -ldrm -I/usr/include/drm
	gcc -o drm_display_info drm_display_info.c -g -ldrm -I/usr/include/drm

clean:
	rm drm_draw_pixels
	rm drm_display_info

install:
	sudo cp drm_draw_pixels /usr/bin/
	sudo cp drm_display_info /usr/bin/

fbdev:
	gcc -o fbdev_draw fbdev_draw.c -g

fbdev_clean:
	rm fbdev_draw

fbdev_install:
	sudo cp fbdev_draw /usr/bin/
