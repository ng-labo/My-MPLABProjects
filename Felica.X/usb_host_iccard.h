#ifndef __USBHOSTICCARD_H__
#define __USBHOSTICCARD_H__

#include <stdint.h>
#include <stdbool.h>

// Section: Constants

// USB Client Events
// EVENT_GENERIC_BASE is defined in usb_common.h

#define EVENT_GENERIC_ATTACH  (EVENT_GENERIC_BASE+0)
#define EVENT_GENERIC_DETACH  (EVENT_GENERIC_BASE+1)
#define EVENT_GENERIC_TX_DONE (EVENT_GENERIC_BASE+2)
#define EVENT_GENERIC_RX_DONE (EVENT_GENERIC_BASE+3)

// Section: USB Data Structures

typedef struct _GENERIC_DEVICE_ID {
    uint16_t  vid;
    uint16_t  pid;
    uint8_t   deviceAddress;
} GENERIC_DEVICE_ID;

typedef struct _GENERIC_DEVICE {
    GENERIC_DEVICE_ID ID;             // Identification information about the device
    uint32_t          rxLength;       // Number of bytes received in the last IN transfer
    uint8_t           clientDriverID; // ID to send when issuing a Device Request
    
    union {
        uint8_t val;                       // uint8_t representation of device status flags
        struct {
            uint8_t initialized    : 1;    // Driver has been initialized
            uint8_t txBusy         : 1;    // Driver busy transmitting data
            uint8_t rxBusy         : 1;    // Driver busy receiving data
        };
    } flags;                            // Generic client driver status flags
    
    uint8_t bulk_endpoint_in;
    uint8_t bulk_endpoint_out;
} GENERIC_DEVICE;

// Global Variables
extern GENERIC_DEVICE   gc_DevData; // Information about the attached device.

// Section: Host Stack Interface Functions

bool USBHostGenericInitialize ( uint8_t address, uint32_t flags, uint8_t clientDriverID );
bool USBHostGenericEventHandler ( uint8_t address, USB_EVENT event, void *data, uint32_t size );

// Section: Function Prototypes and Macro Functions

bool USBHostGenericGetDeviceAddress(GENERIC_DEVICE_ID *pDevID);
uint8_t USBHostGenericRead( uint8_t deviceAddress, void *buffer, uint32_t length);
uint8_t USBHostGenericWrite( uint8_t deviceAddress, void *buffer, uint32_t length);

#define API_VALID(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? true : false )

#define USBHostGenericDeviceDetached(a) ( (((a)==gc_DevData.ID.deviceAddress) && gc_DevData.flags.initialized == 1) ? false : true )
#define USBHostGenericGetRxLength(a) ( (API_VALID(a)) ? gc_DevData.rxLength : 0 )
#define USBHostGenericRxIsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.rxBusy == 1) ? true : false) : true )
#define USBHostGenericTxIsBusy(a) ( (API_VALID(a)) ? ((gc_DevData.flags.txBusy == 1) ? true : false) : true )

#endif //__USBHOSTICCARD_H__
