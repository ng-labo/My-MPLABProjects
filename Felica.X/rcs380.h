#ifndef RCS380_H
#define	RCS380_H

#define RECVBUFSIZE 64
#define WAITCOEF 50

int rcs380_initialize();
int rcs380_sense();

#endif	/* RCS380_H */
