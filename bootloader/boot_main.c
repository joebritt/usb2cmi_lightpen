/*
********************************************************************************
                                                                                
Software License Agreement                                                      
                                                                                
Copyright � 2008 Microchip Technology Inc. and its licensors.  All         
rights reserved.                                                                
                                                                                
Microchip licenses to you the right to: (1) install Software on a single        
computer and use the Software with Microchip 16-bit microcontrollers and        
16-bit digital signal controllers ("Microchip Product"); and (2) at your        
own discretion and risk, use, modify, copy and distribute the device            
driver files of the Software that are provided to you in Source Code;           
provided that such Device Drivers are only used with Microchip Products         
and that no open source or free software is incorporated into the Device        
Drivers without Microchip's prior written consent in each instance.             
                                                                                
You should refer to the license agreement accompanying this Software for        
additional information regarding your rights and obligations.                   
                                                                                
SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY         
KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY              
WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A          
PARTICULAR PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE             
LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY,               
CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY           
DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY         
INCIDENTAL, SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR         
LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY,                 
SERVICES, ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY         
DEFENSE THEREOF), OR OTHER SIMILAR COSTS.                                       
                                                                                
********************************************************************************

 Change History:
  Rev   Description
  ----  -----------------------------------------
  2.7   Updated to include PIC32 support

********************************************************************************
*/

#include "Compiler.h"
#include "GenericTypedefs.h"
#include "HardwareProfile.h"
#include "boot.h"
#include "MDD File System\FSIO.h"
#include "USB\usb.h"
#include "USB\usb_host_msd_scsi.h"


// *****************************************************************************
// *****************************************************************************
// Configuration Bits
// *****************************************************************************
// *****************************************************************************

#define PLL_96MHZ_OFF   0xFFFF
#define PLL_96MHZ_ON    0xF7FF


// *****************************************************************************
// *****************************************************************************
// Configuration Bits
// *****************************************************************************
// *****************************************************************************

// MUST BE THE SAME AS THE APPLICATION CONFIGURATION BITS!

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
    #pragma config POSCMOD  = EC            // Primary Oscillator
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect
    #pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
    #pragma config DEBUG    = ON           // Debugger Disabled for Starter Kit
#else
    #error Cannot define configuration bits.
#endif


// Macro used to call main application
/****************************************************************************
  Function:
    int BootApplication ( void )

  Description:
    This macro is used to launch the application.

  Precondition:
    The application image must be correctly programmed into Flash at the 
    appropriate entry point.

  Parameters:
    None

  Returns:
    This call does not normally return.

  Remarks:
    The application's entry point is defined by the APPLICATION_ADDRESS
    macro in the boot_config.h header file.
***************************************************************************/

#define BootApplication()       (((int(*)(void))(APPLICATION_ADDRESS))())


/****************************************************************************
  Function:
    void LoadApplication ( void )

  Description:
    This routine attempts to initialize and attach to the boot medium, locate
    the boot image file, and load and program it to the flash.

  Precondition:
    The boot loader IO must have been initialized.

  Parameters:
    None

  Returns:
    None

  Remarks:
    None
***************************************************************************/
void LoadApplication ( void )
{
    BOOL    LoadingApplication      = TRUE;
    BOOL    TransportInitialized    = FALSE;
    //BOOL    FileSystemInitialized   = FALSE;
    BOOL    BootMediumAttached      = FALSE;
    BOOL    BootImageFileFound      = FALSE;
    BOOL    BootImageFileError      = FALSE;
    int     ErrCount                = 0;

	int		no_drive_count			= 0;

    // Loader main loop
    while (LoadingApplication)
    {
        if (TransportInitialized)
        {
            // Keep the boot medium alive
            TransportInitialized = BLMedia_MonitorMedia();
            
            // Check for the boot medium to attach
            BootMediumAttached = BLMedia_MediumAttached();
            if (BootMediumAttached)
            {
                if (!BootImageFileError)
                {
                    // Attempt to locate the boot image file
                    BootImageFileFound = BLMedia_LocateFile(BOOT_FILE_NAME);
                    if (BootImageFileFound)
                    {
                        BLIO_ReportBootStatus(BL_FOUND_FILE, "BL: Application image file has been found\r\n");

                        // Read the boot image file and program it to Flash
                        if (BLMedia_LoadFile(BOOT_FILE_NAME))
                        {
                            LoadingApplication = FALSE;
                            BLIO_ReportBootStatus(BL_PROGRAMMED, "BL: Application image has been programmed\r\n");
                        }
                        else
                        {
                            // Error reported by lower layer
                            BootImageFileError = TRUE;
                        }
                    }
                    else
                    {
                        // Count and, if necessary, report the errors locating the file
                        ErrCount++;
                        if (ErrCount > MAX_LOCATE_RETRYS)
                        {
                            ErrCount = 0;
                            BootImageFileError = TRUE;
                            BLIO_ReportBootStatus(BL_FILE_NOT_FOUND, "BL: Application image not found\r\n");
                        }
                    }
                }
            }
            else
            {
                BootImageFileError = FALSE;
				
				UpdateShowBootStatus( SHOW_WAITING_DRIVE_READY );

				if((no_drive_count & 0xff) == 0xff) {
					BLIO_ReportBootStatus(0, "BL: waiting...\r\n");
				}

				if(no_drive_count++ > 65536)
					LoadingApplication = FALSE;
            }
        }
        else
        {
            // Initialize transport layer used to access the boot image's file system
            TransportInitialized = BLMedia_InitializeTransport();
            if (TransportInitialized)
            {
                BLIO_ReportBootStatus(BL_TRANSPORT_INIT, "BL: Transport initialized\r\n");
            }
        }

        // Watch for user to abort the load
        if (BLIO_AbortLoad())
        {
            LoadingApplication = FALSE;
        }
    }

} // LoadApplication


/****************************************************************************
	Utility routines added by Joe Britt for boot status on USB2CMI board

	There are no switches/buttons on USB2CMI.
	There are 2 LEDs, one green and one yellow.

	When the device boots, it waits for a bit to see if a USB thumbdrive
	appears on the USB interface.  While it is waiting (state WAIT_USB_DRIVE_READY),
	the green & yellow LEDs alternately toggle.

	If a USB thumbdrive is found, and the needed hexfile is found (for the
	mouse interface it is cmimouse.hex), the green LED is turned on solid
	and the yellow LED flashes as blocks are programmed.

	If we start to launch the application in flash (either just programmed or
	because no USB thumbdrive or .hex file was found), we turn off both LEDs.
	The application code will then start with the LEDs off.

 ****************************************************************************/

void yellow_led(int state)
{
	if(state)
		PORTBCLR = 0x0002;
	else
		PORTBSET = 0x0002;
}

void toggle_yellow_led()
{
	PORTBINV = 0x0002;
}

void green_led(int state)
{
	if(state)
		PORTBCLR = 0x0001;
	else
		PORTBSET = 0x0001;
}

void toggle_green_led()
{
	PORTBINV = 0x0001;
}

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

void SendStr_UART1(char *s) {
	while(*s)
		SendByte_UART1(*s++);
}

void hw_init(void)
{
	// Yellow (bottom) LED on RB1
	// Green (top) LED on RB0

	TRISBbits.TRISB1 = 0;	// RB1 is OUTPUT
	TRISBbits.TRISB0 = 0;	// RB0 is OUTPUT

	yellow_led(0);
	green_led(0);

	Init_UART1( 38400 );
}


void BLIO_ReportBootStatus ( BOOT_STATUS status, char *message ) {
	BootStatus = status;
	SendStr_UART1( message );
}

unsigned char hextbl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

void printhex32(unsigned int i) {
	int x;

	for(x=0;x!=8;x++) {
		SendByte_UART1(hextbl[(i>>28)&0xf]);	
		i <<= 4;
	}
}

void UpdateShowBootStatus( SHOW_BOOT_STATUS s ) {
	static int wait_rdy_cnt;
	static int pgm_cnt;

	switch(s) {
		case SHOW_POWER_ON:
				green_led(1);
				yellow_led(0);
				wait_rdy_cnt = 0;
				pgm_cnt = 0;
				break;
	
		case SHOW_WAITING_DRIVE_READY:
				if((wait_rdy_cnt++ & 0xfff) == 0xfff) {
					toggle_green_led();
					toggle_yellow_led();
				}
				break;

		case SHOW_PROGRAMMING:
				green_led(1);					// make sure green one is on
				if((pgm_cnt++ & 0x1f) == 0x1f)	// blink yellow LED while we program
					toggle_yellow_led();
				break;

		case SHOW_LAUNCHING_APP:
				green_led(0);
				yellow_led(0);
				break;
	}
}

/****************************************************************************
  Function:
    int main(void)

  Description:
    This is the boot loader's main C-language entry point.  It initializes 
    the boot loader's IO, and uses it to determine if the boot loader should
    be invoked.  If so, it attempts to load the application.  After loading
    and programming the boot image (or immediately, if the boot loader is
    not invoked), it checks the to see if the image in Flash is valid and, 
    if so, calls the application's main entry point.

  Precondition:
    The appropriate startup code must have been executed.

  Parameters:
    None

  Returns:
    Integer exit code (0)

  Remarks:
    This routine is executed only once, after a reset.
***************************************************************************/

int main ( void )
{
    #if defined(__PIC32MX__)
        // Initialize the MCU
        SYSTEMConfigWaitStatesAndPB( GetSystemClock() );
        CheKseg0CacheOn();
        INTEnableSystemMultiVectoredInt();
    #endif

    // Initialize the boot loader IO
    BLIO_InitializeIO();
    BLIO_ReportBootStatus(BL_RESET, "\r\n\r\nBL: *** Reset ***\r\n");

    // Check to see if the user requested loading of a new application
    if (BLIO_LoaderEnabled())
    {
       #if defined(PIC24FJ64GB004_PIM) || defined(PIC24FJ256DA210_DEV_BOARD)
    	//On the PIC24FJ64GB004 Family of USB microcontrollers, the PLL will not power up and be enabled
    	//by default, even if a PLL enabled oscillator configuration is selected (such as HS+PLL).
    	//This allows the device to power up at a lower initial operating frequency, which can be
    	//advantageous when powered from a source which is not gauranteed to be adequate for 32MHz
    	//operation.  On these devices, user firmware needs to manually set the CLKDIV<PLLEN> bit to
    	//power up the PLL.
        {
            unsigned int pll_startup_counter = 600;
            CLKDIVbits.PLLEN = 1;
            while(pll_startup_counter--);
        }
    
        //Device switches over automatically to PLL output after PLL is locked and ready.
        #endif

        #if defined(__PIC24F__)
        INTCON2bits.ALTIVT = 1;
        #endif

		UpdateShowBootStatus( SHOW_POWER_ON );			// green led on

        BLIO_ReportBootStatus(BL_LOADING, "BL: Loading new application image\r\n");
        LoadApplication();
    }

    // Launch the application if the image in Flash is valid
    if (BL_ApplicationIsValid())
    {
        BLIO_ReportBootStatus(BL_BOOTING, "BL: Launching application\r\n");

		UpdateShowBootStatus( SHOW_LAUNCHING_APP );		// here we go!

        // Must disable all interrupts
        BLMedia_DeinitializeTransport();
        

        #if defined(__PIC32MX__)

        INTDisableInterrupts();

        #else

        U1IE = 0;
        U1IR = 0xFF;
        IEC5 = 0;
        IFS5 = 0;
        INTCON2bits.ALTIVT = 0;

        #endif

        ////////////////////////////
        // Launch the application //
        ////////////////////////////
        BootApplication();
    }

    // Should never get here if a valid application was loaded.
    BLIO_ReportBootStatus(BL_BOOT_FAIL, "BL: Application failed to launch\r\n");
    BL_ApplicationFailedToLaunch();

    // Hang system
    while (1)
        ;

    return 0;

} // main

/*
*******************************************************************************
EOF
*******************************************************************************
*/

