Building USB Mouse -> CMI Lightpen adapter code
Joe Britt, August 2010

BUILDING FOR/WITH BOOTLOADER
----------------------------

Build the bootloader project (Release build) and flash into PIC32440MX128H
using ICD3 or equivalent.

Build the mouse project (Release build).  Do not flash.  Switch to command
(DOS) window and go to the mouse\Output directory.

type:

pic32-bin2hex -v cmi_mouse.elf

then type:

rename cmi_mouse.hex cmimouse.hex

then copy cmimouse.hex to your USB thumbdrive.

Plug USB thumbdrive into usb2cmi mouse adapter USB port, and power cycle the
usb2cmi board.  You will see the yellow light flicker as the mouse application
is programmed into the application partition.  When it finishes, it will
start the mouse application.


BUILDING STANDALONG (NO BOOTLOADER)
-----------------------------------

Open the mouse project and remove the 2 Linker Script files (in the IDE).
Build and load onto the usb2cmi board with an ICD3 or equivalent.

Debug/run.

Note that this will overwrite the bootloader, so you will need to build
and re-flash it if you want/need to use it again.


