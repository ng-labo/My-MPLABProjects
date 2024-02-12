#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/i2c1.h"

#define I2C_AQM0802A 0x3E
#ifdef I2C_AQM0802A

// #define AQM0802A_DIGIT 8
#define AQM0802A_DIGIT 16

void LcdCommand(uint8_t ctl, uint8_t data) {
    uint8_t cmd[2];
    cmd[0] = ctl;
    cmd[1] = data;
    Call_I2C1_MasterWrite(I2C_AQM0802A, cmd, 2);
}

void LcdPrint(unsigned char line, char *p)  {
    LcdCommand(0x00, 0x80 | (line==0 ? 0x00 : 0x40));
    
    uint8_t msg[1+AQM0802A_DIGIT+1];

    msg[0] = 0x40; // command for AQM
    int idx = 1;
    while(*p!=0&&idx<=AQM0802A_DIGIT+1){
        msg[idx++] = (uint8_t) *p++;
    }

    Call_I2C1_MasterWrite(I2C_AQM0802A, msg, idx);
}

void LcdInit() {
    uint8_t initcmd[] = { 0x38, 0x39, 0x14, 0x70, 0x56, 0x6C, 0x38, 0x0C, 0x01};
    __delay_ms(40);
    Call_I2C1_MasterWrite(I2C_AQM0802A, initcmd, sizeof(initcmd));
    __delay_ms(1);
}
#endif // I2C_AQM0802A

