/**
  Section: Included Files
*/
#include "defines.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/rtcc.h"
#include "mcc_generated_files/usb/usb.h"

#include "mylib/i2c.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
/*
    Main application
 */

#define SZ_ACC_ARRAY 50
#define SZ_VEL_ARRAY 10
#define STEP_THRESHOLD 2.2f
#define STEP_DELAY_MS 200

#define PERIODIC_MS 10

struct PedometerData {
    int ptrAcc;
    int ptrVel;
    float accXArray[SZ_ACC_ARRAY];
    float accYArray[SZ_ACC_ARRAY];
    float accZArray[SZ_ACC_ARRAY];
    float velArray[SZ_VEL_ARRAY];
    float oldVelEstimate;
    unsigned int timeMsCounter;
    unsigned int stepCounter;
} pd;

float sum(float* a, const int sz){
    int i;
    float r = 0.0;
    for(i=0;i<sz;i++){
      r += *a++;
    }
    return r;  
}

float estimate_velocity(struct PedometerData* pd) {
    float wz0 = sum(pd->accXArray, SZ_ACC_ARRAY) / SZ_ACC_ARRAY;
    float wz1 = sum(pd->accYArray, SZ_ACC_ARRAY) / SZ_ACC_ARRAY;
    float wz2 = sum(pd->accZArray, SZ_ACC_ARRAY) / SZ_ACC_ARRAY;

    float norm_factor = sqrt(wz0 * wz0 + wz1 * wz1 + wz2 * wz2);

    pd->velArray[pd->ptrVel] = (wz0 * (float) pd->accXArray[pd->ptrAcc] +
                                wz1 * (float) pd->accYArray[pd->ptrAcc] +
                                wz2 * (float) pd->accZArray[pd->ptrAcc] ) / norm_factor - norm_factor;

    if(++pd->ptrVel==SZ_VEL_ARRAY) pd->ptrVel=0;
    if(++pd->ptrAcc==SZ_ACC_ARRAY) pd->ptrAcc=0;

    return sum(pd->velArray, SZ_VEL_ARRAY);
}

int ADC_Process(ADC1_CHANNEL target) {
    int conversion;
    ADC1_Initialize();

    ADC1_Enable();
    ADC1_ChannelSelect(target);
    ADC1_SoftwareTriggerEnable();
    __delay_us(10);
    ADC1_SoftwareTriggerDisable();
    while(!ADC1_IsConversionComplete(target));
    conversion = ADC1_ConversionResultGet(target);
    ADC1_Disable();
    return conversion;
}

#define LED1_ON()       (LATAbits.LATA0 = 1)
#define LED1_OFF()      (LATAbits.LATA0 = 0)
#define LED1_FLIP()     (LATAbits.LATA0 = ~LATAbits.LATA0)
#define LED2_ON()       (LATAbits.LATA1 = 1)
#define LED2_OFF()      (LATAbits.LATA1 = 0)
#define LED2_FLIP()     (LATAbits.LATA1 = ~LATAbits.LATA1)

static uint16_t eeprom_magic;
static struct tm eeprom_ts;
static uint16_t eeprom_pd_step;
static uint16_t eeprom_pd_step_history[8];

#define EEPROM_ADDR_MAGIC 0x0000
#define EEPROM_ADDR_TS 0x0010
#define EEPROM_ADDR_PD 0x0030
#define EEPROM_ADDR_PDHIS 0x0040

#define UPDATE_INTERVAL 100

//#define DEBUG_ON_UART1

static void save_data() {
    uint8_t r;
    r = write_24lc64(EEPROM_ADDR_MAGIC, (uint8_t*) &eeprom_magic, 2);
#ifdef DEBUG_ON_UART1
    printf("# r = %d\n", r);
#endif
    r = write_24lc64(EEPROM_ADDR_TS, (uint8_t*) &eeprom_ts.tm_min, 16);
#ifdef DEBUG_ON_UART1
    printf("# r = %d\n", r);
#endif
    r = write_24lc64(EEPROM_ADDR_PD, (uint8_t*) &eeprom_pd_step, 2);
#ifdef DEBUG_ON_UART1
    printf("# r = %d\n", r);
#endif
    r = write_24lc64(EEPROM_ADDR_PDHIS, (uint8_t*) &eeprom_pd_step_history, 16);
#ifdef DEBUG_ON_UART1
    printf("# r = %d\n", r);
#endif
}

static void load_data() {
    uint8_t r;
    r = read_24lc64(EEPROM_ADDR_MAGIC, (uint8_t*) &eeprom_magic, 2);
#ifdef DEBUG_ON_UART1
    printf("## r = %d magic=%x\n", r, eeprom_magic);
#endif
    r = read_24lc64(EEPROM_ADDR_TS, (uint8_t*) &eeprom_ts.tm_min, 16);
    eeprom_ts.tm_sec = 0;
    
    r = read_24lc64(EEPROM_ADDR_PD, (uint8_t*) &eeprom_pd_step, 2);
    r = read_24lc64(EEPROM_ADDR_PDHIS, (uint8_t*) &eeprom_pd_step_history, 16);
}

void APP_Initialize(){
    // data structure for pedmeter processing
    memset(&pd, 0, sizeof(struct PedometerData));

    // read eeprom
    load_data();

#ifdef DEBUG_ON_UART1
    printf("read magic = 0x%x\n", eeprom_magic);
#endif
    if(eeprom_magic==0x8686){
#ifdef DEBUG_ON_UART1
        printf("valid magic\n");
#endif
        pd.stepCounter = eeprom_pd_step;
        RTCC_TimeSet(&eeprom_ts);
        LED1_ON();
    }else{
#ifdef DEBUG_ON_UART1
        printf("memory is not initialized\n");
#endif
        eeprom_magic = 0x8686;
        eeprom_pd_step = pd.stepCounter;
        int i=0;
        for(i=0;i<8;i++) eeprom_pd_step_history[i] = 0;
        RTCC_TimeGet(&eeprom_ts);
        save_data();
    }    
}

static uint8_t readBuffer[64];
static uint8_t readBudderIndex = 0;
static char writeBuffer[64];

#define STRING_OK "OK\n"
#define STRING_NG "NG\n"

void CDC_Tasks(void) {
    if( USBGetDeviceState() < CONFIGURED_STATE ) {
        return;
    }

    if( USBIsDeviceSuspended()== true ) {
        return;
    }

    if( USBUSARTIsTxTrfReady() == true){
        uint8_t numBytesRead;

        numBytesRead = getsUSBUSART(&readBuffer[readBudderIndex], sizeof(readBuffer)-readBudderIndex-1);
        if(numBytesRead==0){
            return;
        }
        
        readBudderIndex += numBytesRead;
        readBuffer[readBudderIndex] = 0;
        if(readBudderIndex>63){
#ifdef DEBUG_ON_UART1
            printf("reset buffer\n");
#endif
            readBudderIndex = 0;
            readBuffer[0] = 0;
            return;
        }
        
        if(readBuffer[readBudderIndex-1]=='\n'){
#ifdef DEBUG_ON_UART1
            printf((const char*) readBuffer);
#endif
            
            if(readBuffer[0]=='T'&&readBudderIndex==12){
                // TYYMMDDhhmm
                // 01234567890
                uint8_t* p = &readBuffer[1];
                int YY = (*p++ - '0') * 10;
                YY += (*p++ - '0');
                int MM =  (*p++ - '0') * 10;
                MM += (*p++ - '0');
                int DD =  (*p++ - '0') * 10;
                DD += (*p++ - '0');
                int hh =  (*p++ - '0') * 10;
                hh += (*p++ - '0');
                int mm =  (*p++ - '0') * 10;
                mm += (*p++ - '0');
                if(MM<0||MM>12||DD<0||DD>31||hh<0||hh>23||mm<0||mm>59){
                    putsUSBUSART(STRING_NG);
                }else{
                    struct tm tmbuf;
                    tmbuf.tm_year = YY;
                    tmbuf.tm_mon = MM;
                    tmbuf.tm_mday = DD;
                    tmbuf.tm_hour = hh;
                    tmbuf.tm_min = mm;
                    tmbuf.tm_sec = 0;
                    RTCC_TimeSet(&tmbuf);
                    eeprom_ts = tmbuf;
                    putsUSBUSART(STRING_OK);
                }
            } else if(readBuffer[0]=='T'&&readBudderIndex==2){
                struct tm tmbuf;
                RTCC_TimeGet(&tmbuf);
                sprintf(writeBuffer, "%d/%02d/%02d %02d:%02d\n",
                        tmbuf.tm_year, tmbuf.tm_mon, tmbuf.tm_mday,
                        tmbuf.tm_hour, tmbuf.tm_min);
                putsUSBUSART(writeBuffer);
            } else if(readBuffer[0]=='G'){
                sprintf(writeBuffer, "%u %u %u %u %u %u %u %u %u\n",
                        pd.stepCounter,
                        eeprom_pd_step_history[0],
                        eeprom_pd_step_history[1],
                        eeprom_pd_step_history[2],
                        eeprom_pd_step_history[3],
                        eeprom_pd_step_history[4],
                        eeprom_pd_step_history[5],
                        eeprom_pd_step_history[6],
                        eeprom_pd_step_history[7]);
                putsUSBUSART(writeBuffer);
            } else if(readBuffer[0]=='R'){
                pd.stepCounter = 0;
                putsUSBUSART(STRING_OK);
            }
            readBudderIndex = 0;
            readBuffer[0] = 0;
        }

    }

    CDCTxService();
}

//#define CHECK_PEFORM_24LC64
#ifdef CHECK_PEFORM_24LC64
void test(){

    uint8_t i;
    uint8_t r;
    load_data();
    printf("load_data eeprom_magic=%x\n",  eeprom_magic);
    
    eeprom_magic = 0x8686;
    save_data();
    __delay_ms(50);
    load_data();
    printf("eeprom_magic=%x.....\n",  eeprom_magic);
    
    //while(1);
    LED1_ON();
    eeprom_magic = 0x1234;
    eeprom_pd_step = 123;
    for(i=0;i<8;i++){
        eeprom_pd_step_history[i] = i;
    }
    save_data();
    __delay_ms(1000);
    load_data();
    printf("magic = %x\n", eeprom_magic);
    printf("step = %d\n", eeprom_pd_step);
    for(i=0;i<8;i++){
        printf("his[%d]=%d\n", i, eeprom_pd_step_history[i]);
    }
    uint16_t buf;
    read_24lc64(EEPROM_ADDR_MAGIC, (uint8_t*) &buf, 2);
    printf("buf=%x\n", buf);
    LED1_OFF();
    
    eeprom_magic = 0x8686;
    eeprom_pd_step = 0;
    for(i=0;i<8;i++){
        eeprom_pd_step_history[i] = 0;
    }
    RTCC_TimeGet(&eeprom_ts);    
    save_data();
    
    __delay_ms(10);
    load_data();
    printf("hung-up");
    while(1);
}
#endif

int main(void) {
    // initialize the device
    SYSTEM_Initialize();
    
    __delay_ms(1000);
#ifdef DEBUG_ON_UART1
    printf("Hello Pedometer\n");
    printf("sizeof(struct tm)=%d\n", sizeof(struct tm));
#endif

#ifdef CHECK_PEFORM_24LC64
    test();    
#endif
    
    APP_Initialize();

#ifdef DEBUG_ON_UART1
    printf("saved.magic = %x\n", eeprom_magic);
    printf("pd.stepCounter = %d\n", pd.stepCounter);
    {
        struct tm tmbuf;
        if(RTCC_TimeGet(&tmbuf)){
            printf("%d %02d/%02d %02d:%02d:%02d\n",
                    pd.stepCounter,
                    tmbuf.tm_mon, tmbuf.tm_mday,
                    tmbuf.tm_hour, tmbuf.tm_min, tmbuf.tm_sec);
        }
    }
#endif    
    
    LED2_OFF();

    struct tm curtm;
    RTCC_TimeGet(&curtm);
    uint16_t tic = 0;
    unsigned int last_stepCounter = 0;
    
    while (1) {
        CDC_Tasks();
        
        pd.accZArray[pd.ptrAcc] = (float) (ADC_Process(channel_AN11) - 512) / 102.4;
        pd.accYArray[pd.ptrAcc] = (float) (ADC_Process(channel_AN10) - 512) / 102.4;
        pd.accXArray[pd.ptrAcc] = (float) (ADC_Process(channel_AN9) - 512) / 102.4;
        float velEstimate = estimate_velocity(&pd);

        if(velEstimate > STEP_THRESHOLD &&
           pd.oldVelEstimate <= STEP_THRESHOLD &&
           (pd.timeMsCounter > STEP_DELAY_MS)){
            pd.stepCounter ++;
            pd.timeMsCounter = 0;
            
            if(pd.stepCounter%UPDATE_INTERVAL==0){
                eeprom_pd_step = pd.stepCounter;
                RTCC_TimeGet(&eeprom_ts);
                save_data();
#ifdef DEBUG_ON_UART1
                printf("saved %d %02d/%02d %02d:%02d:%02d\n",
                        eeprom_pd_step,
                        eeprom_ts.tm_mon, eeprom_ts.tm_mday,
                        eeprom_ts.tm_hour, eeprom_ts.tm_min, eeprom_ts.tm_sec);
#endif    
            }
            LED1_ON();
        }
        
        if(STEP_DELAY_MS<pd.timeMsCounter){
            LED1_OFF();
        }
        
        pd.oldVelEstimate = velEstimate;
        pd.timeMsCounter += PERIODIC_MS;
        
        tic++;
        if(tic>6000){
            tic = 0;
            /*if(last_stepCounter>0&&last_stepCounter==pd.stepCounter){
                LED2_OFF();
                Sleep();
            }else {
                last_stepCounter = pd.stepCounter;
            }*/
            
            if(RTCC_TimeGet(&curtm)){
                if(curtm.tm_mday!=eeprom_ts.tm_mday){
                    int i;
                    for(i=6;i>=0;i--){
                        eeprom_pd_step_history[i+1] = eeprom_pd_step_history[i];
                    }
                    eeprom_pd_step_history[0] = pd.stepCounter;
                    pd.stepCounter = 0;
                }
                if(curtm.tm_mday!=eeprom_ts.tm_mday||curtm.tm_hour != eeprom_ts.tm_hour){
                    eeprom_pd_step = pd.stepCounter;
                    eeprom_ts = curtm;
                    save_data();
                }
            }
        }else{
            //LED2_ON();
        }
        __delay_ms(9);
    }

    return 1;
}
/**
 End of File
*/

