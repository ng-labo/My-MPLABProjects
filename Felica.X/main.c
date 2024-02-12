/*
  Includes generated files by MCC
*/
#include "defines.h"
#include "i2c_local.h"

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/usb/usb.h"
#include "mcc_generated_files/usb/usb_host_local.h"
#include "mcc_generated_files/uart1.h"

#include "usb_host_iccard.h"
#include "mylib/i2c.h"

#include "rcs380.h"

#include <stdio.h>
/*
 * Application
 */

int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    USBHostInit(0);
    
    __delay_ms(1000);
    OledInit(OLEDROTATION);
    
    char* bootmsg[] = {
        "Hello !!",
        "Support RC-S380"
    };
    int i;
    for(i=0;i<sizeof(bootmsg)/sizeof(char*);i++){
        OledPrint("%s\n", bootmsg[i]);
        //printf("%s\n", bootmsg[i]);
    }
    
    LATAbits.LATA1 = 1;
    rcs380_initialize();
    LATAbits.LATA1 = 0;
    
    OledPrint("device ok\n");
    
    while(1){
        if(rcs380_sense()){
            LATAbits.LATA1 = 1;
        }
        __delay_ms(1000);        
    }

    return 1;
}
/**
 End of File
*/

