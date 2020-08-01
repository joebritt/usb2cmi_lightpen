/******************************************************************************
	Framework derived from Microchip sample code Mouse_demo.c.

	Lightpen emulation code Copyright 2010 Joe Britt (britt@shapetable.com).
	All rights reserved.  Released under the 2-clause BSD license:

	Redistribution and use in source and binary forms, with or without modification, are
	permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS OR IMPLIED
	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
	ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
	NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	The views and conclusions contained in the software and documentation are those of the
	authors and should not be interpreted as representing official policies, either expressed
	or implied, of Joe Britt.

	----------------------------------------------------------------------------
	Original Microchip header below:

    Any Low speed/Full Speed USB Mouse can be connected. 
	This file schedules the HID ransfers, and interprets the report received from the mouse.
	X & Y axis coordinates, Left & Right Click received from the mouse are available.

	* File Name:       Mouse_demo.c
	* Company:         Microchip Technology, Inc.

	Software License Agreement

	The software supplied herewith by Microchip Technology Incorporated
	(the “Company”) for its PICmicro® Microcontroller is intended and
	supplied to you, the Company’s customer, for use solely and
	exclusively on Microchip PICmicro Microcontroller products. The
	software is owned by the Company and/or its supplier, and is
	protected under applicable copyright laws. All rights are reserved.
	Any use in violation of the foregoing restrictions may subject the
	user to criminal sanctions under applicable laws, as well as to
	civil liability for the breach of the terms and conditions of this
	license.

	THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
	WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
	TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
	IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
	CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

	Version          Date    Comments
	--------------------------------------------------------------------------------
	?            14-Apr-2008 First release

*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "GenericTypeDefs.h"
#include "HardwareProfile.h"

#include "usb_config.h"

#include "usb/usb.h"
#include "usb/usb_host_hid_parser.h"
#include "usb/usb_host_hid.h"

#include <plib.h>

#include "pos_II.h"
#include "pos_IIx.h"

/*
	This is used to tweak the offsets when tuning the pixel offset tables
	to minimize cursor jitter.

	Note that for machines which use the Q045 video board (Series I/II), the analog
	adjustments for active screen area position & width make automatic de-jittering
	difficult.

	You can use the RV1 and RV2 pots on that board to adjust the video to minimize
	jitter.
*/
//#define	TUNE_TIMING		1

// *****************************************************************************
// Configuration Bits
// *****************************************************************************

#if defined( __PIC32MX__ )
    #pragma config UPLLEN   = ON            // USB PLL Enabled
    #pragma config FPLLMUL  = MUL_20        // PLL Multiplier
    #pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
    #pragma config FPLLIDIV = DIV_2         // PLL Input Divider
    #pragma config FPLLODIV = DIV_1         // PLL Output Divider
    #pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
    #pragma config FWDTEN   = OFF           // Watchdog Timer
    #pragma config WDTPS    = PS1           // Watchdog Timer Postscale
    #pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
    #pragma config OSCIOFNC = OFF           // CLKO Enable
    #pragma config POSCMOD  = EC            // Primary Oscillator, External Clock
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect
    #pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
    #pragma config DEBUG    = ON			// Debugger Disabled for Starter Kit
#else
    #error Cannot define configuration bits.
#endif

// *****************************************************************************
// USB Data Structures
// *****************************************************************************

typedef enum _APP_STATE
{
    DEVICE_NOT_CONNECTED,
    DEVICE_CONNECTED,		/* Device Enumerated  - Report Descriptor Parsed */
    READY_TO_TX_RX_REPORT,
    GET_INPUT_REPORT,		/* perform operation on received report */
    INPUT_REPORT_PENDING,
    ERROR_REPORTED 
} APP_STATE;

typedef struct _HID_REPORT_BUFFER
{
    WORD  Report_ID;
    WORD  ReportSize;
    BYTE  ReportData[4];	// BYTE* ReportData;
    WORD  ReportPollRate;
}   HID_REPORT_BUFFER;

// *****************************************************************************
// USB Internal Function Prototypes
// *****************************************************************************

void AppInitialize(void);
BOOL AppGetParsedReportDetails(void);
void App_Detect_Device(void);
void App_ProcessInputReport(void);
BOOL USB_HID_DataCollectionHandler(void);

// *****************************************************************************
// USB Constants
// *****************************************************************************
#define MAX_ALLOWED_CURRENT             (500)         // Maximum power we can supply in mA
//#define MINIMUM_POLL_INTERVAL           (0x0A)        // Minimum Polling rate for HID reports is 10ms

#define USAGE_PAGE_BUTTONS              (0x09)

#define USAGE_PAGE_GEN_DESKTOP          (0x01)

#define MAX_ERROR_COUNTER               (10)

// *****************************************************************************
// General USB global Variables
// *****************************************************************************

APP_STATE App_State_Mouse = DEVICE_NOT_CONNECTED;

HID_DATA_DETAILS Appl_Mouse_Buttons_Details;
HID_DATA_DETAILS Appl_XY_Axis_Details;

HID_REPORT_BUFFER  Appl_raw_report_buffer;

// need safety margin?  these were 3, and other memory was getting stomped
HID_USER_DATA_SIZE Appl_Button_report_buffer[16];
HID_USER_DATA_SIZE Appl_XY_report_buffer[16];

BYTE ErrorDriver;
BYTE ErrorCounter;
BYTE NumOfBytesRcvd;

BOOL ReportBufferUpdated;

// *****************************************************************************
// Lightpen emulation global Variables
// *****************************************************************************

SHORT			xpos;					//	lightpen cursor position
SHORT			ypos;

unsigned int	y_bias;					// scanlines to active video (different for I/II vs. IIx)

SHORT			gpad_x;					// G-Pad (Series III) cursor position
SHORT			gpad_y;

unsigned int	lastVSYNC;				// count captured at last VSYNC
unsigned int	lastStat;

unsigned int	*pos_tbl;				// pixel -> clock offset table
unsigned int	pos;					// xpos converted to TMR23 count

unsigned int	show_cursor;
unsigned int	show_cursor_gpad;

unsigned int	lineCount;
unsigned int	cursorLine;
unsigned int	drawnLines;

unsigned int	mouse_left_button;		// non-zero when left button down

unsigned int	blinker;				// used to blink the green LED when no video present

unsigned int	real_hit_count_now;		// counts HIT pulses from real LP
unsigned int	real_hit_count_last;	// pulse count at last vsync

//******************************************************************************
// USB Support Functions
//******************************************************************************

BOOL USB_ApplicationEventHandler( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    switch( event )
    {
        case EVENT_VBUS_REQUEST_POWER:
            // The data pointer points to a byte that represents the amount of power
            // requested in mA, divided by two.  If the device wants too much power,
            // we reject it.
            if (((USB_VBUS_POWER_EVENT_DATA*)data)->current <= (MAX_ALLOWED_CURRENT / 2))
            {
                return TRUE;
            }
            else
            {
                //UART2PrintString( "\r\n***** USB Error - device requires too much current *****\r\n" );
            }
            break;

        case EVENT_VBUS_RELEASE_POWER:
            // Turn off Vbus power.
            return TRUE;
            break;

        case EVENT_HUB_ATTACH:
            //UART2PrintString( "\r\n***** USB Error - hubs are not supported *****\r\n" );
            return TRUE;
            break;

        case EVENT_UNSUPPORTED_DEVICE:
            //UART2PrintString( "\r\n***** USB Error - device is not supported *****\r\n" );
            return TRUE;
            break;

        case EVENT_CANNOT_ENUMERATE:
            //UART2PrintString( "\r\n***** USB Error - cannot enumerate device *****\r\n" );
            return TRUE;
            break;

        case EVENT_CLIENT_INIT_ERROR:
            //UART2PrintString( "\r\n***** USB Error - client driver initialization error *****\r\n" );
            return TRUE;
            break;

        case EVENT_OUT_OF_MEMORY:
            //UART2PrintString( "\r\n***** USB Error - out of heap memory *****\r\n" );
            return TRUE;
            break;

        case EVENT_UNSPECIFIED_ERROR:   // This should never be generated.
            //UART2PrintString( "\r\n***** USB Error - unspecified *****\r\n" );
            return TRUE;
            break;

		case EVENT_HID_RPT_DESC_PARSED:
			 #ifdef APPL_COLLECT_PARSED_DATA
			     return(APPL_COLLECT_PARSED_DATA());
		     #else
				 return TRUE;
			 #endif
			break;

        default:
            break;
    }
    return FALSE;
}


/*
LIGHT PEN EMULATION

Q219 Video Basics
-----------------
The active video is 512 pixels wide and 256 pixels tall.  From the service manual, 
the actual video signal is 84 bytes/line (672 dot clocks) and 304 lines, so a full 
frame is 672 dot clks/line * 304 lines = 204,288 dot clocks.

Each HSYNC starts at the 68th byte position on a line and lasts for 6 byte times 
(so 48 dot clocks).

Since there are 84 bytes/line, that means:
64 bytes of pixels + 6 bytes of HSYNC + 14 bytes of nothing.

Each VSYNC starts on line 272 and lasts for 4 lines, or 672 * 4 = 2688 dot clocks = 259 uS.

Since there are 304 lines, that means:
256 lines of pixels + 4 lines of VSYNC + 44 lines of nothing.

Lightpen Basics
---------------
There are 2 active-low TTL signals from the LP: HIT goes low when the photodetector 
sees the beam go past, and TOUCH goes low when the user touches the end of the pen 
with his/her finger.

Q219 deglitches the HIT signal by requiring that the 2 scanlines after an initial 
HIT also generate a HIT.  That generates LPSTB which latches the counters and 
generates an interrupt.  Interrupt handler reads counters to get LP position.

TOUCH is just like a mouse button.

Mouse -> Lightpen Emulation
---------------------------
There is a Virtual Light Pen (VLP) coordinate with X,Y (xpos, ypos variables) expressed
as screen pixels (X = 0->511, Y = 0->255).  These X & Y values are updated when the mouse
moves by the USB host code running on the PIC32.

Mouse movement generates relative movement values.  These are added to the VLP coordinate 
to update the VLP position.  So, the X and Y values always indicate where the cursor
should be "drawn."

An LM1881 Video Sync Separator is fed the video signal from the Q219, and extracts
VSYNC and Composite Sync (CSYNC).  CSYNC is both VSYNC and HSYNC.

We use the PIC32's 32 bit high-speed timer and two Input Capture peripherals to synchronize
with the Q219 video.  We then use one of the PIC32's Output Compare peripherals to generate
pulses on the lightpen HIT line with the right timing to make the Q219 think that the 
lightpen saw the beam fly by.

The PIC32's Timer 2 and 3 are bonded into a 32 bit counter, clocked by the internal 
80MHz clock.

The natural dotclk of the Q219 board is 10.38MHz.  There are then 7.7 80MHz timer ticks per
10.38MHz dot clock.  This is enough resolution to generate HIT pulses within a dotclk
(pixel) window to get a stable cursor on the screen.

When we get a VSYNC interrupt, we clear the line counter -- we're now at the top of the
frame.  We then wait for the VSYNC to end, an know that from this point on the CSYNC
interrupts we get are really HSYNC interrupts.

Each CSYNC/HSYNC interrupt increments our line counter and checks to see if the current
cursor (VLP) position has a scanline on this line.  If it does, the Output Compare is
set up to generate a HIT pulse at the appropriate pixel position and with the appropriate
length.

Note that the Q219 has a feature called a "hit deglitcher."  Because of this, the hotspot
of the cursor is really at the leftmost point of the *second* scanline of the cursor.
(That's why I gave the mouse cursor the shape it has.)

Status LEDs
-----------
There are 2 LEDs on the board:

The Green LED indicates power and video detected, and is normally on solid.  
If there is no video detected, it blinks.

The Yellow LED indicates successfully communication with the USB mouse.  Normally it is
on solid.  If the board can't communicate with the USB mouse, it will be OFF.

Operation in Conjunction with a Real Lightpen
---------------------------------------------
Using this interface does not preclude use of a real lightpen.  The mouse cursor can be
toggled off and on with the right mouse button.  If the mouse cursor is on and a real
lightpen is brought to the CRT, the interface will pass the real HIT pulses through to
the Q219.  The interface will also detect the presence of real HIT pulses and disable the
mouse cursor.  When the real lightpen HIT pulses cease, the mouse cursor will be
re-enabled.  So, you can freely switch back and forth between a real lightpen and the
mouse.

*/

void lpe_init(void)
{
	// Yellow (bottom) LED on RB1
	// Green (top) LED on RB0

	TRISBbits.TRISB1 = 0;	// RB1 is OUTPUT
	TRISBbits.TRISB0 = 0;	// RB0 is OUTPUT

	PORTBSET = 0x0002;		// Yellow LED off until we find the mouse

	PORTBCLR = 0x0001;		// Green LED on


	// TOUCH in from real LP is on RF1
	// !TOUCH out on RF0

	TRISFbits.TRISF1 = 1;	// RF1 is INPUT
	TRISFbits.TRISF0 = 0;	// RF0 is OUTPUT
	
	PORTFCLR = 0x0001;		// drive LOW (inactive, inverted)


	// HIT in from real LP is on RD0

	TRISDbits.TRISD0 = 1;	// RD0 is INPUT

	IEC0bits.INT0IE = 0;	// disable INT0
	INTCONbits.INT0EP = 0;	// RD0 is INT0, interrupt on falling edge
	IFS0bits.INT0IF = 0; 	// clear the interrupt flag


	// Output Compare 2

	TRISDbits.TRISD1 = 0;	// OC2 is on RD1
	PORTDCLR = 0x0002;		// ensure initially driven low (inactive, inverted)


	// Set up Timer 2/3

	T2CON = 0;				// timer STOP, reset everything
	T3CON = 0;				// be sure timer 3 is stopped, too

	T2CONbits.TCS = 0;		// clocked from PBCLK (80MHz)
	T2CONbits.T32 = 1;		// bond timer 2 & 3
	T2CONbits.TCKPS = 0;	// prescale 1:1, so 80MHz count rate

	TMR23 = 0;				// clear timer

	PR2 = 0xFFFFFFFF;

	
	INTConfigureSystem( INT_SYSTEM_CONFIG_MULT_VECTOR );

	// -----------------------
	// VSYNC capture

	// Set up Input Capture 3 (VSYNC) for Timer 2/3

	TRISDbits.TRISD10 = 1;	// IC3 is on RD10

	IC3CON = 0;				// disable everything
	IC3CON = 0;

	IC3CONbits.C32 = 1;		// 32 bit capture

	IC3CONbits.ICM = 2;		// capture every falling edge

	IC3CONbits.ON = 1;		// enable capture


	// -----------------------
	// HSYNC capture

	// Set up Input Capture 2 (HSYNC) for Timer 2/3

	TRISDbits.TRISD9 = 1;	// IC2 is on RD9

	IC2CON = 0;				// disable everything
	IC2CON = 0;

	IC2CONbits.C32 = 1;		// 32 bit capture

	IC2CONbits.ICM = 2;		// capture every falling edge

	IC2CONbits.ON = 1;		// enable capture

	// set up interrupt handlers -- IPL7, the highest.  Need to be higher than USB
	//  to make handling HSYNC the higher priority.  Without this, the cursor glitches.

    INTSetVectorPriority(		INT_INPUT_CAPTURE_3_VECTOR, INT_PRIORITY_LEVEL_6	);	// VSYNC
    INTSetVectorSubPriority(	INT_INPUT_CAPTURE_3_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTClearFlag(				INT_IC3												);

	// Enable VSYNC interrupts

    INTEnable(					INT_IC3, INT_ENABLED								);

    INTSetVectorPriority(		INT_INPUT_CAPTURE_2_VECTOR, INT_PRIORITY_LEVEL_7	);	// HSYNC
    INTSetVectorSubPriority(	INT_INPUT_CAPTURE_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTClearFlag(				INT_IC2												);
 
	// HSYNC interrupts will get enabled as soon as we have valid VSYNCs

	// set up handler for HIT from real lightpen (if fitted)

    INTSetVectorPriority(		INT_EXTERNAL_0_VECTOR, INT_PRIORITY_LEVEL_5	);
    INTSetVectorSubPriority(	INT_EXTERNAL_0_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTClearFlag(				INT_INT0											);

	// Enable INT0 (RD0, real LP HIT)

    INTEnable(					INT_INT0, INT_ENABLED								);

	// Go!

    INTEnableInterrupts();

	T2CONbits.ON = 1;			// timer RUN
}


int vsync_history_index = 0;
unsigned int vsync_history[16];				// last 16 TMR23 counts at VSYNC
											// used to detect Series I/II vs. IIx video timing

#define MAX_XPOS				511			// selected so you can just see cursor tip at right edge of screen
#define	MAX_YPOS				255

#define	Y_BIAS_IIX				28			// scanline offset
#define Y_BIAS_II				25

#define	CURSOR_HEIGHT			3			// in scanlines


// We get here in response to VSYNC falling edge

#pragma interrupt _InputCapture3Handler ipl6 vector 13
void _InputCapture3Handler(void)
{
	unsigned int deglitch;
	unsigned int video_type_detect;

	INTDisableInterrupts();

	/* ----- Did the real lightpen generate HIT pulses during the last frame? ----- */

	real_hit_count_last = real_hit_count_now;
	real_hit_count_now = 0;

	/* ----- We have VSYNC, keep the green LED on ----- */

	blinker = 0;
	PORTBCLR = 0x0001;

	/* ----- VSYNC fell, wait for it to go up, then clear dot counter ----- */

	INTClearFlag( INT_IC3 );

	IC3CONbits.ON = 0;

	lineCount = 0;							// new frame!

	video_type_detect = TMR23;
	
	if ( (video_type_detect & 0x00FF0000) == 0x00170000 ) {
		y_bias = Y_BIAS_IIX;
		pos_tbl = pixel_offsets_IIx;
	}
	else {
		y_bias = Y_BIAS_II;
		pos_tbl = pixel_offsets_II;
	}

	vsync_history[vsync_history_index&0xf] = video_type_detect;
	vsync_history_index++;

	cursorLine = ypos + y_bias;				// set up to draw the cursor

	deglitch = 0;
	while( !PORTDbits.RD10 ) {				// wait for VSYNC to go high
		if ( deglitch++ > 0x4000 )			// I've seen bogus VSYNC interrupts when there
			break;							//  is no video connected.
	}

	TMR23 = 0;								// clear dot counter

	IC3CONbits.ON = 1;						// capture back on for this new frame

	/* ----- toggle HSYNC capture on/off to clear any possible overflow error ----- */
	/*       if we don't do this, and it overflows, we stop getting interrupts      */

	IC2CONbits.ON = 0;						// capture 2 off, this will clear any errors
	IC2CONbits.ON = 1;						// capture 2 back on

	/* ----- re-enable interrupts ----- */

	INTEnable( INT_IC3, INT_ENABLED );		// VSYNC
	INTEnable( INT_IC2, INT_ENABLED );		// HSYNC

    INTEnableInterrupts();
}



// format is ending count, starting count for scanline pulse
unsigned int shape[CURSOR_HEIGHT][2] = { {72,24}, {64,0}, {72,24} };


// I've noticed that CSYNC from the LM1881 seems to flap around if there is
// no valid video and no video input cable connected.
//
// This can spuriously call the HSYNC input capture interrupt handler so
// much that the main loop bogs way down (blinking LED slows down or stops).
//
// Disable HSYNC interrupt handling if no VSYNC.  VSYNC handler will
// re-enable when (if) we start getting VSYNCs again.

int flap_count = 0;
int pos_history_index = 0;
unsigned int pos_history[16];		// last 16 IC2BUF counts
									// used to detect HSYNC flapping due to loss of video signal

#define FLAP_THRESHOLD	0x01000000	// determined empirically
#define	MAX_FLAP_COUNT	10			// assume this many in a row is a good indicator of flapping

#ifdef TUNE_TIMING
unsigned int adjust = 0;
#endif

// We get here in response to HSYNC falling edge

#pragma interrupt _InputCapture2Handler ipl7 vector 9
void _InputCapture2Handler(void)
{
	INT_EN_DIS	enable = INT_ENABLED;			// assume we will re-enable

	INTDisableInterrupts();

	INTClearFlag(INT_IC2);

	lineCount++;

	pos = IC2BUF;								// dotclk count at falling edge of HSYNC

	pos_history[pos_history_index&0xf] = pos;
	pos_history_index++;

	if ( !real_hit_count_last && show_cursor && (lineCount == cursorLine) && !IC2CONbits.ICOV ) {

		// set up the pulse that will be this scanline's slice of the cursor

		pos += ( pos_tbl[xpos] );				// offset into this scanline

#ifdef TUNE_TIMING
		pos += adjust;
#endif

		OpenOC2( OC_ON | OC_TIMER_MODE32 | OC_TIMER2_SRC | OC_SINGLE_PULSE, 
								pos+shape[drawnLines][0], pos+shape[drawnLines][1] );

		if ( ++drawnLines == CURSOR_HEIGHT ) {
			cursorLine = ypos;
			drawnLines = 0;
		} else
			cursorLine++;						// next scanline we want
	} 

	/* ----- toggle HSYNC capture on/off to clear any possible overflow error ----- */
	/*       if we don't do this, and it overflows, we stop getting interrupts      */

	IC2CONbits.ON = 0;							// capture 2 off, this will clear any errors
	IC2CONbits.ON = 1;							// capture 2 back on

	// If we see MAX_FLAP_COUNT hsync intervals greater than FLAP_THRESHOLD, assume we are
	//  flapping and return with hysnc interrupts disabled.
	//
	// Next vsync will re-enable hysnc interrupts.

	if ( pos > FLAP_THRESHOLD ) {
		flap_count++;
		if ( flap_count > MAX_FLAP_COUNT ) {
			flap_count = 0;
			enable = INT_DISABLED;				// return with hsync interrupts off								
		}
	} else
		flap_count = 0;							// look for MAX_FLAP_COUNT in a row

	/* ----- re-enable interrupts ----- */

	INTEnable( INT_IC2, enable );

    INTEnableInterrupts();
}


// We get here in response to real lightpen HIT falling edge

#pragma interrupt _Int0Handler ipl5 vector 3
void _Int0Handler(void)
{
	INTDisableInterrupts();

	INTClearFlag( INT_INT0 );

	real_hit_count_now++;

	/* ----- re-enable interrupts ----- */

	INTEnable( INT_INT0, INT_ENABLED );

    INTEnableInterrupts();
}

#ifdef TUNE_TIMING
void Init_UART1( unsigned int bps ) {

	UARTConfigure( UART1,		UART_ENABLE_PINS_TX_RX_ONLY );
	UARTSetDataRate( UART1,		GetPeripheralClock(), bps );
	UARTSetLineControl( UART1,	UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1 );

	UARTEnable( UART1,			UART_ENABLE_FLAGS( UART_PERIPHERAL | UART_RX | UART_TX ) );
}


void SendByte_UART1(unsigned char c) {
	int timeout = 100000;

	while ( timeout && !UARTTransmitterIsReady( UART1 ) ) {
		timeout--;		
	}

	if ( timeout > 0 )
		UARTSendDataByte( UART1, c );
}

void print_num(int num) {
	int x;

	x = num/1000;
	SendByte_UART1( x + 0x30 );
	num -= (x*1000);

	x = num/100;
	SendByte_UART1( x + 0x30 );
	num -= (x*100);

	x = num/10;
	SendByte_UART1( x + 0x30 );
	num -= (x*10);

	SendByte_UART1( num + 0x30 );
	
}

void dump_pos_timing() {
	print_num( xpos );
	SendByte_UART1(',');
	print_num( adjust + pos_tbl[xpos] );
	SendByte_UART1(0x0d);
	SendByte_UART1(0x0a);
}
#endif  // TUNE_TIMING


void Init_UART2( unsigned int bps ) {

	UARTConfigure( UART2,		UART_ENABLE_PINS_TX_RX_ONLY );
	UARTSetDataRate( UART2,		GetPeripheralClock(), bps );
	UARTSetLineControl( UART2,	UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1 );

	UARTEnable( UART2,			UART_ENABLE_FLAGS( UART_PERIPHERAL | UART_RX | UART_TX ) );
}


void SendByte_UART2(unsigned char c) {
	int timeout = 100000;

	while ( timeout && !UARTTransmitterIsReady( UART2 ) ) {
		timeout--;		
	}

	if ( timeout > 0 )
		UARTSendDataByte( UART2, c );
}


#define MAX_GPAD_X		0x3ff
#define	MAX_GPAD_Y		0x3ff

/*
	UART Tx FIFO is 4 bytes.
	Each mouse movement results in 6 bytes being sent.

	At 9,600 bps, that takes ~1ms/byte

	This is all done just from the main loop, interrupt code will continue
	 and can interrupt the spinwait used by UART code.
*/

void Send_GPad_Packet() {
	unsigned char b;

	SendByte_UART2( 0x80 );		// control byte

	b = 0xe0;					// 111p tsc0: p = pen on pad
								//            t = touch
								//            s = shift key down (keyboard i/f adds)
								//			  c = ctl key down (keyboard i/f adds)

	if ( mouse_left_button )
		b |= 0x08;				// touch

	if ( show_cursor_gpad )
		b |= 0x10;				// pen on pad

	SendByte_UART2( b );

	SendByte_UART2( 0xe0 | (gpad_x & 0x001f) );				// low 5 bits
	SendByte_UART2( 0xe0 | ((gpad_x >> 5) & 0x001f ));		// hi 5 bits

	SendByte_UART2( 0xe0 | (gpad_y & 0x001f) );				// low 5 bits
	SendByte_UART2( 0xe0 | ((gpad_y >> 5) & 0x001f ));		// hi 5 bits
}


//******************************************************************************
// Main
//******************************************************************************

#ifdef TUNE_TIMING
#define	INIT_XPOS		0
#define	INIT_YPOS		0x18
#else
#define	INIT_XPOS		255
#define	INIT_YPOS		127
#endif

int main (void)
{
	SHORT last_gpad_x;		// previous G-Pad (Series III) cursor position
	SHORT last_gpad_y;		//  (used to prevent re-sending same position over & over)
	SHORT last_show_cursor_gpad;
	SHORT last_mouse_left_button;

	int buzz;

	int	value;

	value = SYSTEMConfigWaitStatesAndPB( GetSystemClock() );
    
	// Enable the cache for the best performance
	CheKseg0CacheOn();
    
	INTEnableSystemMultiVectoredInt();
    
	value = OSCCON;
	while (!(value & 0x00000020))
	{
		value = OSCCON;		// Wait for PLL lock to stabilize
	}
        
	AD1PCFG = 0xFFFF;   	// Set analog pins to digital.
	TRISF   = 0x00;
	TRISD   = 0x00C0;

	// UART2 is used to communicate with the Keyboard interface (if installed)

	Init_UART2( 9600 );

	// Init the USB host stack

	USBInitialize( 0 );

	// Init our globals

	xpos = INIT_XPOS;			// initial cursor position
	ypos = INIT_YPOS;

	y_bias = Y_BIAS_IIX;		// assume we're a IIx, interrupt code will select the right
								//  setting based on video timing

#ifdef TUNE_TIMING
	adjust = 0;
	Init_UART1( 9600 );
#endif

	last_gpad_x = gpad_x = (MAX_GPAD_X / 2);
	last_gpad_y = gpad_y = (MAX_GPAD_Y / 2);

	show_cursor = 1;			// cursor on by default

	show_cursor_gpad = 1;		// separate one for G-Pad, since video detector automatically
								//  turns the other one back on
	mouse_left_button = 0;

	real_hit_count_now = real_hit_count_last = 0;

	blinker = 0;				// will blink the green LED if VSYNCs aren't coming in

	// Configure the hardware & set up interrupts

	lpe_init();

	Send_GPad_Packet();			// get cursor on-screen and centered
      
	// ==================================================================================
	// Main loop!
    
	while(1)
	{
		if (blinker++ > 0x8000) {
			blinker = 0;				// as long as we're getting VSYNCs, the VSYNC int
			PORTBINV = 0x0001;			//  handler will keep yanking blinker back to 0

			// If vsyncs (video) has been gone long enough to flip the LED, 
			// the user probably turned off the CMI or pulled the video cable.
			// Go back to the initial cursor state and position, so it won't be
			// potentially confusing when video returns.

			xpos = INIT_XPOS;			// initial cursor position
			ypos = INIT_YPOS;
			show_cursor = 1;			// cursor on by default
		}

		// see if we need to pass on the TOUCH signal from the real LP

		if( mouse_left_button || (PORTFbits.RF1 == 0) )
			PORTFSET = 0x0001;			// drive HIGH (active, inverted)
		else
			PORTFCLR = 0x0001;			// drive LOW (inactive, inverted)

		USBTasks();						// go poll the mouse
		App_Detect_Device();

		switch(App_State_Mouse)
		{
                case DEVICE_NOT_CONNECTED:
                          	USBTasks();

							PORTBSET = 0x0002;			// Yellow LED OFF

							if(USBHostHID_ApiDeviceDetect()) /* True if report descriptor is parsed with no error */
							{
								App_State_Mouse = DEVICE_CONNECTED;
							}
                    break;

                case DEVICE_CONNECTED:
                          	App_State_Mouse = READY_TO_TX_RX_REPORT;
							
							PORTBCLR = 0x0002;			// Yellow LED ON

							xpos = INIT_XPOS;			// mouse attached, center cursor and
							ypos = INIT_YPOS;
							show_cursor = 1;			// make sure the cursor is visible
                    break;

                case READY_TO_TX_RX_REPORT:
                             if(!USBHostHID_ApiDeviceDetect())
                             {
                                App_State_Mouse = DEVICE_NOT_CONNECTED;
                             }
                             else
                             {
                                App_State_Mouse = GET_INPUT_REPORT;
                             }

                    break;

                case GET_INPUT_REPORT:
                            if(USBHostHID_ApiGetReport(Appl_raw_report_buffer.Report_ID,0,
                                                        Appl_raw_report_buffer.ReportSize, Appl_raw_report_buffer.ReportData))
                            {
                               /* Host may be busy/error -- keep trying */
                            }
                            else
                            {
                                App_State_Mouse = INPUT_REPORT_PENDING;
                            }
                            USBTasks();
                    break;

                case INPUT_REPORT_PENDING:
							if(USBHostHID_ApiTransferIsComplete(&ErrorDriver,&NumOfBytesRcvd))
							{
 								if(ErrorDriver ||(NumOfBytesRcvd != Appl_raw_report_buffer.ReportSize ))
								{
                                  ErrorCounter++ ; 
                                  if(MAX_ERROR_COUNTER <= ErrorDriver)
                                     App_State_Mouse = ERROR_REPORTED;
                                  else
                                     App_State_Mouse = READY_TO_TX_RX_REPORT;
                                }
                                else
                                {
                                  ErrorCounter = 0; 
                                  ReportBufferUpdated = TRUE;
                                  App_State_Mouse = READY_TO_TX_RX_REPORT;

								  last_gpad_x = gpad_x;
								  last_gpad_y = gpad_y;

								  last_show_cursor_gpad = show_cursor_gpad;
								  last_mouse_left_button = mouse_left_button;

                                  App_ProcessInputReport();

								  // if the cursor is on, and something changed, send to CMI
								  // if the cursor just changed on/off state, send to CMI

								  if (	( show_cursor_gpad &&
										  ( (gpad_x != last_gpad_x) || (gpad_y != last_gpad_y) ||
										    (mouse_left_button != last_mouse_left_button) ) )
										||
										( show_cursor_gpad != last_show_cursor_gpad ) ) 
									Send_GPad_Packet();
                                }
                            }
                    break;

               case ERROR_REPORTED:
                    break;
                default:
                    break;

		}
	}
}



/****************************************************************************
  Function:
    void App_ProcessInputReport(void)

  Description:
    This function processes input report received from HID device.

  Precondition:
    None
***************************************************************************/
void App_ProcessInputReport(void)
{
	CHAR data;

   /* process input report received from device */
	USBHostHID_ApiImportData( Appl_raw_report_buffer.ReportData,
								Appl_raw_report_buffer.ReportSize,
                          		Appl_Button_report_buffer, 
								&Appl_Mouse_Buttons_Details );

    USBHostHID_ApiImportData( Appl_raw_report_buffer.ReportData, 
								Appl_raw_report_buffer.ReportSize,
                          		Appl_XY_report_buffer, 
								&Appl_XY_Axis_Details );

#ifndef TUNE_TIMING
	// update lightpen position, if cursor is on

	if (show_cursor) {

		data = Appl_XY_report_buffer[0];	// X axis

		xpos += data;

		if (xpos < 0)
			xpos = 0;
		else if (xpos > MAX_XPOS)
			xpos = MAX_XPOS;

    	data = Appl_XY_report_buffer[1];	// Y axis
		
		ypos += data;

		if (ypos < 0)
			ypos = 0;
		else if (ypos > MAX_YPOS)
			ypos = MAX_YPOS;
	}
#endif

	// update G-Pad position, if cursor is on

	if (show_cursor_gpad) {

		data = Appl_XY_report_buffer[0];	// X axis

		gpad_x += data;

		if (gpad_x < 0)
			gpad_x = 0;
		else if (gpad_x > MAX_GPAD_X)
			gpad_x = MAX_GPAD_X;

    	data = Appl_XY_report_buffer[1];	// Y axis
		
		gpad_y -= data;						// G-Pad Y axis is reversed

		if (gpad_y < 0)
			gpad_y = 0;
		else if (gpad_y > MAX_GPAD_Y)
			gpad_y = MAX_GPAD_Y;
	}

#ifndef TUNE_TIMING
	if(Appl_Button_report_buffer[0])
		mouse_left_button = 1;
	else
		mouse_left_button = 0;

	if(Appl_Button_report_buffer[1]) {		// right button toggles cursor on/off
		show_cursor = !show_cursor;
		show_cursor_gpad = !show_cursor_gpad;
	}
#else
	if(Appl_Button_report_buffer[0]) {
		dump_pos_timing();
		xpos++;
		adjust = 0;
	}	

	if(Appl_Button_report_buffer[1])
		adjust++;
#endif
}


/****************************************************************************
  Function:
    void App_Detect_Device(void)

  Description:
    This function monitors the status of device connected/disconnected

  Precondition:
    None
***************************************************************************/

void App_Detect_Device(void)
{
  if(!USBHostHID_ApiDeviceDetect())
  {
     App_State_Mouse = DEVICE_NOT_CONNECTED;
  }
}


/****************************************************************************
  Function:
    BOOL USB_HID_DataCollectionHandler(void)
  Description:
    This function is invoked by HID client , purpose is to collect the 
    details extracted from the report descriptor. HID client will store
    information extracted from the report descriptor in data structures.
    Application needs to create object for each report type it needs to 
    extract.

    For ex: HID_DATA_DETAILS Appl_ModifierKeysDetails;

    HID_DATA_DETAILS is defined in file usb_host_hid_appl_interface.h

    Each member of the structure must be initialized inside this function.
    Application interface layer provides functions :
    USBHostHID_ApiFindBit()
    USBHostHID_ApiFindValue()

    These functions can be used to fill in the details as shown in the demo
    code.

  Precondition:
    None

  Parameters:
    None

  Return Values:
    TRUE    - If the report details are collected successfully.
    FALSE   - If the application does not find the the supported format.

  Remarks:
    This Function name should be entered in the USB configuration tool
    in the field "Parsed Data Collection handler".
    If the application does not define this function , then HID cient 
    assumes that Application is aware of report format of the attached
    device.
***************************************************************************/

BOOL USB_HID_DataCollectionHandler(void)
{
  	BYTE NumOfReportItem = 0;
  	BYTE i;
  	USB_HID_ITEM_LIST* pitemListPtrs;
  	USB_HID_DEVICE_RPT_INFO* pDeviceRptinfo;
  	HID_REPORTITEM *reportItem;
  	HID_USAGEITEM *hidUsageItem;
  	BYTE usageIndex;
  	BYTE reportIndex;

  	pDeviceRptinfo = USBHostHID_GetCurrentReportInfo(); // Get current Report Info pointer
  	pitemListPtrs = USBHostHID_GetItemListPointers();   // Get pointer to list of item pointers

  	BOOL status = FALSE;

   /* Find Report Item Index for Modifier Keys */
   /* Once report Item is located , extract information from data structures provided by the parser */
	NumOfReportItem = pDeviceRptinfo->reportItems;

   	for(i=0;i<NumOfReportItem;i++)
    {
       reportItem = &pitemListPtrs->reportItemList[i];
       if((reportItem->reportType==hidReportInput) && (reportItem->dataModes == (HIDData_Variable|HIDData_Relative))&&
           (reportItem->globals.usagePage==USAGE_PAGE_GEN_DESKTOP))
        {
           /* We now know report item points to modifier keys */
           /* Now make sure usage Min & Max are as per application */
            usageIndex = reportItem->firstUsageItem;
            hidUsageItem = &pitemListPtrs->usageItemList[usageIndex];

            reportIndex = reportItem->globals.reportIndex;
            Appl_XY_Axis_Details.reportLength = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
            Appl_XY_Axis_Details.reportID = (BYTE)reportItem->globals.reportID;
            Appl_XY_Axis_Details.bitOffset = (BYTE)reportItem->startBit;
            Appl_XY_Axis_Details.bitLength = (BYTE)reportItem->globals.reportsize;
            Appl_XY_Axis_Details.count=(BYTE)reportItem->globals.reportCount;
            Appl_XY_Axis_Details.interfaceNum= USBHostHID_ApiGetCurrentInterfaceNum();
        }
        else if((reportItem->reportType==hidReportInput) && (reportItem->dataModes == HIDData_Variable)&&
           (reportItem->globals.usagePage==USAGE_PAGE_BUTTONS))
        {
           /* We now know report item points to modifier keys */
           /* Now make sure usage Min & Max are as per application */
            usageIndex = reportItem->firstUsageItem;
            hidUsageItem = &pitemListPtrs->usageItemList[usageIndex];

            reportIndex = reportItem->globals.reportIndex;
            Appl_Mouse_Buttons_Details.reportLength = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
            Appl_Mouse_Buttons_Details.reportID = (BYTE)reportItem->globals.reportID;
            Appl_Mouse_Buttons_Details.bitOffset = (BYTE)reportItem->startBit;
            Appl_Mouse_Buttons_Details.bitLength = (BYTE)reportItem->globals.reportsize;
            Appl_Mouse_Buttons_Details.count=(BYTE)reportItem->globals.reportCount;
            Appl_Mouse_Buttons_Details.interfaceNum= USBHostHID_ApiGetCurrentInterfaceNum();
        }
    }

   if(pDeviceRptinfo->reports == 1)
    {
        Appl_raw_report_buffer.Report_ID = 0;
        Appl_raw_report_buffer.ReportSize = (pitemListPtrs->reportList[reportIndex].inputBits + 7)/8;
//        Appl_raw_report_buffer.ReportData = (BYTE*)malloc(Appl_raw_report_buffer.ReportSize);
        Appl_raw_report_buffer.ReportPollRate = pDeviceRptinfo->reportPollingRate;
        status = TRUE;
    }

    return(status);
}

