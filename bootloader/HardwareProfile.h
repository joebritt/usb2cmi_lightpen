// CMI Mouse Interface HardwareProfile.h

#ifndef _HARDWARE_PROFILE_CMI_MOUSE_H_
#define _HARDWARE_PROFILE_CMI_MOUSE_H_


// ---------------- my HardwareProfile for the USB2CMI interface -------------------

#define PIC32MX440F128H
    
#define GetSystemClock()            80000000UL
#define GetPeripheralClock()        80000000UL  // Will be divided down
#define GetInstructionClock()       (GetSystemClock() / 2)

// Clock values
#define MILLISECONDS_PER_TICK       10                  // -0.000% error
#define TIMER_PRESCALER             TIMER_PRESCALER_8   // At 60MHz
#define TIMER_PERIOD                37500               // At 60MHz


#include <p32xxxx.h>
#include <plib.h>

#if 0
		#define tris_usb_bus_sense  TRISBbits.TRISB5    // Input
//  //    
//  //    #if defined(USE_USB_BUS_SENSE_IO)
//  //    #define USB_BUS_SENSE       PORTBbits.RB5
//  //    #else
        #define USB_BUS_SENSE       1
//  //    #endif
//  //    
        #define tris_self_power     TRISAbits.TRISA2    // Input
//  //    
//  //    #if defined(USE_SELF_POWER_SENSE_IO)
//  //    #define self_power          PORTAbits.RA2
//  //    #else
        #define self_power          1
//  //    #endif
        
        // External Transceiver Interface
        #define tris_usb_vpo        TRISBbits.TRISB3    // Output
        #define tris_usb_vmo        TRISBbits.TRISB2    // Output
        #define tris_usb_rcv        TRISAbits.TRISA4    // Input
        #define tris_usb_vp         TRISCbits.TRISC5    // Input
        #define tris_usb_vm         TRISCbits.TRISC4    // Input
        #define tris_usb_oe         TRISCbits.TRISC1    // Output
        
        #define tris_usb_suspnd     TRISAbits.TRISA3    // Output 
#endif

#endif  


