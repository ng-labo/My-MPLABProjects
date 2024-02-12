/* includes and defines depending on a project */
#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#define PrintOnDisplay OledPrint

/* includes for application */
#include <stdint.h>
#include <stdio.h>
#include "mcc_generated_files/usb/usb.h"
#include "mcc_generated_files/usb/usb_host.h"
#include "usb_host_iccard.h"

// include for this .c file
#include "rcs380.h"

//#define DEBUG_MODE
#ifdef DEBUG_MODE
#include "mcc_generated_files/uart1.h"
static void hexdump(const uint8_t* b, const int sz) {
    int i;
    int l = sz;
    while(l>0){
        int lp = (l>8) ? 8 : l;
        for (i = 0; i < lp; i++) printf("%02X ", *b++);
        printf("\n");
        l -= 8;
    }
}
#endif

static int rcs380_write(GENERIC_DEVICE* dev, uint8_t* cmd, int cmd_len){
    uint8_t r;
    int i;
    
    r = USBHostGenericWrite(dev->ID.deviceAddress, cmd, cmd_len);
    if(r){
#ifdef DEBUG_MODE
        printf("...Write()=%d txBusy=%d\n",r,  dev->flags.txBusy);
#endif
        return 1;
    }
    for(i=0;i<WAITCOEF;i++){
        USBHostTasks();
        if(dev->flags.txBusy==0)break;
        __delay_ms(1);
    }
    
    return dev->flags.txBusy==0 ? 0 : 1;
}

static int rcs380_readwrite(GENERIC_DEVICE* dev, uint8_t* cmd,
        int cmd_len, uint8_t* retdata, int* ret_len){
    uint8_t r;
    int i;
    uint8_t recvdata[RECVBUFSIZE];
    uint8_t recv_len;
    
    //
    rcs380_write(dev, cmd, cmd_len);
    
    for(i=0;i<RECVBUFSIZE;i++) recvdata[i] = 0;
    r = USBHostGenericRead(dev->ID.deviceAddress, recvdata, RECVBUFSIZE);
#ifdef DEBUG_MODE
    printf("...Read()=%d rxBusy=%d\n",r,  dev->flags.rxBusy);
#endif
    if(r){
        return 1;
    }

    for(i=0;i<WAITCOEF;i++){
        USBHostTasks();
        if(dev->flags.rxBusy==0)break;
        __delay_ms(1);
    }
    if(dev->flags.rxBusy) return 1;
    
    recv_len = (uint8_t) dev->rxLength;
    if(recvdata[0]!=0x00||recvdata[1]!=0x00||recvdata[2]!=0xff||
       recvdata[3]!=0x00||recvdata[4]!=0xff||recvdata[5]!=0x00){
    #ifdef DEBUG_MODE
        printf("(ack):");  hexdump(recvdata, recv_len);
    #endif
        // not ack, unexpected response
        return 1;
    }
    
    for(i=0;i<RECVBUFSIZE;i++) recvdata[i] = 0;
    r = USBHostGenericRead(dev->ID.deviceAddress, recvdata, RECVBUFSIZE);
#ifdef DEBUG_MODE
    printf("*...Read()=%d rxBusy=%d\n",r,  dev->flags.rxBusy);
#endif
    if(r){
        return 1;
    }

    for(i=0;i<WAITCOEF;i++){
        USBHostTasks();
        if(dev->flags.rxBusy==0)break;
        __delay_ms(1);
    }
    if(dev->flags.rxBusy) return 1;
    
    if(recvdata[0]==0x00||recvdata[1]==0x00||
       recvdata[2]==0xff||recvdata[3]==0xff||recvdata[4]==0xff){
        if(recvdata[8]==0xd7&&recvdata[9]==cmd[9]+1){
            uint8_t data_len = recvdata[5]; // ignore recvdata[6]
            if(retdata&&data_len-2 <= *ret_len){
                for(i=0;i<data_len-2;i++) retdata[i] = recvdata[10+i];
                *ret_len = data_len-2;
            }
        #ifdef DEBUG_MODE
            printf("(data):\n");
            hexdump(&recvdata[10], data_len-2);
        #endif
        }else{
        #ifdef DEBUG_MODE
            printf("(raw-data):\n");
            hexdump(recvdata, 16);
        #endif
            return 1;
        }
    }else{
        #ifdef DEBUG_MODE
            printf("(raw-data):\n");
            hexdump(recvdata, 16);
        #endif
        // not data, unexpected response
        return 1;
    }
    return 0;
}

static uint8_t sendbuffer[] = 
    { 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
      0xd6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  };

static uint8_t invoke_command(GENERIC_DEVICE* dev, uint8_t cmd,
        const uint8_t* args, const int args_size, uint8_t* recvbuf, int* recvlen){
    uint8_t buf_idx = 5; // start to process
    sendbuffer[buf_idx++] = 2 + args_size;
    sendbuffer[buf_idx++] = 0;
    sendbuffer[buf_idx++] = (uint16_t) (256 - sendbuffer[5] - sendbuffer[6]); // set modulo
    buf_idx++; // skip position fixed 0xd6
    sendbuffer[buf_idx++] = cmd;
    uint16_t cksum = 0xd6 + sendbuffer[9];
    int i;
    for(i=0;i<args_size;i++){
        sendbuffer[buf_idx++] = args[i];
        cksum += args[i];
    }
    
    sendbuffer[buf_idx++] = (uint16_t) (256 - cksum); // set modulo
    sendbuffer[buf_idx++] = 0;

    return rcs380_readwrite(dev, sendbuffer, buf_idx, recvbuf, recvlen);
}

static uint8_t setCommandType(GENERIC_DEVICE* dev, uint8_t o) {
    uint8_t args[] = { 1 };
    return invoke_command(dev, 0x2a, args, sizeof(args), NULL, 0);
}

static uint8_t getFirmwareVersion(GENERIC_DEVICE* dev, int* version) {
    int vlen = 2;
    return invoke_command(dev, 0x20, NULL, 0, (uint8_t*) version, &vlen);
}

static uint8_t getPDDataVersion(GENERIC_DEVICE* dev, int* version) {
    int vlen = 2;
    return invoke_command(dev, 0x22, NULL, 0, (uint8_t*) version, &vlen);
}

static uint8_t switchRF(GENERIC_DEVICE* dev, uint8_t ch) {
    uint8_t args[] = { 0 };
    return invoke_command(dev, 0x06, args, sizeof(args), NULL, 0);
}

static uint8_t inSetRF(GENERIC_DEVICE* dev,
        uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4) {
    uint8_t args[] = { a1, a2, a3, a4 };
    return invoke_command(dev, 0x00, args, sizeof(args), NULL, 0);
}

static uint8_t inSetProtocol(GENERIC_DEVICE* dev, const uint8_t* protocol,
        const int size) {
    return invoke_command(dev, 0x02, protocol, size, NULL, 0);
}

static uint8_t inCommRF(GENERIC_DEVICE* dev, const uint8_t* data,
        const int size, uint8_t* codebytes, int* codelen){
    return invoke_command(dev, 0x04, data, size, codebytes, codelen);
}

extern GENERIC_DEVICE gc_DevData;

int rcs380_initialize() {
    GENERIC_DEVICE *dev = &gc_DevData;
    int fver = 0xffff;
    int dver = 0xffff;
    
    while(1){
        USBHostTasks();
        if(gc_DevData.flags.initialized)break;
        __delay_ms(10);
    }
    
    uint8_t SoftReset[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00};
    rcs380_readwrite(dev, SoftReset, sizeof(SoftReset), NULL, 0);

    if(setCommandType(dev, 1)){
        return 1;
    }
    
    // remenber endian
    if(getFirmwareVersion(dev, &fver)){
        return 1;
    }
    //PrintOnDisplay("firmware:%x.%x\n", fver>>8, fver&0xff);
    
    if(getPDDataVersion(dev, &dver)){
        return 1;
    }
    //PrintOnDisplay("package data:%x.%x\n", dver>>8, dver&0xff);

    if(switchRF(dev, 0)){
        return 1;
    }
    
    return 0;
}

int rcs380_sense() {
    GENERIC_DEVICE *dev = &gc_DevData;
    
    uint8_t codebytes[48];
    
    if(setCommandType(dev, 1)){
        return 1;
    }
    
    if(switchRF(dev, 0)){
        return 1;
    }
    
    // imitates github.com/nfcpy/src/nfc/clf/rcs380.py
    
    if(inSetRF(dev, 1, 1, 15, 1)){ // 212F
        return 1;
    }
    
    const uint8_t set_protocol_default[] = {
        0x00, 0x18, 0x01, 0x01, 0x02, 0x01, 0x03, 0x00,
        0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x08,
        0x08, 0x00, 0x09, 0x00, 0x0A, 0x00, 0x0B, 0x00,
        0x0C, 0x00, 0x0E, 0x04, 0x0F, 0x00, 0x10, 0x00,
        0x11, 0x00, 0x12, 0x00, 0x13, 0x06
    };
    
    if(inSetProtocol(dev, set_protocol_default, sizeof(set_protocol_default))){
        return 1;
    }
    
    const uint8_t set_protocol[] = { 0x00, 0x18 };

    if(inSetProtocol(dev, set_protocol, sizeof(set_protocol))){
        return 1;
    }
    
    uint8_t acard = 0;
    const uint8_t param[] = { 0x6e, 0x00, 0x06, 0x00, 0xff, 0xff, 0x01, 0x00 };
    while(1){
        int codelen = 25; // 18 + 7
        if(inCommRF(dev, param, sizeof(param), codebytes, &codelen)){
            return 1;
        }
        int i;
        if(codelen==25&&codebytes[0]==0&&codebytes[1]==0
                      &&codebytes[2]==0&&codebytes[3]==0){
            if(acard==0){
                for(i=7;i<9;i++) OledPrint("%02x", codebytes[i]);
                PrintOnDisplay(" ********");
                for(i=13;i<15;i++) OledPrint("%02x", codebytes[i]);
                PrintOnDisplay("\n");
                
                if(inSetRF(dev, 1, 1, 15, 1)){ // 212F
                    break;
                }
                
                if(inSetProtocol(dev, set_protocol_default, sizeof(set_protocol_default))){
                    break;
                }
                
                // RC-S380 is Type3Tag ?
                // imitates github.com/nfcpy/src/nfc/tag/tt3.py#read_without_encryption()
                
                // TODO : process logically these magic numbers
                codelen = 34;
                uint8_t woenc[] = { 0x54, 0x01 ,       // timeout value
                    0x10 ,                             // length
                    0x06 ,                             // command?
                    0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 , // idm
                    0x01 ,0x0f ,0x09 ,0x01 ,0x80 ,0x00 // service code/block code
                };
                for(i=0;i<8;i++) woenc[4+i] = codebytes[7+i];
                
                inCommRF(dev, woenc, sizeof(woenc), codebytes, &codelen);
                if(codelen == 34){
                    PrintOnDisplay("%d Yen\n", (((uint16_t) codebytes[29])<<8) + codebytes[28]);
                }else{
                    PrintOnDisplay("No data or Error\n");
                }
            }
            acard = 1;
        }else{
            acard = 0;
        }
        
        __delay_ms(1000);
    }
    
    if(switchRF(dev, 0)){
        return 1;
    }

    return 0;

}

/* end of file*/
