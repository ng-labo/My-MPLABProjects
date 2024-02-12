#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/i2c1.h"

#include "mylib/i2c.h"

/*
 * LM75B
 * I2C Clock : 100Hz
 */

#define I2C_LM75B (0x48)

double readLM75B() {
    uint8_t data[2];
    data[0]=0;
    data[1]=0;
    
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    uint8_t c = I2C_TO_CNT;    
    I2C1_MasterRead(data, 2, I2C_LM75B, &status);
    while (status == I2C1_MESSAGE_PENDING && c--) __delay_us(100);
    
    return (double) (((data[0]<<8) + data[1])>>5) / 8.0f;
}
