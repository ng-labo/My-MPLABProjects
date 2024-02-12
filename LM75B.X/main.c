#define _XTAL_FREQ 16000000

#include <stdio.h>
#include <pic.h>
#include <htc.h>
#include <pic16f18326.h>

#pragma config FEXTOSC = OFF
#pragma config CLKOUTEN = OFF
//#pragma config RSTOSC = HFINT1 // 1MHz
#pragma CSWEN = ON             // to change system clock by NOSC NDIV in OCCON1
#pragma FCMEN = OFF
#pragma config MCLRE = OFF,PWRTE = OFF,WDTE = OFF,LPBOREN = OFF
#pragma config BOREN = OFF,BORV = LOW,PPS1WAY = OFF,STVREN = OFF
#pragma config DEBUG = OFF
#pragma config WRT = OFF,CP = OFF,CPD = OFF
//#pragma config LVP = OFF

//#define MYDBG

void flush_ledc2(int c) {
    for(int i=0;i<c;i++){
        LATCbits.LATC2 = 1; // LED
        __delay_ms(500);        
        LATCbits.LATC2 = 0;
        __delay_ms(500);        
    }
}

#define I2C_WRITE_MODE 0x00
#define I2C_READ_MODE 0x01

//#define ENABLE_LCD
//#define ENABLE_LM75B
//#define ENABLE_24LC64
#define ENABLE_BME280

inline void i2c_idle(void) {
    while(SSP1CON2bits.SEN   |
          SSP1CON2bits.PEN   |
          SSP1CON2bits.RCEN  |
          SSP1CON2bits.ACKEN |
          SSP1STATbits.R_nW);
}

inline void i2c_start(void) {
    i2c_idle();
    SSP1IF = 0;
    SSP1CON2bits.SEN = 1;
    while(SSP1IF==0);
    SSP1IF = 0;
}

inline void i2c_stop(void) {
    SSP1IF = 0;
    SSP1CON2bits.PEN = 1;
    while(SSP1IF==0);
    SSP1IF = 0;
    i2c_idle();
}

inline void i2c_send(unsigned char data){
    SSP1IF = 0;
    SSP1BUF = data;
    while(SSP1IF==0);
    SSP1IF = 0;
    i2c_idle();
}

#if defined(ENABLE_24LC64) || defined(ENABLE_BME280)
// i2c_Restart - Re-Start I2C communication
inline void i2c_restart(void){
    SSP1IF = 0;
    SSP1CON2bits.RSEN=1;
    while(SSP1IF==0);
    SSP1IF = 0;
}
#endif // ENABLE_24LC64
#if defined(ENABLE_24LC64) || defined(ENABLE_LM75B) || defined(ENABLE_BME280)
inline unsigned char i2c_recv(void){
    SSP1CON2bits.RCEN = 1;
    while(SSP1CON2bits.RCEN);
    return(SSP1BUF);
}
// 0:ack 1:nack
inline void i2c_send_ack(unsigned char b){
    SSP1CON2bits.ACKDT = b;
    SSP1CON2bits.ACKEN = 1;
    while(SSP1CON2bits.ACKEN);
}
#endif // ENABLE_24LC64 or ENABLE_LM75

#ifdef ENABLE_LM75B
#define I2C_ADDRESS_LM75B 0x48

void initLM75B() {
    i2c_start();
    i2c_send(I2C_ADDRESS_LM75B<<1|I2C_WRITE_MODE);
    i2c_send(0);
    i2c_stop();
}
    
float readLM75B() {
    unsigned char r1, r2;
    i2c_start();
    i2c_send((I2C_ADDRESS_LM75B<<1)|I2C_READ_MODE);
    r1 = i2c_recv();
    i2c_send_ack(0);
    r2 = i2c_recv();
    i2c_send_ack(1);
    i2c_stop();
    
    return (float) (((r1<<8) + r2)>>5) / 8.0f;
}
#endif // ENABLE_LM75B

#ifdef ENABLE_LCD

#define I2C_ADDRESS_LCD0802 0x3e
#define MAXDIGIT 8

void LCDcommand(unsigned char c) {
     i2c_start();
     i2c_send(I2C_ADDRESS_LCD0802<<1|I2C_WRITE_MODE);
     i2c_send(0x80);
     i2c_send(c);
     i2c_stop();
     __delay_us(27);
}

void LCDinit() {
     LCDcommand(0x38);
     LCDcommand(0x39);
     LCDcommand(0x14);
     LCDcommand(0x70);
     LCDcommand(0x56);
     LCDcommand(0x6c);
     __delay_ms(200);
     LCDcommand(0x38);
     LCDcommand(0x0c);
     LCDcommand(0x01);
     __delay_ms(1);
     LCDcommand(0x80);
     LCDcommand(0x01);
}

void LCDprint(const char *p) {
     i2c_start();
     i2c_send(I2C_ADDRESS_LCD0802<<1|I2C_WRITE_MODE);
     i2c_send(0x40);
     int i=0;
     while(*p){
         i2c_send(*p++);
         if(++i==MAXDIGIT) break; 
     }
     i2c_stop();
     __delay_us(27);
}

void LCDcursor(unsigned char col, int row) {
     LCDcommand(0x80 | (col + (row==1 ? 0x40 : 0x00)));
}

#endif // ENABLE_LCD

#ifdef ENABLE_24LC64
#define I2C_ADDRESS_24LC64 0x50

unsigned char write_24lc64(unsigned int address, unsigned char data){
    i2c_start();
    i2c_send(I2C_ADDRESS_24LC64<<1|WRITE_MODE);
    i2c_send(address >> 8);
    i2c_send(address & 0x00FF);
    i2c_send(data);
    i2c_stop();
    __delay_ms(15);
}

unsigned char read_24lc64(unsigned int address){
    unsigned char data;
    i2c_start();
    i2c_send(I2C_ADDRESS_24LC64<<1|WRITE_MODE);
    i2c_send(address >> 8);
    i2c_send(address & 0x00FF);
    i2c_restart();
    i2c_send(I2C_ADDRESS_24LC64<<1|READ_MODE);
    data = i2c_recv();
    i2c_send_ack(1);
    i2c_stop();
    return data; 
}

#endif

#ifdef ENABLE_BME280
#define I2C_BME280 0x76

uint16_t getUShort(uint8_t* data, uint16_t index){
    return (((uint16_t) data[1+index])<<8) + (uint16_t) data[index];
}

int16_t getShort(uint8_t* data, uint16_t index){
    return (int16_t) (((uint16_t) data[1+index])<<8) + (uint16_t) data[index];
}

void read_bme280(uint8_t address, uint8_t* data, uint16_t count) {
    i2c_start();
    i2c_send(I2C_BME280<<1|I2C_WRITE_MODE);
    i2c_send(address);
    i2c_restart();
    i2c_send(I2C_BME280<<1|I2C_READ_MODE);
    for(uint16_t i=0;i<count;i++){
        *data++ = i2c_recv();
        i2c_send_ack((i==count-1) ? 1 : 0);
    }
    i2c_stop();
    return; 
}

uint8_t write_bme280(uint8_t address, uint8_t data) {
    i2c_start();
    i2c_send(I2C_BME280<<1|I2C_WRITE_MODE);
    i2c_send(address);
    i2c_send(data);
    i2c_stop();
    __delay_ms(15);
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
    return pvar / 100.0f; // return as hPa
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

void call_bme280(float* temperature, float* pressure, float* humidity) {
    uint8_t cal1[24];
    uint8_t cal2[1];
    uint8_t cal3[7];

    write_bme280(0xf2, 2);
    __delay_ms(5);
    write_bme280(0xf4, (2<<5)|(2<<2)|1);
    __delay_ms(5);
    
    read_bme280(0x88, cal1, 24);
    read_bme280(0xa1, cal2, 1);
    read_bme280(0xe1, cal3, 7);
    
    //__delay_ms(17); # 16.2msec in python code
    __delay_ms(50);

    uint8_t data[8];
    read_bme280(0xf7, data, 8);
        
    float* t_fine;
    *temperature = calc_temperature(data, cal1, t_fine);
    *pressure = calc_pressure(data, cal1, t_fine);
    *humidity = calc_humidity(data, cal2, cal3, t_fine);

}

#endif // I2C_BME280


volatile unsigned int tocnt = 0;
unsigned char RxBuf[10];
unsigned int RxIdx = 0;

void putch(unsigned char ch) {
    tocnt = 0;
    while (!TXIF) {
        if(10<tocnt)return;
    }
    TX1REG = ch;
}

void reset_RxBuffer(){
    RxIdx = 0;
    RxBuf[0] = '\0';
}

unsigned int tic_sec = 0;
#define FACTOR1SEC 16
int INTR_CNT = FACTOR1SEC;
void __interrupt () process(void) {
    if (PIR1bits.TMR2IF) {
        INTR_CNT--;
        if(INTR_CNT==0){
            INTR_CNT = FACTOR1SEC;
            tocnt++;
            tic_sec++;
        }
        PIR1bits.TMR2IF = 0;
    }
    if (PIR1bits.RCIF) {
        if(RxIdx==9)RxIdx=0;
        unsigned char rx = RC1REG;
        if(rx!='\r'&&rx!='\n'){
            RxBuf[RxIdx++] = rx;
            RxBuf[RxIdx]='\0';
        }
    }
}

#define BOARD_PS 3.3f                      // 3.3 volt supply
#define SPLIT_VOLTAGE (10000.0f/20000.0f) // rate of split voltage
#define ADC_MAX 1024.0f                    // ad convert bits
#define VSC_LSB 0.0064453125f // BOARD_PS / ADC_MAX / SPLIT_VOLTAGE
                               // 3.3 / 1024 / 10000 * 20000
float process_adc() {
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);
    unsigned int value = ADRES;
    
    float voltage = (float) value * VSC_LSB;
    
    return voltage;
}

#define AT_HELLO    "AT\r\n"
#define AT_REG_LINK "AT+CIPSTART=\"UDP\",\"192.168.1.100\",2999\r\n"
#define AT_REQ_DEEPSLEEP "AT+GSLP=0\r\n"
    
void reset_esp_wroom() {
    for(int i=0;i<2;i++){
        LATCbits.LATC2 = 1; // LED
        
        LATCbits.LATC1 = 0;
        __delay_ms(300);
        LATCbits.LATC1 = 1;

        __delay_ms(3700);
        LATCbits.LATC2 = 0; // LED

        reset_RxBuffer();
        printf(AT_HELLO);
        __delay_ms(2000);
        
        //LCDcursor(0, 1);
        //LCDprint(RxBuf);
        // getting "ATOK"
        if(RxBuf[2]=='O'&&RxBuf[3]=='K'){
            break;
        }
    }
    
    printf(AT_REG_LINK);
    __delay_ms(100);
}

void notify_to_esp_wroom(int i, unsigned int temperature, unsigned int pressure, unsigned int humidity, unsigned int voltage){
    char at_msg[32];
    char at_cmd[20];
    int len_msg = sprintf(at_msg, "%d %u %u %u %u\r\n", i, temperature, pressure, humidity, voltage);
    sprintf(at_cmd, "AT+CIPSEND=%d\r\n", len_msg);

    printf(at_cmd);
    __delay_ms(50);
    printf(at_msg);
    __delay_ms(50);
}

void sleep_esp_wroom() {
    printf(AT_REQ_DEEPSLEEP);
}

/* PIC16F18326
 * VDD 1 14 VSS
 * RA5 2 13 RA0
 * RA4 3 12 RA1
 * RA3 4 11 RA2
 * RC5 5 10 RC0
 * RC4 6  9 RC1
 * RC3 7  8 RC2
 */
int main() {
    // REGISTER 7-1, 7-6
    OSCCON1bits.NDIV = 0b0000;   // Clock Divider 1
    OSCCON1bits.NOSC = 0b000;    // 2x PLL
    OSCFRQbits.HFFRQ = 0b0100;   // 16MHz
    
    INLVLC  = 0b00000000;
    ANSELA  = 0b00000000;
    ANSELC  = 0b00001000;
    TRISA   = 0b00110000;  // RA4(SCL1),RA5(SDA1)
    TRISC   = 0b00101000;  // RC4(TX),  RC5(RX),  RC3(ADC)
    WPUA    = 0b00110000;  // weak up 
    WPUC    = 0b00000000;  // weak up 
    PORTA   = 0b00000000;
    PORTC   = 0b00000000;
    LATA    = 0b00000000;
    LATC    = 0b00000000;
    
    SSP1CLKPPS = 0x04;  // RA4 - input dst = SCL1
    SSP1DATPPS = 0x05;  // RA5 - input dst = SDA1
    RA4PPS     = 0x18;  // RA4 - output src = SCL1
    RA5PPS     = 0x19;  // RA5 - output src = SDA1

    // I2C configuration
    SSP1STAT = 0b10000000;
    SSP1CON1 = 0b00101000; // [7] WCOL   = 0    : write colision
                           // [6] SSPOV  = 0    : no overflow
                           // [5] SSPEN  = 1    : SDA, SCL enable
                           // [4] CKP    = 0    : clock parameter
                           // [3:0] SSPM = 1000 : I2C master mode 
                           //                     clock=FOSC/(4*(SSP1ADD+1))
                           //                           16MHz/(4*(9+1))=400KHz
    SSP1CON3 = 0x00;
    SSP1ADD  = 0x09;
    INTCON   = 0x00;  // interrupt disable
    
    // UART configuration
    // TX : RC4
    // RX : RC5
    RC4PPS = 0b00010100;        // RC4: TX
    RXPPS  = 0b00010101;        // RC5: RX
    TX1STA   = 0b00100100;      // TX-enable, high-speed(REGISTER 31-1)
    RC1STA   = 0b10010000;      // Serial-port enable(REGISTER 31-2))
    BAUD1CON = 0b00001000;      // 16-bit Baud Rate Generator is used
    //SPBRG = 8;//baud 9600~103 , 115000~8 in SYNC=1,BRG16=1
    SPBRG = 34;// 115000  in SYNC=1,BRG16=1
               // 16000000/115200/4-1 = 33.72222222222222

    // AD configuration
    //ADCON0bits.ADON = 1;
    ADCON0 = 0b01001101; // RC3
    ADCON1 = 0b11000000; // out on right
                         // ADC CLOCK ; TABLE 22-2: 0b100 Focs/4 1.0uSec in 4MHz
                         // enable ADC    
    
    // TIMER2 configuration
    T2CONbits.T2OUTPS = 0b1111; // Timer2 postscaler 1:16
    T2CONbits.T2CKPS = 0b11;    // Timer2 prescaler 1:64
    // Timer Fq : FOSC / 4 / post-scale / pre-scale
    // 16000000 / 4 / 64 / 16 = 3906.25 Hz
    // 3906.25 / 16 = 244.140625 < 255
    PR2 = 244 - 1; // Set comparator
    T2CONbits.TMR2ON = 1; // Timer2 enable
    PIE1bits.TMR2IE = 1;  // Timer2 interrupt enable
    PIR1bits.TMR2IF = 0;  // Clear timer2 interrupt flag
    
    PIR1bits.RCIF = 0;
    PIE1bits.RCIE = 1;
    
    INTCONbits.PEIE = 1;  // Peripheral interrupt enable
    INTCONbits.GIE = 1;   // Global interrupt enable

#ifdef ENABLE_LCD
    __delay_ms(500);
    LCDinit();
    __delay_ms(500);
#endif // ENABLE_LCD
    
#ifdef ENABLE_LCD
    LCDcursor(0, 0); LCDprint("BME280");
#endif // ENABLE_LCD
    reset_esp_wroom();
#ifdef ENABLE_LCD
    LCDcursor(0, 0); LCDprint("      ");
#endif // ENABLE_LCD
        

    float temperature;
    float pressure;
    float humidity;
    float source_voltage;
    
    call_bme280(&temperature, &pressure, &humidity);
    flush_ledc2(3);
    __delay_ms(1000);
    source_voltage = process_adc();
    notify_to_esp_wroom(0, (unsigned int) (temperature * 100.0f),
                           (unsigned int) (pressure * 10.0f),
                           (unsigned int) (humidity * 100.0f),
                           (unsigned int) (source_voltage * 100.0f));    
    //notify_to_esp_wroom(0, (unsigned int)(readLM75B()*10.0f), (unsigned int)(process_adc()*100.0f));    
    __delay_ms(1000);
    flush_ledc2(2);
    sleep_esp_wroom();
    __delay_ms(1000);

    flush_ledc2(4);

#ifdef MYDBG
#define RECORD_PERIOD_SEC 30
#define MAX_RECORD_INDEX 2
#else
#define RECORD_PERIOD_SEC 45
#define MAX_RECORD_INDEX 40
#endif
    unsigned int record_temperature[MAX_RECORD_INDEX];
    unsigned int record_voltage[MAX_RECORD_INDEX];
    unsigned int record_pressure[MAX_RECORD_INDEX];
    unsigned int record_humidity[MAX_RECORD_INDEX];
    
    unsigned char record_index = 0;
    unsigned int last_tic_sec = 0;
    tic_sec = 0;
    while(1){
        while(last_tic_sec==tic_sec);
        last_tic_sec = tic_sec;
        
        if((tic_sec%5)!=0)continue;
        
        LATCbits.LATC2 = 1; // LED
        //float temperature = readLM75B();
       
#ifdef ENABLE_LCD        
        char lcdbuf[24];
        sprintf(lcdbuf, "%4.1f", temperature);
        LCDcursor(0, 0); LCDprint(lcdbuf);
#endif // ENABLE_LCD
        
        if(tic_sec>=RECORD_PERIOD_SEC){
            tic_sec = 0;
            last_tic_sec = tic_sec;
            
            call_bme280(&temperature, &pressure, &humidity);
            source_voltage = process_adc();

            record_temperature[record_index] = (unsigned int) (temperature * 100.0f);
            record_pressure[record_index] = (unsigned int) (pressure * 10.0f);
            record_humidity[record_index] = (unsigned int) (humidity * 100.0f);
            record_voltage[record_index] = (unsigned int) (source_voltage * 100.0f);
            record_index++;
            if(record_index == MAX_RECORD_INDEX){
                record_index = 0;
                
                reset_esp_wroom();
                
                for(int i=0;i<MAX_RECORD_INDEX;i++){
                    notify_to_esp_wroom(MAX_RECORD_INDEX-i-1, record_temperature[i],  record_pressure[i],  record_humidity[i], record_voltage[i]);
                }
                
                sleep_esp_wroom();
            }
        }else{
            __delay_ms(50);
        }
        
        LATCbits.LATC2 = 0; // LED
        
    }

    return 0;
}
