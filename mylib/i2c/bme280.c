#include "defines.h"
#include "i2c_local.h"
#include "mylib/i2c.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/i2c1.h"

#define I2C_BME280 0x76
#ifdef I2C_BME280
uint16_t getUShort(uint8_t* data, uint16_t index){
    return (((uint16_t) data[1+index])<<8) + (uint16_t) data[index];
}

int16_t getShort(uint8_t* data, uint16_t index){
    return (int16_t) (((uint16_t) data[1+index])<<8) + (uint16_t) data[index];
}

uint8_t read_bme280(uint8_t address, uint8_t* data, uint16_t count) {
    uint8_t cmd[1];
    cmd[0] = address;
    Call_I2C1_MasterWrite(I2C_BME280, cmd, 1);
    
    I2C1_MESSAGE_STATUS status = I2C1_MESSAGE_PENDING;
    I2C1_MasterRead(data, count, I2C_BME280, &status);
    uint8_t c = I2C_TO_CNT;
    while (status == I2C1_MESSAGE_PENDING && c--) __delay_us(100);
    return (status == I2C1_MESSAGE_COMPLETE);    
}

uint8_t write_bme280(uint8_t address, uint8_t data) {
    uint8_t cmd[2];
    cmd[0] = address;
    cmd[1] = data;
    return Call_I2C1_MasterWrite(I2C_BME280, cmd, 2);
}

float calc_temperature(uint8_t* data, uint8_t* cal1, float* t_fine) {
    uint32_t temp_raw = (((uint32_t) data[3]) << 12) + (((uint32_t) data[4]) << 4) + (((uint32_t) data[5]) >> 4);    

    uint16_t dig_T1 = getUShort(cal1, 0);
    int16_t dig_T2 = getShort(cal1, 2);
    int16_t dig_T3 = getShort(cal1, 4);
    
    int32_t var1 = (((temp_raw>>3) - (dig_T1<<1)) * dig_T2) >> 11;
    int32_t var2 = (temp_raw>>4) - dig_T1;
    var2 *= var2;
    var2 >>=12;
    var2 *= dig_T3;
    var2 >>=14;
    *t_fine = (float) var1 + (float) var2;
    return (*t_fine * 5.0f + 128.0f ) / 256.0f / 100.0f;
}

float calc_pressure(uint8_t* data, uint8_t* cal1, float* t_fine) {    
    uint32_t pres_raw = (((uint32_t) data[0]) << 12) + (((uint32_t) data[1]) << 4) + (((uint32_t) data[2]) >> 4);

    uint16_t dig_P1 = getUShort(cal1, 6);
    int16_t dig_P2 = getShort(cal1, 8);
    int16_t dig_P3 = getShort(cal1, 10);
    int16_t dig_P4 = getShort(cal1, 12);
    int16_t dig_P5 = getShort(cal1, 14);
    int16_t dig_P6 = getShort(cal1, 16);
    int16_t dig_P7 = getShort(cal1, 18);
    int16_t dig_P8 = getShort(cal1, 20);
    int16_t dig_P9 = getShort(cal1, 22);

    float fvar1 = *t_fine / 2.0f - 64000.0f;
    float fvar2 = fvar1 * fvar1 * (float) dig_P6 / 32768.0f;
    fvar2 += (fvar1 * (float) dig_P5 * 2.0f);
    fvar2 = (fvar2 / 4.0f) + (float) dig_P4 * 65536.0f;
    fvar1 = (dig_P3 * fvar1 / 524288.0f + dig_P2) * fvar1 / 524288.0f;
    fvar1 = (1.0 + fvar1 / 32768.0f) * (float) dig_P1;
    if(fvar1==0.0f){
        return 0.0f;
    }
    float pvar = 1048576.0f - pres_raw;
    pvar = ((pvar - fvar2 / 4096.0f) * 6250.0f) / fvar1;
    fvar1 = (float) dig_P9 * pvar * pvar / 2147483648.0f;
    fvar2 = (float) dig_P8 * pvar / 32768.0f;
    pvar += (fvar1 + fvar2 + (float) dig_P7) / 16.0f;
    return pvar / 100.0f;
}

float calc_humidity(uint8_t* data, uint8_t* cal2, uint8_t* cal3, float* t_fine) {
    uint32_t hum_raw = (((uint32_t) data[6]) << 8) +  data[7];

    uint8_t dig_H1 = cal2[0];
    int16_t dig_H2 = getShort(cal3, 0);
    uint8_t dig_H3 = cal3[2];

    int16_t dig_H4 = (((int16_t) cal3[3]) << 4) | (cal3[4] & 0x0F);
    int16_t dig_H5 = (((int16_t) cal3[5]) << 4) | (cal3[4]>>4);
    int16_t dig_H6 = (int16_t) cal3[6];
    
    float hvar = *t_fine - 76800.0f;
    hvar = (hum_raw - ((float) dig_H4 * 64.0f + (float) dig_H5 / 16384.0f * hvar)) * ((float) dig_H2 / 65536.0 * (1.0f + dig_H6 / 67108864.0f * hvar * (1.0f + (float) dig_H3 / 67108864.0f * hvar)));
    hvar *= (1.0 - (float) dig_H1 * hvar / 524288.0);
    if (hvar > 100) {
        hvar = 100.0f;
    } else if(hvar < 0){
        hvar = 0.0f;
    }
    return hvar;
 }

void call_bme280(float* temperature, float* humidity, float* pressure) {
    uint8_t cal1[24];
    uint8_t cal2[1];
    uint8_t cal3[7];

    write_bme280(0xf2, 2);
    write_bme280(0xf4, (2<<5)|(2<<2)|1);
    
    read_bme280(0x88, cal1, 24);
    read_bme280(0xa1, cal2, 1);
    read_bme280(0xe1, cal3, 7);
    
    __delay_ms(17);

    uint8_t data[8];
    read_bme280(0xf7, data, 8);
        
    float* t_fine;
    *temperature = calc_temperature(data, cal1, t_fine);
    *pressure = calc_pressure(data, cal1, t_fine);
    *humidity = calc_humidity(data, cal2, cal3, t_fine);

}
#endif // I2C_BME280
