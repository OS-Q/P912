

#include "Modbus.h"
#include "ModbusPort.h"
#include "RegConstant.h"
#include "uart.h"

#define MODBUS_DMA_RX                      RXBuf

/*******************************ModBus Functions*******************************/
#define MODBUS_READ_COILS                  1
#define MODBUS_READ_DISCRETE_INPUTS        2
#define MODBUS_READ_HOLDING_REGISTERS      3
#define MODBUS_READ_INPUT_REGISTERS        4
#define MODBUS_WRITE_SINGLE_COIL           5
#define MODBUS_WRITE_SINGLE_REGISTER       6
#define MODBUS_WRITE_MULTIPLE_COILS        15
#define MODBUS_WRITE_MULTIPLE_REGISTERS    16

/****************************End of ModBus Functions***************************/

#define QITAS_FALSE_FUNCTION                    0
#define QITAS_FALSE_SLAVE_ADDRESS               1
#define QITAS_DATA_NOT_READY                    2
#define QITAS_DATA_READY                        3

#define QITAS_ERROR_CODE_01                     0x01                            // Function code is not supported
#define QITAS_ERROR_CODE_02                     0x02                            // Register address is not allowed or write-protected
#define QITAS_ERROR_CODE_03                     0x03                            // Some data values are out of range, invalid number of register

Config Qitas;

typedef enum
{
    QITAS_RXTX_IDLE,
    QITAS_RXTX_START,
    QITAS_RXTX_DATABUF,
    QITAS_RXTX_WAIT_ANSWER,
    QITAS_RXTX_TIMEOUT
}QITAS_RXTX_STATE;

typedef struct
{
  unsigned char     Address;
  unsigned char     Function;
  unsigned char     DataBuf[MODBUS_RXTX_BUFFER_SIZE];
  unsigned short    DataLen;
}QITAS_RXTX_DATA;

/**********************Slave Transmit and Receive Variables********************/
QITAS_RXTX_DATA     Qitas_Tx_Data;
unsigned int        Qitas_Tx_Current              = 0;
unsigned int        Qitas_Tx_CRC16                = 0xFFFF;
QITAS_RXTX_STATE    Qitas_Tx_State                = QITAS_RXTX_IDLE;
unsigned char       Qitas_Tx_Buf[MODBUS_TRANSMIT_BUFFER_SIZE];
unsigned int        Qitas_Tx_Buf_Size             = 0;

QITAS_RXTX_DATA     Qitas_Rx_Data;
unsigned int        Qitas_Rx_CRC16                = 0xFFFF;
QITAS_RXTX_STATE    Qitas_Rx_State                = QITAS_RXTX_IDLE;
unsigned char       Qitas_Rx_Data_Available       = FALSE;

volatile unsigned short ModbusTimerValue         = 0;

/**************** End of Slave Transmit and Receive Variables *******************/
/*
 * Function Name        : CRC16
 * @param[in]           : Data  - Data to Calculate crc16
 * @param[in/out]       : crc16   - Anlik crc16 degeri
 * @How to use          : First initial data has to be 0xFFFF.
 */

void Qitas_CRC16(const unsigned char Data, unsigned int* crc16)
{
    unsigned int i;
    *crc16 = *crc16 ^(unsigned int) Data;
    for (i = 8; i > 0; i--)
    {
        if (*crc16 & 0x0001)
            *crc16 = (*crc16 >> 1) ^ 0xA001;
        else
            *crc16 >>= 1;
    }
}

/******************************************************************************/
/*
 * Function Name        : DoTx
 * @param[out]          : TRUE
 * @How to use          : It is used for send data package over physical layer
 */
unsigned char Qitas_DoSlaveTX(void)
{  
    ModBus_UART_String(Qitas_Tx_Buf,Qitas_Tx_Buf_Size);
    Qitas_Tx_Buf_Size = 0;
    return TRUE;
}

/*****************************************************************************/
/*
 * Function Name        : SendMessage
 * @param[out]          : TRUE/FALSE
 * @How to use          : This function start to sending messages
 */

unsigned char QitasSendMessage(void)
{
    if (Qitas_Tx_State != QITAS_RXTX_IDLE) return FALSE;
    Qitas_Tx_Current  =0;
    Qitas_Tx_State    =QITAS_RXTX_START;
    return TRUE;
}

/******************************************************************************/
/*
 * Function Name        : HandleModbusError
 * @How to use          : This function generated errors to Modbus Master
 */

void HandleModbusError(char ErrorCode)
{
    // Initialise the output buffer. The first byte in the buffer says how many registers we have read
    Qitas_Tx_Data.Function    = ErrorCode | 0x80;
    Qitas_Tx_Data.Address     = Qitas.slave.addr;
    Qitas_Tx_Data.DataLen     = 0;
    QitasSendMessage();
}

/******************************************************************************/
/*
 * Function Name        : HandleModbusReadHoldingRegisters
 * @How to use          : Modbus function 03 - Read holding registers
 */

#if MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0
void HandleModbusReadHoldingRegisters(void)
{
    // Holding registers are effectively numerical outputs that can be written to by the host.
    // They can be control registers or analogue outputs.
    // We potientially have one - the pwm output value
    unsigned int    Qitas_StartAddress        = 0;
    unsigned int    Qitas_NumberOfRegisters   = 0;
    unsigned int    Qitas_i                   = 0;

    // The message contains the requested start address and number of registers
    Qitas_StartAddress        = ((unsigned int) (Qitas_Rx_Data.DataBuf[0]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[1]);
    Qitas_NumberOfRegisters   = ((unsigned int) (Qitas_Rx_Data.DataBuf[2]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[3]);

    // If it is bigger than RegisterNumber return error to Modbus Master
    if((Qitas_StartAddress+Qitas_NumberOfRegisters)>NUMBER_OF_OUTPUT_REGISTERS) HandleModbusError(QITAS_ERROR_CODE_02);
    else
    {
        // Initialise the output buffer. The first byte in the buffer says how many registers we have read
        Qitas_Tx_Data.Function    = MODBUS_READ_HOLDING_REGISTERS;
        Qitas_Tx_Data.Address     = Qitas.slave.addr;
        Qitas_Tx_Data.DataLen     = 1;
        Modbus_flash2regs();
        for (Qitas_i = 0; Qitas_i < Qitas_NumberOfRegisters; Qitas_i++)
        {
            unsigned short Qitas_CurrentData = Registers[Qitas_StartAddress+Qitas_i].ActValue;
            Qitas_Tx_Data.DataBuf[Qitas_Tx_Data.DataLen]        = (unsigned char) ((Qitas_CurrentData & 0xFF00) >> 8);
            Qitas_Tx_Data.DataBuf[Qitas_Tx_Data.DataLen + 1]    = (unsigned char) (Qitas_CurrentData & 0xFF);
            Qitas_Tx_Data.DataLen+= 2;
            Qitas_Tx_Data.DataBuf[0] = Qitas_Tx_Data.DataLen - 1;
        }
        QitasSendMessage();
    }
}
#endif

/******************************************************************************/
/*
 * Function Name        : HandleModbusReadInputRegisters
 * @How to use          : Modbus function 06 - Write single register
 */

#if MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0

void HandleModbusWriteSingleRegister(void)
{
    // Write single numerical output
    unsigned int    Qitas_Address   = 0;
    unsigned int    Qitas_Value     = 0;
    unsigned char   Qitas_i         = 0;

    // The message contains the requested start address and number of registers
    Qitas_Address   = ((unsigned int) (Qitas_Rx_Data.DataBuf[0]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[1]);
    Qitas_Value     = ((unsigned int) (Qitas_Rx_Data.DataBuf[2]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[3]);

    // Initialise the output buffer. The first byte in the buffer says how many registers we have read
    Qitas_Tx_Data.Function    = MODBUS_WRITE_SINGLE_REGISTER;
    Qitas_Tx_Data.Address     = Qitas.slave.addr;
    Qitas_Tx_Data.DataLen     = 4;

    if(Qitas_Address>=NUMBER_OF_OUTPUT_REGISTERS)  HandleModbusError(QITAS_ERROR_CODE_03);   //处理寄存器超出异常
    else
    {
        // ModBus_Regs_store(Qitas_Address,Qitas_Value);
        Registers[Qitas_Address].ActValue=Qitas_Value;
        // Output data buffer is exact copy of input buffer
        
        for (Qitas_i = 0; Qitas_i < 4; ++Qitas_i)
        {
            Qitas_Tx_Data.DataBuf[Qitas_i] = Qitas_Rx_Data.DataBuf[Qitas_i];
        }    
    }
    QitasSendMessage();
    Modbus_regs2flash();
}
#endif

/******************************************************************************/
/*
 * Function Name        : HandleModbusWriteMultipleRegisters
 * @How to use          : Modbus function 16 - Write multiple registers
 */

#if MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED > 0

void HandleModbusWriteMultipleRegisters(void)
{
    // Write single numerical output
    unsigned int    Qitas_StartAddress            =0;
    unsigned char   Qitas_ByteCount               =0;
    unsigned int    Qitas_NumberOfRegisters       =0;
    unsigned char   Qitas_i                       =0;
    unsigned int    Qitas_Value                   =0;

    // The message contains the requested start address and number of registers
    Qitas_StartAddress        = ((unsigned int) (Qitas_Rx_Data.DataBuf[0]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[1]);
    Qitas_NumberOfRegisters   = ((unsigned int) (Qitas_Rx_Data.DataBuf[2]) << 8) + (unsigned int) (Qitas_Rx_Data.DataBuf[3]);
    Qitas_ByteCount           = Qitas_Rx_Data.DataBuf[4];

    // If it is bigger than RegisterNumber return error to Modbus Master
    if((Qitas_StartAddress+Qitas_NumberOfRegisters)>NUMBER_OF_OUTPUT_REGISTERS) HandleModbusError(QITAS_ERROR_CODE_03);
    else
    {
        // Initialise the output buffer. The first byte in the buffer says how many outputs we have set
        Qitas_Tx_Data.Function    = MODBUS_WRITE_MULTIPLE_REGISTERS;
        Qitas_Tx_Data.Address     = Qitas.slave.addr;
        Qitas_Tx_Data.DataLen     = 4;
        Qitas_Tx_Data.DataBuf[0]  = Qitas_Rx_Data.DataBuf[0];
        Qitas_Tx_Data.DataBuf[1]  = Qitas_Rx_Data.DataBuf[1];
        Qitas_Tx_Data.DataBuf[2]  = Qitas_Rx_Data.DataBuf[2];
        Qitas_Tx_Data.DataBuf[3]  = Qitas_Rx_Data.DataBuf[3];
        
        // Output data buffer is exact copy of input buffer
        for (Qitas_i = 0; Qitas_i <Qitas_NumberOfRegisters; Qitas_i++)
        {
            Qitas_Value=(Qitas_Rx_Data.DataBuf[5+2*Qitas_i]<<8)+(Qitas_Rx_Data.DataBuf[6+2*Qitas_i]);
            Registers[Qitas_StartAddress+Qitas_i].ActValue=Qitas_Value;
        }
        QitasSendMessage();
        Modbus_regs2flash();
    }
}
#endif

/******************************************************************************/
/*
 * Function Name        : RxDataAvailable
 * @return              : If Data is Ready, Return TRUE
 *                        If Data is not Ready, Return FALSE
 */

unsigned char Qitas_RxDataAvailable(void)
{
    unsigned char Result    = Qitas_Rx_Data_Available;   
    Qitas_Rx_Data_Available = FALSE;
    return Result;
}

/******************************************************************************/
/*
 * Function Name        : CheckRxTimeout
 * @return              : If Time is out return TRUE
 *                        If Time is not out return FALSE
 */

unsigned char Qitas_CheckRxTimeout(void)
{
    // A return value of true indicates there is a timeout    
    if (ModbusTimerValue>= MODBUS_TIMEOUTTIMER)
    {
        ModbusTimerValue =0;
        ReceiveCounter   =0;
        return TRUE;
    }
    return FALSE;
}

/******************************************************************************
 * Function Name        : CheckBufferComplete
 * @return              : If data is ready, return              DATA_READY
 *                        If slave address is wrong, return     FALSE_SLAVE_ADDRESS
 *                        If data is not ready, return          DATA_NOT_READY
 *                        If functions is wrong, return         FALSE_FUNCTION
 */

#if MODBUS_DMA_LEN

unsigned char CheckModbusBufferComplete(void)
{
    int ExpectedReceiveCount=0;
    if(strlen(MODBUS_DMA_RX)>4)
    {
        if(MODBUS_DMA_RX[0]==Qitas.slave.addr)
        {
            if(MODBUS_DMA_RX[1]==0x01 || MODBUS_DMA_RX[1]==0x02 || MODBUS_DMA_RX[1]==0x03 || MODBUS_DMA_RX[1]==0x04 || MODBUS_DMA_RX[1]==0x05 || MODBUS_DMA_RX[1]==0x06)
            {
                ExpectedReceiveCount = 8;
            }
            else if(MODBUS_DMA_RX[1]==0x0F || MODBUS_DMA_RX[1]==0x10)
            {
                ExpectedReceiveCount=MODBUS_DMA_RX[6]+9;
            }
            else
            {
                ReceiveCounter=0;
                return QITAS_FALSE_FUNCTION;
            }
        }
        else
        {
            ReceiveCounter=0;
            return QITAS_FALSE_SLAVE_ADDRESS;
        }
    }
    else return QITAS_DATA_NOT_READY;
    if(ReceiveCounter==ExpectedReceiveCount)
    {
        memcpy(ReceiveBuffer,MODBUS_DMA_RX,ExpectedReceiveCount);
        memset(MODBUS_DMA_RX,0,sizeof(MODBUS_DMA_RX));
        return QITAS_DATA_READY;
    }
    return QITAS_DATA_NOT_READY;
}

#else

unsigned char CheckModbusBufferComplete(void)
{
    int ExpectedReceiveCount=0;

    if(ReceiveCounter>4)
    {
        if(ReceiveBuffer[0]==Qitas.slave.addr)
        {
            if(ReceiveBuffer[1]==0x01 || ReceiveBuffer[1]==0x02 || ReceiveBuffer[1]==0x03 || ReceiveBuffer[1]==0x04 || ReceiveBuffer[1]==0x05 || ReceiveBuffer[1]==0x06)
            {
                ExpectedReceiveCount = 8;
            }
            else if(ReceiveBuffer[1]==0x0F || ReceiveBuffer[1]==0x10)
            {
                ExpectedReceiveCount=ReceiveBuffer[6]+9;
            }
            else
            {
                ReceiveCounter=0;
                return QITAS_FALSE_FUNCTION;
            }
        }
        else
        {
            ReceiveCounter=0;
            return QITAS_FALSE_SLAVE_ADDRESS;
        }
    }
    else return QITAS_DATA_NOT_READY;
    if(ReceiveCounter==ExpectedReceiveCount)
    {
        return QITAS_DATA_READY;
    }
    return QITAS_DATA_NOT_READY;
}

#endif
/******************************************************************************/
/*
 * Function Name        : RxRTU
 * @How to use          : Check for data ready, if it is good return answer
 */

void Qitas_RxRTU(void)
{
    unsigned char   Qitas_i;
    unsigned char   Qitas_ReceiveBufferControl=0;

    Qitas_ReceiveBufferControl = CheckModbusBufferComplete();

    if(Qitas_ReceiveBufferControl==QITAS_DATA_READY)
    {
        Qitas_Rx_Data.Address               =ReceiveBuffer[0];
        Qitas_Rx_CRC16                      = 0xffff;
        Qitas_CRC16(Qitas_Rx_Data.Address, &Qitas_Rx_CRC16);
        Qitas_Rx_Data.Function              =ReceiveBuffer[1];
        Qitas_CRC16(Qitas_Rx_Data.Function, &Qitas_Rx_CRC16);

        Qitas_Rx_Data.DataLen=0;

        for(Qitas_i=2;Qitas_i<ReceiveCounter;Qitas_i++)
        {
            Qitas_Rx_Data.DataBuf[Qitas_Rx_Data.DataLen++]=ReceiveBuffer[Qitas_i];
        }         
        Qitas_Rx_State =QITAS_RXTX_DATABUF;
        ReceiveCounter=0;
    }

    Qitas_CheckRxTimeout();

    if ((Qitas_Rx_State == QITAS_RXTX_DATABUF) && (Qitas_Rx_Data.DataLen >= 2))
    {
        // Finish off our crc16 check
        Qitas_Rx_Data.DataLen -= 2;
        for (Qitas_i = 0; Qitas_i < Qitas_Rx_Data.DataLen; ++Qitas_i)
        {
            Qitas_CRC16(Qitas_Rx_Data.DataBuf[Qitas_i], &Qitas_Rx_CRC16);
        }        
        if (((unsigned int) Qitas_Rx_Data.DataBuf[Qitas_Rx_Data.DataLen] + ((unsigned int) Qitas_Rx_Data.DataBuf[Qitas_Rx_Data.DataLen + 1] << 8)) == Qitas_Rx_CRC16)
        {
            // Valid message!
            Qitas_Rx_Data_Available = TRUE;
        }
        Qitas_Rx_State = QITAS_RXTX_IDLE;
    }
}

/******************************************************************************/
/*
 * Function Name        : TxRTU
 * @How to use          : If it is ready send answers!
 */

void Qitas_TxRTU(void)
{
    Qitas_Tx_CRC16                =0xFFFF;
    Qitas_Tx_Buf_Size             =0;
    Qitas_Tx_Buf[Qitas_Tx_Buf_Size++]   =Qitas_Tx_Data.Address;
    Qitas_CRC16(Qitas_Tx_Data.Address, &Qitas_Tx_CRC16);
    Qitas_Tx_Buf[Qitas_Tx_Buf_Size++]   =Qitas_Tx_Data.Function;
    Qitas_CRC16(Qitas_Tx_Data.Function, &Qitas_Tx_CRC16);

    for(Qitas_Tx_Current=0; Qitas_Tx_Current < Qitas_Tx_Data.DataLen; Qitas_Tx_Current++)
    {
        Qitas_Tx_Buf[Qitas_Tx_Buf_Size++]=Qitas_Tx_Data.DataBuf[Qitas_Tx_Current];
        Qitas_CRC16(Qitas_Tx_Data.DataBuf[Qitas_Tx_Current], &Qitas_Tx_CRC16);
    }
    
    Qitas_Tx_Buf[Qitas_Tx_Buf_Size++] = Qitas_Tx_CRC16 & 0x00FF;
    Qitas_Tx_Buf[Qitas_Tx_Buf_Size++] =(Qitas_Tx_CRC16 & 0xFF00) >> 8;

    Qitas_DoSlaveTX();
    Qitas_Tx_State    =QITAS_RXTX_IDLE;
}

/******************************************************************************/
/*
 * Function Name        : ProcModbus
 * @How to use          : ModBus main core! Call this function into main!
 */


void ProcModbus(void)
{
    if (Qitas_Tx_State != QITAS_RXTX_IDLE) Qitas_TxRTU();
    Qitas_RxRTU();                                                        
    if (Qitas_RxDataAvailable())                                            // If data is ready enter this!
    {
        if (Qitas_Rx_Data.Address == Qitas.slave.addr)                      // Is Data for us?
        {
            switch (Qitas_Rx_Data.Function)                                     // Data is for us but which function?
            {
                #if MODBUS_READ_HOLDING_REGISTERS_ENABLED > 0
                case MODBUS_READ_HOLDING_REGISTERS:
                {   
                    HandleModbusReadHoldingRegisters();
                    break;  
                }
                #endif
                #if MODBUSWRITE_SINGLE_REGISTER_ENABLED > 0
                case MODBUS_WRITE_SINGLE_REGISTER:     
                {   
                    HandleModbusWriteSingleRegister();
                    break;  
                }
                #endif
                #if MODBUS_WRITE_MULTIPLE_REGISTERS_ENABLED > 0
                case MODBUS_WRITE_MULTIPLE_REGISTERS:  
                {   
                    HandleModbusWriteMultipleRegisters();      
                    break;  
                }
                #endif
                default:
                { 
                    HandleModbusError(QITAS_ERROR_CODE_01);    
                    break;  
                }
            }
        }
    }
}

/******************************************************************************/
/*
 * Function Name        : InitModbus
 * @How to use          : Qitas ModBus slave initialize
 */

void check_cfg_data(void)
{
    Modbus_flash2regs();
    if(Qitas.slave.baud>6)
    {
        Qitas.slave.addr = 0x02;
        Qitas.slave.baud = 0x03;
        Qitas.slave.ver = 0x100;          
        Modbus_config_update(); 
    }
}

void InitModbus(void)
{

    check_cfg_data();
    // ModBus_UART_Initialise(9600);
    switch (Qitas.slave.baud)
    {
        case 1:
            ModBus_UART_Initialise(2400); 
        break;
        case 2:
            ModBus_UART_Initialise(4800); 
        break;
        case 3:
            ModBus_UART_Initialise(9600); 
        break; 
        case 4:
            ModBus_UART_Initialise(19200); 
        break;    
        default:
            ModBus_UART_Initialise(9600);
        break;
    }
       
}

