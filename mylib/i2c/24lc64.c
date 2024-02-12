#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/i2c1.h"

#define I2C_24LC64 0x50

#ifdef I2C_24LC64
#define MAX_LEN_WRITE_TO_24LC64 256

static uint8_t cmd[2 + MAX_LEN_WRITE_TO_24LC64];

uint8_t write_24lc64(uint16_t address, uint8_t* data, uint8_t count){
    cmd[0] = address >> 8;
    cmd[1] = (uint8_t) (address & 0x00FF);
    uint8_t i = 0;
    for(;i<count;i++, data++) cmd[2+i] = *data;
    uint8_t ret = Call_I2C1_MasterWrite(I2C_24LC64, cmd, 2 + count);
    __delay_ms(10);
    return ret;
}

uint8_t read_24lc64(uint16_t address, uint8_t* data, uint16_t count) {
    cmd[0] = address >> 8;
    cmd[1] = (uint8_t) (address & 0x00FF);
    
    return Call_I2C1_MasterWriteRead(I2C_24LC64, cmd, 2, data, count);
}

#endif //

