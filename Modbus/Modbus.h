#ifndef __QMODBUS_H
#define __QMODBUS_H

#include "HAL_device.h"
#include "HAL_conf.h"


#define NUMBER_OF_OUTPUT_REGISTERS                    27                    // Modbus RTU Slave Output Register Number                                                      
#define MODBUS_TIMEOUTTIMER                         1000                    // Timeout Constant for Modbus RTU Slave [millisecond]
#define MODBUS_READ_HOLDING_REGISTERS_ENABLED      ( 1 )                   // If you want to use make it 1, or 0
#define MODBUSWRITE_SINGLE_REGISTER_ENABLED        ( 1 )                   // If you want to use make it 1, or 0
#define MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED    ( 1 )                   // If you want to use make it 1, or 0

#define  MODBUS_REGISTERS (MODBUS_READ_HOLDING_REGISTERS_ENABLED || MODBUSWRITE_SINGLE_REGISTER_ENABLED || MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED)

// Buffers for Qitas Modbus RTU Slave
#define MODBUS_RECEIVE_BUFFER_SIZE                 (NUMBER_OF_OUTPUT_REGISTERS*2 + 5) 
#define MODBUS_TRANSMIT_BUFFER_SIZE                MODBUS_RECEIVE_BUFFER_SIZE
#define MODBUS_RXTX_BUFFER_SIZE                    MODBUS_TRANSMIT_BUFFER_SIZE

// #define MODBUS_DMA_LEN                            (MODBUS_TRANSMIT_BUFFER_SIZE + 2) 
typedef struct
{
  unsigned char     LByte; 
  unsigned char     HByte;       
}rpart;

typedef union
{
    short     ActValue;
    rpart     V;
}RegStructure;

typedef struct
{
    RegStructure    LR;
    RegStructure    HR;      
}doubleReg;

typedef union
{
  doubleReg    DR;
  unsigned int  W;
}CRC32b;

typedef struct
{
  unsigned char     addr;         //Modbus RTU Slave addr[0-255]
  unsigned char     baud;      
  unsigned short    ver;
}QITAS_MB_PORT;

typedef union 
{
  unsigned int  pdata;
  QITAS_MB_PORT slave;
}Config;

// Variable for Slave Address                              
extern Config Qitas;

extern RegStructure Registers[NUMBER_OF_OUTPUT_REGISTERS];
extern volatile unsigned short ModbusTimerValue;

// Main Functions
unsigned char CheckModbusBufferComplete(void);

extern void  InitModbus(void);
extern void  ProcModbus(void);

#include "ModbusPort.h"
#include "RegConstant.h"

#endif
