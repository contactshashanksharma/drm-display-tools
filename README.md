# drm-display-tools

drm-draw-pixels:
A very basic and tiny libdrm based tools which:
 - creates a framebuffer
 - fills a pattern on this buffer
 - and shows this buffer on the display
 
 Using the libDRM and KMS APIs.
 
 drm-display-info:
 Another basic tool to dump available resources of a DRM card
 - prints available CRTCs
 - prints available Connectors (and properties of first connector)
 - prints available Encoders
 - prints available planes
 
 # Dependencies:
 
 
 It just needs libDRM on a Linux based system
 
 $ sudo apt install libdrm-dev
 
 # Building:
 
 
 $ make clean
 
 $ make
 
 # Running:
 
 # run drm_display_info, From any terminal:
 
 $ ./drm_display_info
 
 
 # To run drm_draw_pixels, go to a non-gui console and run
 
 $ sudo chvt 4 (or maybe ctrl + alt + F4)
 
 $ ./drm_draw_pixels
