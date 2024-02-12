#include "defines.h"
#include "i2c_local.h"
#include "i2c.h"
#include "mcc_generated_files/i2c1.h"

//#define TEMP_DBG
#ifdef TEMP_DBG
#include "mcc_generated_files/uart1.h"
#endif
/*
 I2C routine
 */

uint8_t Call_I2C1_MasterWrite(uint8_t device_address, uint8_t* msg, uint8_t msg_len) {
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    I2C1_MasterWrite(msg, msg_len, device_address, &status);
    uint8_t c = I2C_TO_CNT;
    while (status == I2C1_MESSAGE_PENDING && c--) __delay_us(100);
    
#ifdef TEMP_DBG
    printf("status=%d\n", status);
#endif
    return (status != I2C1_MESSAGE_COMPLETE);
}

uint8_t Call_I2C1_MasterWriteRead(uint8_t device_address, uint8_t* msg, uint8_t msg_len, uint8_t* recv, uint8_t recv_len) {
    I2C1_MESSAGE_STATUS status;
    static I2C1_TRANSACTION_REQUEST_BLOCK   trblock[2];

    I2C1_MasterWriteTRBBuild(&trblock[0], msg,  msg_len, device_address);
    I2C1_MasterReadTRBBuild(&trblock[1], recv, recv_len, device_address);

    I2C1_MasterTRBInsert(2, trblock, &status);
    
    uint8_t c = I2C_TO_CNT;
    while ((status == I2C1_MESSAGE_PENDING) && (c--)) __delay_ms(100);
    
    return (status != I2C1_MESSAGE_COMPLETE);
}