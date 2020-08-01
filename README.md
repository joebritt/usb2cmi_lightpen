# usb2cmi_lightpen
Use a USB mouse instead of (or in conjunction with) the lightpen on a Fairlight CMI.

Some work will be needed to get the code to build with the latest Microchip MPLAB X tools,
so I recommend installing the v8.63 release of MPLAB (pre-MPLAB X). 

Microchip is good about making legacy tools available, you can find version 8.63 here:

https://www.microchip.com/development-tools/pic-and-dspic-downloads-archive

# Basic Idea

When in use, the interface generates lightpen emulation signals that make the CMI video hardware
"think" a lightpen is there. The signals are generated to create a small (smaller than is normal
with the real lightpen) cursor on the screen.

Moving the mouse, you will see the on-screen cursor track the mouse movement.

Clicking the left button (or button, if it's a 1-button mouse) is like touching the tip of the
real lightpen.

If the mouse has a right button, clicking it will toggle the cursor off and on.

If a real lightpen is fitted to the VDU, it will continue to work in conjunction with the mouse.
Normally, the mouse cursor will be shown on the screen. If you bring the lightpen to the screen,
the lightpen cursor will appear and the mouse cursor will disappear. You can then interact using
the lightpen as normal. When you move the lightpen away from the screen, the mouse cursor will
reappear.

# Version 1

V1 is the original external box style interface. It is powered by a 5V AC adapter.
The 5-pin XLR cable from the CMI to the monitor ("VDU" in Fairlight parlance) connects
to the box. This cable carries a monochrome composite video signal with PAL timing from the CMI
and 2 TTL level signals from the lightpen to the CMI. 

These 3 signals are carried over 3x 75 ohm coax cables bundled together and sheathed similar to
an analog RGB video cable. The pinout for this cable is not straight-through, please see the
details in the monitor_cable folder.

Using two of these 5-pin XLR cables, the box is connected between the CMI and VDU.
Of course, the box can be used without the VDU, in which case only 1 5-pin XLR cable is needed.

V1 has 2 LEDs. These are used to show the status of the video signal (flashes if no video
detected) and presence of a mouse (flashes if no mouse detected).

V1 has an RCA jack which provides unbuffered access to the CMI video signal. Since it is
unbuffered, the legacy VDU cannot (normally) be used at the same time as the RCA jack.
(V2 buffers the video signal to allow this use case.)

# Version 2

V2 is an updated version that fits completely inside the CMI. It is the same core hardware design.

Differences:
  - No AC power supply needed, it is powered from 5V that is tapped from the CMI front panel board
  - RCA jack provides a buffered CMI video signal, so it and the VDU may be used simultaneously
  - Video RCA jack and USB connector are routed to a panel that mounts to the CMI bottom pan
  - laser-cut "ears" attach to the interface board and let it occupy an unused CMI slot


