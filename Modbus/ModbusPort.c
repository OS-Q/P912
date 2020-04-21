
#include "Modbus.h"
#include "ModbusPort.h"
#include "uart.h"

#define RS485_W       GPIO_ResetBits(GPIOB,GPIO_Pin_5);delay_ms(1)
#define RS485_R         delay_ms(1);GPIO_SetBits(GPIOB,GPIO_Pin_5)


// Modbus RTU Variables
volatile unsigned char   ReceiveBuffer[MODBUS_RECEIVE_BUFFER_SIZE];         // Buffer to collect data from hardware
volatile unsigned char   ReceiveCounter=0;                                  // Collected data number

// UART Initialize for Microconrollers, yes you can use another phsycal layer!
void ModBus_UART_Initialise(uint32_t baud)
{
    uart1_init(baud);
}

// This is used for send one character
void ModBus_UART_Char(unsigned char c)
{
    RS485_W;
    //  UartSendByte(c);
    UART_SendData(UART1,c);
    while(!UART_GetFlagStatus(UART1, UART_FLAG_TXEPT));
    RS485_R;
    // Loop until the end of transmission
}


// This is used for send string, better to use DMA for it)
unsigned char ModBus_UART_String(unsigned char *s, unsigned int Length)
{
    //Set Max485 in Transmitter mode
    RS485_W;
    UartSendGroup(s, Length);
    RS485_R;
    return TRUE;
}

/*************************Interrupt Fonction Slave*****************************/
// Call this function into your UART Interrupt. Collect data from it!
void ReceiveInterrupt(unsigned char Data)
{
    ReceiveBuffer[ReceiveCounter] = Data;
    ReceiveCounter++;
    if(ReceiveCounter>MODBUS_RECEIVE_BUFFER_SIZE) ReceiveCounter=0;
    ModbusTimerValue=0;   
}
// Better to use DMA


// Call this function into 1ms Interrupt or Event!
void ModBus_TimerValues(void)
{
    ModbusTimerValue++;
}
