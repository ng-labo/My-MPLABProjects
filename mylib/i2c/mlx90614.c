#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/i2c1.h"

#include "mylib/i2c.h"

/*
 * MLX90614
 * I2C Clock : 100Hz
 */

#define I2C_MLX90614 (0x5a)

double readMLX90614() {
    uint16_t r;
    
    uint8_t cmd[1];
    // cmd[0]= 0x06; // _MLX90614_TA
    cmd[0]= 0x07; // _MLX90614_TOBJ1
    uint8_t data[2];
    data[0]=0;
    data[1]=0;

    Call_I2C1_MasterWriteRead(I2C_MLX90614, cmd, 1, data, 2);
    r = ((uint16_t) data[1]<<8) | data[0];
    
    return (double) r * 0.02 - 273.15;
}
