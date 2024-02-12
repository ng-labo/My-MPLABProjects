#include <pic.h>
#include <htc.h>
#include <pic16f18326.h>

#pragma config FEXTOSC = OFF
#pragma config CLKOUTEN = OFF
#pragma config RSTOSC = HFINT1  // 1MHz
#pragma config CSWEN = ON       // to change system clock by NOSC NDIV in OCCON1
#pragma config FCMEN = OFF
#pragma config MCLRE = OFF, PWRTE = OFF, WDTE = OFF, LPBOREN = OFF
#pragma config BOREN = OFF, BORV = LOW, PPS1WAY = OFF, STVREN = OFF
#pragma config DEBUG = OFF
#pragma config WRT = OFF, CP = OFF, CPD = OFF
//#pragma config LVP = OFF

#define _XTAL_FREQ 10000000

#define LED1_ON()       (LATCbits.LATC5 = 1)
#define LED1_OFF()      (LATCbits.LATC5 = 0)
#define LED1_FLIP()     (LATCbits.LATC5 = ~LATCbits.LATC5)
#define SELECT_SW1()    (PORTCbits.RC3==1)

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

// i2c_Restart - Re-Start I2C communication
void i2c_restart(void){
    SSP1IF = 0;
    SSP1CON2bits.RSEN=1;
    while(SSP1IF==0);
    SSP1IF = 0;
}

inline void i2c_send(unsigned char data){
    SSP1IF = 0;
    SSP1BUF = data;
    while(SSP1IF==0);
    SSP1IF = 0;
    i2c_idle();
}

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

#define I2C_ADDRESS_LCD0802 (0x3e<<1)
#define I2C_ADDRESS_INA219 (0x40<<1)

#define MAXDIGIT 8

void LCDcommand(unsigned char c) {
     i2c_start();
     i2c_send(I2C_ADDRESS_LCD0802);
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
     i2c_send(I2C_ADDRESS_LCD0802);
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

//#define INA219_16V_400mA
//#define INA219_16V_800mA
//#define INA219_16V_1600mA
#define INA219_32V_800mA

#ifdef INA219_16V_400mA
#define INA219CONF 1
#define CURRENT_LSB   0.01220f  // 1.220e-05f * 1000.0f
#endif
#ifdef INA219_16V_800mA
#define INA219CONF 2
#define CURRENT_LSB   0.02439f  // 2.439e-05f * 1000.0f
#endif
#ifdef INA219_16V_1600mA
#define INA219CONF 3
#define CURRENT_LSB   0.04878f  // 4.878e-05f * 1000.0f
#endif
#ifdef INA219_32V_800mA
#define INA219CONF 6
#define CURRENT_LSB   0.02439f  // 2.439e-05f * 1000.0f
#endif
void initINA219() {
    i2c_start();
    i2c_send(I2C_ADDRESS_INA219);
    i2c_send(0x05);
#ifdef INA219_16V_400mA
    i2c_send(0x83);
    i2c_send(0x33);
#endif
#if defined(INA219_16V_800mA) || defined(INA219_32V_800mA)
    i2c_send(0x41);
    i2c_send(0x99);
#endif
#ifdef INA219_16V_1600mA
    i2c_send(0x20);
    i2c_send(0xcc);
#endif
    i2c_stop();
    __delay_ms(50);
    
    i2c_start();
    i2c_send(I2C_ADDRESS_INA219);
    i2c_send(0x00);
#ifdef INA219_16V_400mA
    i2c_send(0x21);
#endif
#ifdef INA219_16V_800mA
    i2c_send(0x09);
#endif
#ifdef INA219_32V_800mA
    i2c_send(0x29);
#endif
#ifdef INA219_16V_1600mA
    i2c_send(0x31);
#endif
    i2c_send(0x9f);
    i2c_stop();
    __delay_ms(50);
}
 
unsigned int read_register(unsigned char addr) {
    unsigned int ret;
    i2c_start();
    i2c_send(I2C_ADDRESS_INA219);
    i2c_send(addr);
    i2c_restart();
    i2c_send(I2C_ADDRESS_INA219|1);
    ret = i2c_recv() * 256;
    i2c_send_ack(0);
    ret |= i2c_recv();
    i2c_send_ack(1);
    i2c_stop();
    
    return ret;
}
void display_value(const int num, const char l1, const char l2) {
    char buf[16];
    char d = (char) (num / 10000);
    char z = d;
    buf[0] = (d>0) ? '0' + d : ' ';
    d = (num%10000)/1000;
    buf[1] = (d>0) ? '0' + d : ((z==0) ? ' ' : '0');
    z = z>0 ? 1 : d;
    d = (num%1000)/100;
    buf[2] = (d>0) ? '0' + d : ((z==0) ? ' ' : '0');
    d = (num%100)/10;
    buf[3] = '0' + d;
    buf[4] = '.';
    buf[5] = '0' + (num%10);
    buf[6] = l1;
    buf[7] = l2;
    LCDcursor(0, 0); LCDprint(buf);    
}

volatile unsigned int debug_val;
void process_adc() {
    ADCON0 = 0b00001001; // RA2
    __delay_us(20);
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);
    unsigned int value = ADRES;

    display_value(value * 10, '-', '-');
}

void process_ina219() {
    //unsigned int voltage = read_register(0x02); // voltage
    unsigned int current = read_register(0x04); // current

    float current_mA = (float) current;
    if (current_mA>32768.0) current_mA -= 65536.0;
    current_mA *= CURRENT_LSB;

    int num = (int) (current_mA * 10);
    if(num<0) num = 0;

    display_value(num, 'm', 'A');
        
}

#ifdef HAS_TIMER2
#define FACTOR1SEC 32
int INTR_CNT = FACTOR1SEC;
volatile unsigned int clk = 0;
void __interrupt () periodic_process(void) {
    if (PIR1bits.TMR2IF) {
        INTR_CNT--;
        if(INTR_CNT==0){
            INTR_CNT = FACTOR1SEC;
            //LED1_FLIP();
            clk++;
        }
        PIR1bits.TMR2IF = 0;
    }
}
#endif

int main() {
    // OSCILLATOR configuration
    // REGISTER 7-1, 7-6
    OSCCON1bits.NDIV = 0b0000;   // Clock Divider 1
    OSCCON1bits.NOSC = 0b000;
    OSCFRQbits.HFFRQ = 0b0011;   // 4MHz
    
    ANSELA  = 0b00000100;  // RA2 - ADC
    ANSELC  = 0b00000000;
    TRISA   = 0b00000000;
    TRISC   = 0b00001011;  // RC5 - LED RC3 - SW RC0/RC1 - I2C
    WPUA    = 0b00000000;  //
    WPUC    = 0b00001011;  // weak up
    PORTA   = 0b00000000;
    PORTC   = 0b00000000;
    
    // I2C configuration
    SSP1CLKPPS = 0x10;  // RC0 - input dst = SCL1
    SSP1DATPPS = 0x11;  // RC1 - input dst = SDA1
    RC0PPS     = 0x18;  // RC0 - output src = SCL1
    RC1PPS     = 0x19;  // RC1 - output src = SDA1

    SSP1STAT = 0b10000000;
    SSP1CON1 = 0b00101000; // [7] WCOL   = 0    : write colision
                           // [6] SSPOV  = 0    : no overflow
                           // [5] SSPEN  = 1    : SDA, SCL enable
                           // [4] CKP    = 0    : clock parameter
                           // [3:0] SSPM = 1000 : I2C master mode 
                           //                     clock=FOSC/(4*(SSP1ADD+1))
                           //                           4MHz/(4*(9+1))=100KHz
    SSP1CON3 = 0x00;
    SSP1ADD  = 9;
    INTCON   = 0x00;  // interrupt disable
    
    // AD configuration
    ADCON0bits.ADON = 1;
    ADCON1 = 0b11000000; // out on right
                         // ADC CLOCK ; TABLE 22-1: 0b100 Focs/4 1.0uSec in 4MHz
                         // enable ADC

    // greeting display    
    LED1_ON();
    __delay_ms(500);
    LCDinit();
    __delay_ms(500);
    char buf[8];
    buf[0] = '0' + INA219CONF;
    buf[1] = '\0';
    LCDcursor(0, 1); LCDprint(buf); //"INA219");
    __delay_ms(500);
    LED1_OFF();
    
#ifdef HAS_TIMER2
    T2CONbits.T2OUTPS = 0b1111; // Timer2 postscaler 1:16
    T2CONbits.T2CKPS = 0b11;    // Timer2 prescaler 1:64
    // The clock source for TMR2 is fixed to the system oscillator divided by 4
    // 32000000 / 4 / 64 / 16 = 7812.5 Hz
    // 1 / (32000000 / 64 / 16) = 0.000032 sec 
    // 7812.5/128 = 244.140625 < 255
    
    PR2 = 244 - 1; // Set comparator
    
    T2CONbits.TMR2ON = 1; // Timer2 enable
    PIE1bits.TMR2IE = 1;  // Timer2 interrupt enable
    PIR1bits.TMR2IF = 0;  // Clear timer2 interrupt flag
    
    INTCONbits.PEIE = 1;  // Peripheral interrupt enable
    INTCONbits.GIE = 1;   // Global interrupt enable
#endif

    initINA219();
    
    while(1){
        if(SELECT_SW1()){
            process_ina219();
        }else{
            process_adc();
        }
        debug_val += 1;
        __delay_ms(1000);
    }
    
    return 0;
}
