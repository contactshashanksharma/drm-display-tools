# drm-display-tools

drm-draw-pixels:
A very basic and tiny libdrm based tools which:
 - creates a framebuffer
 - fills multiple patterns on this buffer
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

 # Build and Install the helper paint library first

 $ make paint

 $ sudo make paint-install

 
 # Build the tools now

 $ make
 
 # Install the tools
 $ sudo make install
 
 # Using these tools:
 
 To run drm_display_info, from any terminal, simply:
 
 $ ./drm_display_info
 
 
 To run drm_draw_pixels, go to a non-gui console and run
 
 
 $ sudo chvt 4 (or maybe ctrl + alt + F4)
 
 $ sudo ./drm_draw_pixels


# fbdev_tools: Framebuffer ecosystem based graphics tools

fbdev_draw: A very basic fbdev based tool, which:
- opens the framebuffer device (fb0)
- maps the buffer memory
- draws a tricolor pattern on screen

# Building:

 $ make fbdev_clean

 $ make fbdev

 # Running:

# To run fbdev_draw, go to a non-gui console and run

 $ sudo chvt 4 (or maybe ctrl + alt + F4)

 $ sudo ./fbdev_draw
