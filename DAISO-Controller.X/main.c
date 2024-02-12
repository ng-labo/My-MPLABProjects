/*
  Includes generated files by MCC
*/
#include "defines.h"

#include "mcc_generated_files/system.h"
#include "mcc_generated_files/usb/usb.h"
#include "mcc_generated_files/usb/usb_host_local.h"
#include "mcc_generated_files/uart1.h"

#include "app_host_hid_mouse.h"
#include <stdio.h>
/*
 * Application
 */

int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    LATBbits.LATB7 = 0;
    LATBbits.LATB8 = 0;
    APP_HostHIDMouseInitialize();
    
    __delay_ms(1000);
    
    char* bootmsg[] = {
        "",
        "Hello !!",
        "DAISO-Controller"
    };
    int i;
    for(i=0;i<sizeof(bootmsg)/sizeof(char*);i++){
        printf("%s\n", bootmsg[i]);
    }
    
    uint8_t deviceAddress = 0;
    

    while(1){
        USBHostTasks();
        deviceAddress = USBHostHIDDeviceDetect();
        if(deviceAddress!=0){
            printf("deviceAddress %d\n", deviceAddress);
            break;
        }
        __delay_ms(1);
    }
    while(1){
        USBHostTasks();
        if(USBHostDeviceStatus(deviceAddress)==USB_DEVICE_ATTACHED){
            printf("ATTACHED\n");
            break;
        }
        __delay_ms(1);
    }

    MOUSE* mouse = GetMOUSE();
    while(!mouse->inUse){
        USBHostTasks();
        APP_HostHIDMouseTasks(deviceAddress);
        __delay_ms(100);
    }
    
    uint32_t cnt = 0;
    int8_t pos_x = 0, pos_y = 0, wheel = 0;
    int8_t last_pos_x = 0, last_pos_y = 0, last_wheel = 0;
    bool isupdate = false;
    int last_sec = 0;
    bool inmode = false;
    
    while(1){
        USBHostTasks();
        APP_HostHIDMouseTasks(deviceAddress);
        
        if(mouse->deflection.parsed.data[0]){
            pos_x = (int8_t) mouse->deflection.parsed.data[0];
        }
        if(mouse->deflection.parsed.data[1]){
            pos_y = (int8_t) mouse->deflection.parsed.data[0];
        }
        if(mouse->deflection.parsed.data[2]){
            wheel = (int8_t) mouse->deflection.parsed.data[2];
        }
        isupdate = (pos_x!=last_pos_x) || (pos_y!=last_pos_y) || (wheel!=last_wheel);
        if(isupdate||mouse->buttons.parsed.data[0]||mouse->buttons.parsed.data[1]||mouse->buttons.parsed.data[2]){ // || cnt%1000==0){
            last_pos_x = pos_x;
            last_pos_y = pos_y;
            last_wheel = wheel;
            printf("[%d|%d|%d](%2d,%2d) %d\n", 
                                mouse->buttons.parsed.data[0],
                                mouse->buttons.parsed.data[1],
                                mouse->buttons.parsed.data[2],
                                pos_x, pos_y, wheel);            
        }
        if(mouse->buttons.parsed.data[0]){
            LATBbits.LATB7 = 1;
            LATBbits.LATB8 = 0;
        }else if(mouse->buttons.parsed.data[1]){
            LATBbits.LATB7 = 0;
            LATBbits.LATB8 = 1;
        }else{
            LATBbits.LATB7 = 0;
            LATBbits.LATB8 = 0;            
        }
        
        cnt++;
        __delay_ms(1);
        
    }

    return 1;
}
/**
 End of File
*/

