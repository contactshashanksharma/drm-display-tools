all:
	gcc -o drm_draw_pixels drm_draw_pixels.c -g -ldrm -I/usr/include/drm
	gcc -o drm_display_info drm_display_info.c -g -ldrm -I/usr/include/drm

clean:
	rm drm_draw_pixels

install:
	sudo cp drm_draw_pixels /usr/bin/
