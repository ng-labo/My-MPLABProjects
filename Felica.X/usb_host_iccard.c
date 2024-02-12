/*
 * USB-HOST application implementaion for Contactless IC Card device.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "mcc_generated_files/usb/usb.h"
#include "usb_host_iccard.h"

//#define DEBUG_MODE

#ifdef DEBUG_MODE
#include "mcc_generated_files/uart1.h"
#include <stdio.h>
#endif

// Global Variables

GENERIC_DEVICE  gc_DevData;

// Section: Host Stack Interface Functions

bool USBHostGenericInitialize ( uint8_t address, uint32_t flags, uint8_t clientDriverID ) {
    uint8_t *pDesc;
    int i;

    // Initialize state
    gc_DevData.flags.initialized = 0;
    gc_DevData.rxLength  = 0;
    gc_DevData.flags.val = 0;

    // Save device the address
    gc_DevData.ID.deviceAddress = address;
    
    // VID, & PID
    pDesc  = USBHostGetDeviceDescriptor(address);
    pDesc += 8;
    gc_DevData.ID.vid  =  (uint16_t)*pDesc;       pDesc++;
    gc_DevData.ID.vid |= ((uint16_t)*pDesc) << 8; pDesc++;
    gc_DevData.ID.pid  =  (uint16_t)*pDesc;       pDesc++;
    gc_DevData.ID.pid |= ((uint16_t)*pDesc) << 8; pDesc++;

    // Client Driver ID
    gc_DevData.clientDriverID = clientDriverID;

#ifdef DEBUG_MODE
    printf( "USB device Client Initalized:\n");
    printf( " flags=0x%2lx address=%d\n", flags, address );
    printf( " VID:PID=0x%04x:0x%04x\n",  gc_DevData.ID.vid, gc_DevData.ID.pid );
#endif

    // getting bulk-endpoint numbers
    
    pDesc = USBHostGetCurrentConfigurationDescriptor( address );
    i = 0;
    // noOfInterface = pDesc[i+4];
    while (i < ((USB_CONFIGURATION_DESCRIPTOR *)pDesc)->wTotalLength) {
        if (pDesc[i+1] == USB_DESCRIPTOR_ENDPOINT) {
            if(pDesc[i+3] == USB_TRANSFER_TYPE_BULK && (pDesc[i+2]&0x80) == _EP_IN){
                gc_DevData.bulk_endpoint_in = pDesc[i+2];
            }
            if(pDesc[i+3] == USB_TRANSFER_TYPE_BULK && (pDesc[i+2]&0x80) == _EP_OUT) {
                gc_DevData.bulk_endpoint_out = pDesc[i+2];
            }
        }
        i += pDesc[i];
    }
#ifdef DEBUG_MODE
    printf("bulk_endpoint_in=0x%02x\n", gc_DevData.bulk_endpoint_in);
    printf("bulk_endpoint_out=0x%02x\n", gc_DevData.bulk_endpoint_out);
#endif
    if (gc_DevData.bulk_endpoint_in != 0 && gc_DevData.bulk_endpoint_out != 0) {
        // Generic Client Driver Init Complete.
        gc_DevData.flags.initialized = 1;
    }

    return (gc_DevData.flags.initialized==1);
}

bool USBHostGenericEventHandler( uint8_t address, USB_EVENT event, void *data, uint32_t size ) {
#ifdef DEBUG_MODE2
    //printf("[USBHostGenericEventHandler:event %d size %d]\n", event, size);
#endif

    // Make sure it was for our device
    if (address != gc_DevData.ID.deviceAddress) {
        return false;
    }

    // Handle specific events.
    switch (event) {
    case EVENT_DETACH:
        gc_DevData.flags.val        = 0;
        gc_DevData.ID.deviceAddress = 0;
    #ifdef DEBUG_MODE
        printf( "USB Generic Client Device Detached: address=%d\n", address );
    #endif
        return true;

    // requires USB_ENABLE_TRANSFER_EVENT  definition in usb_host_config.h
    case EVENT_TRANSFER:
        if ( (data != NULL) && (size == sizeof(HOST_TRANSFER_DATA)) ) {
            uint32_t dataCount = ((HOST_TRANSFER_DATA *) data)->dataCount;

            if (((HOST_TRANSFER_DATA *) data)->bEndpointAddress == gc_DevData.bulk_endpoint_in) {
                gc_DevData.flags.rxBusy = 0;
                gc_DevData.rxLength = dataCount;
            } else if (((HOST_TRANSFER_DATA *) data)->bEndpointAddress == gc_DevData.bulk_endpoint_out) {
                gc_DevData.flags.txBusy = 0;
            } else {
                return false;
            }

            return true;

        } else {
            return false;
        }
    case EVENT_SUSPEND:
    case EVENT_RESUME:
    case EVENT_BUS_ERROR:
        if(gc_DevData.flags.rxBusy){
            gc_DevData.flags.rxBusy = 0;
            gc_DevData.rxLength = 0;
        }
    default:
        break;
    }

    return false;
}

// Section: Application Callable Functions

uint8_t USBHostGenericRead(uint8_t deviceAddress, void *buffer, uint32_t length) {
    uint8_t RetVal;

    if (!API_VALID(deviceAddress)) return USB_INVALID_STATE;
    if (gc_DevData.flags.rxBusy)   return USB_BUSY;
    
    uint8_t endpoint = gc_DevData.bulk_endpoint_in;
    
    USBHostSetNAKTimeout(deviceAddress, endpoint, 1, 50);
    USBHostClearEndpointErrors(deviceAddress, endpoint);

    gc_DevData.flags.rxBusy = 1;
    gc_DevData.rxLength = 0;
    RetVal = USBHostRead(deviceAddress, endpoint, (uint8_t *) buffer, length);
    if (RetVal != USB_SUCCESS) {
        gc_DevData.flags.rxBusy = 0;
    }

    return RetVal;

}

uint8_t USBHostGenericWrite( uint8_t deviceAddress, void *buffer, uint32_t length ) {
    uint8_t RetVal;

    if (!API_VALID(deviceAddress)) return USB_INVALID_STATE;
    if (gc_DevData.flags.txBusy)   return USB_BUSY;
    
    uint8_t endpoint = gc_DevData.bulk_endpoint_out;
    USBHostSetNAKTimeout(deviceAddress, endpoint, 1, 100);
    USBHostClearEndpointErrors(deviceAddress, endpoint);

    gc_DevData.flags.txBusy = 1;
    RetVal = USBHostWrite(deviceAddress, endpoint, (uint8_t *) buffer, length);
    if (RetVal != USB_SUCCESS) {
        gc_DevData.flags.txBusy = 0;
    }

    return RetVal;
}
/* end of file*/
