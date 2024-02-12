#ifndef MYLIB_I2C_H
#define MYLIB_I2C_H

#include <stdint.h>

#define I2C_TO_CNT 255 // Time Out CouNT
#define I2C_RT_MAX 1  // ReTry MAX

// mylib/i2c.c
uint8_t Call_I2C1_MasterWrite(uint8_t device_address, uint8_t* msg, uint8_t msg_len);
uint8_t Call_I2C1_MasterWriteRead(uint8_t device_address, uint8_t* msg, uint8_t msg_len, uint8_t* recv, uint8_t recv_len);

// aqm0802a.c
void LcdCommand(uint8_t ctl, uint8_t data);
void LcdPrint(unsigned char line, char *p);
void LcdInit();

// ssd1306.c
void OledInit(uint8_t rot);
void OledPutPrint(uint8_t cx, uint8_t cy, const char*, ...);

void OledPrint(const char *, ...);
void OledClear();
void OledShutdown();

// 24lc64.c
uint8_t write_24lc64(uint16_t address, uint8_t* data, uint8_t count);
uint8_t read_24lc64(uint16_t address, uint8_t* data, uint16_t count);

// mlx90614.c
double readMLX90614();

// lm75b.c
double readLM75B();

#endif // MYLIB_I2C_H
