/*******************************************************************************
Copyright 2016 Microchip Technology Inc. (www.microchip.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

To request to license the code under the MLA license (www.microchip.com/mla_license), 
please contact mla_licensing@microchip.com
*******************************************************************************/

#include "mcc_generated_files/usb/usb_host_hid.h"
// *****************************************************************************
// *****************************************************************************
// Type definitions
// *****************************************************************************
// *****************************************************************************

typedef enum _APP_STATE
{
    DEVICE_NOT_CONNECTED,
    WAITING_FOR_DEVICE,
    DEVICE_CONNECTED, /* Device Enumerated  - Report Descriptor Parsed */
    GET_INPUT_REPORT, /* perform operation on received report */
    INPUT_REPORT_PENDING,
    ERROR_REPORTED
} MOUSE_STATE;


typedef struct
{
    MOUSE_STATE state;
    bool inUse;

    uint8_t *buffer;
    uint16_t size;
    uint16_t pollRate;
    uint8_t address;

    struct
    {       
        struct
        {
            HID_DATA_DETAILS details;
            HID_USER_DATA_SIZE data[5];
        } parsed;
    } deflection;

    struct
    {
        struct
        {
            HID_DATA_DETAILS details;
            HID_USER_DATA_SIZE data[20];
        } parsed;
    } buttons;

} MOUSE;

/*********************************************************************
* Function: void APP_HostHIDMouseInitialize(void);
*
* Overview: Initializes the demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_HostHIDMouseInitialize();

/*********************************************************************
* Function: void APP_HostHIDMouseTasks(void);
*
* Overview: Keeps the demo running.
*
* PreCondition: The demo should have been initialized via
*   the APP_HostHIDMouseInitialize() function
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_HostHIDMouseTasks(uint8_t deviceAddress);

/*********************************************************************
* Function: bool APP_HostHIDMouseReportParser(void);
*
* Overview: parses the HID report to determine if this device is supported
*
* PreCondition: None
*
* Input: None
*
* Output: true if supported, false otherwise
*
********************************************************************/
bool APP_HostHIDMouseReportParser(void);

MOUSE* GetMOUSE();
