all:
	gcc -o drm_draw_pixels drm_draw_pixels.c -g -ldrm -lpaint -I/usr/include/drm
	gcc -o drm_display_info drm_display_info.c -g -ldrm -I/usr/include/drm

clean:
	rm libpaint.so
	rm drm_draw_pixels
	rm drm_display_info

install:
	sudo cp drm_draw_pixels /usr/bin/
	sudo cp drm_display_info /usr/bin/

paint:
	gcc -c -fpic -g paint.c
	gcc -shared -o libpaint.so paint.o

paint-install:
	sudo cp libpaint.so /usr/lib/



fbdev:
	gcc -o fbdev_draw fbdev_draw.c -g

fbdev_clean:
	rm fbdev_draw

fbdev_install:
	sudo cp fbdev_draw /usr/bin/
