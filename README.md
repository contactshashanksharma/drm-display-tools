# drm-display-tool


A very basic and tiny tool which:
 - creates a framebuffer
 - fills a pattern on this buffer
 - and shows this buffer on the display
 
 Using the libDRM and KMS APIs.
 
 # Dependencies:
 
 
 It just needs libDRM on a Linux based system
 $ sudo apt install libdrm-dev
 
 # Building:
 
 
 $ make clean
 $ make
 
 # Running:
 
 
 # Go to a non-gui console
 
 $ sudo chvt 4 (or maybe ctrl + alt + F4)
 
 $ ./drm_draw_pixels
