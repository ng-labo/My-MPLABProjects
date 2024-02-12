#define _XTAL_FREQ 16000000

#include <pic.h>
#include <htc.h>
#include <pic12f1822.h>

// configuration 1
#pragma config FOSC     = INTOSC   // Internal Osc
#pragma config WDTE     = OFF      // Watch dog timer
#pragma config PWRTE    = ON       // run after 64ms on power
#pragma config MCLRE    = OFF      // external reset
#pragma config CP       = OFF      // protect program memory
#pragma config CPD      = OFF      // protect data memory
#pragma config BOREN    = ON       // check power down
#pragma config CLKOUTEN = OFF      // CLKOUT on RA4
#pragma config IESO     = OFF      //
#pragma config FCMEN    = OFF      // 

// configuration 2
#pragma config WRT    = OFF        // protect flash memory
#pragma config PLLEN  = OFF        // PLL enable
#pragma config STVREN = ON         // reset on stack over/underflow
#pragma config BORV   = HI         // check power down--2.5
//#pragma config LVP    = OFF        // low power supply programing
// 5859 INTs = 1Ah
#define TARGET_INTs_COUNTER 5859
/* PIC12F1822
 * VDD 1 8 VSS
 * RA5 2 7 RA0
 * RA4 3 6 RA1
 * RA3 4 5 RA2
 */
void main(void) {
    //OSCCONbits.SPLLEN = 0b;
    //OSCCONbits.IRCF   = 0b1011; // 1MHz
    //OSCCONbit2 not used
    //OSCCONbits.SCS    = 0b10; // internal
    OSCCON = 0b01111010; // 16MHz

    ANSELA = 0;    
    TRISA = 0b00000100;

    // POWER-SUPPLY
    LATAbits.LATA4 = 1;
    // LED
    for(int i=0;i<3;i++){
        LATAbits.LATA5 = 1;
        __delay_ms(500);
        LATAbits.LATA5 = 0;
        __delay_ms(500);
    }
    unsigned int counter = 0;
    while(1) {
        if(PORTAbits.RA2==0){
            LATAbits.LATA5 = ~LATAbits.LATA5;
            counter++;
            if(counter>TARGET_INTs_COUNTER){
                // stop power supply
                LATAbits.LATA4 = 0;
                // leave ON LED
                LATAbits.LATA5 = 1;
                break;
            }
            while(PORTAbits.RA2==0);
        }
    }
    while(1);
    return;
}
