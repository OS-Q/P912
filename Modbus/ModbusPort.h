#ifndef __MODBUSPORT__H
#define __MODBUSPORT__H

#include "Modbus.h"

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

extern volatile unsigned char    ReceiveBuffer[MODBUS_RECEIVE_BUFFER_SIZE];
extern volatile unsigned char    ReceiveCounter;

extern void             ModBus_UART_Initialise(uint32_t baud);
extern void             ModBus_UART_Char(unsigned char c);
extern unsigned char    ModBus_UART_String(unsigned char *s, unsigned int Length);
extern void             ReceiveInterrupt(unsigned char Data);
extern void             ModBus_TimerValues(void);

#endif
