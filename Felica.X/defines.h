#ifndef DEFINES_H
#define	DEFINES_H
#define FOSC (32000000UL)
#define FCY (FOSC/2)
#include <libpic30.h>

/* deep sleep for pic
 */
#if defined(__XC8__)
#define SLEEP_MODE()  SLEEP()
#elif defined(__XC16__)
#define SLEEP_MODE()  Sleep()
#elif defined(__XC32__)
#define SLEEP_MODE()  _wait()
#endif

#define OLEDROTATION 0

#endif	/* DEFINES_H */

