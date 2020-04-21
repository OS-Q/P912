#include "init.h"
#include "RegConstant.h"

volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;

uint32_t CheckWriteProtectblank(void)
{
    uint32_t protectstatus = 0;
    u16 data1;
    int i = 0;

    for (i = 0; i < 4; i++) {
        data1 = *(u16 *)(0x1ffff808 + i * 2); //地址Address必须是2的整数倍
        if (data1 != 0xFFFF) {
            protectstatus = 1;
            break;
        }
    }
    return protectstatus;
}


uint32_t CheckReadProtect(void)
{
    uint32_t protectstatus = 0;
    u16 data1;
    int i = 0;
    if ((FLASH->OBR & 0x02) != (uint32_t)RESET) {
        // Read Protect on 0x1FFFF800 is set
        protectstatus = 1;
    }else 
    {
        for (i = 0; i < 8; i++) 
        {
            data1 = *(u16 *)(0x1ffe0000 + i * 2); //地址Address必须是2的整数倍
            if (data1 != 0xFFFF) {
                protectstatus = 2;
                break;
            }
        }
    }
    return protectstatus;
}

uint16_t Modbus_write_reg(uint32_t addr,uint16_t Data)
{
    uint16_t buff;
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASH_ErasePage(addr);
    FLASH_ClearFlag(FLASH_FLAG_EOP);
    FLASHStatus = FLASH_ProgramHalfWord(addr, Data);
    FLASH_ClearFlag(FLASH_FLAG_EOP);
    FLASH_Lock();
    buff=(*(__IO uint16_t*) addr);
    if(buff != Data) return buff;
    return 0;
}

uint32_t Modbus_write_word(uint32_t addr,uint32_t Data)
{
    uint32_t buff=(*(__IO uint32_t*) addr);
    if(buff != Data)
    {
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
        FLASH_ErasePage(addr);
        FLASH_ClearFlag(FLASH_FLAG_EOP );
        FLASHStatus = FLASH_ProgramWord(addr, Data);
        FLASH_ClearFlag(FLASH_FLAG_EOP );
        FLASH_Lock();
        buff=(*(__IO uint32_t*) addr);
        if(buff != Data)  return buff;
    }
    return 0;
}


/***********************Input/Output Coils and Registers***********************/

#if MODBUS_REGISTERS

RegStructure  Registers[NUMBER_OF_OUTPUT_REGISTERS];

/********************************************************************************************************
**REG0 ： 通信地址
**REG1 ： 通信配置
**REG2 ： 自动重合循环1时间
**REG3 ： 自动重合循环2时间
**REG4 ： 自动重合循环3时间
**REG5 ： 自动重合循环4时间
**REG6 ： 自动重合循环5时间
**REG7 ： 自动重合稳定时间
**REG8 ： 自动重合使能
**REG9 ： 电机延时补偿
**REG10 ：ADC控制模式（0=测量模式、1=控制模式）
**REG11 ：系统运行电压
**REG12 ：系统运行温度
**REG13 ：ADC1值
**REG14 ：ADC2值
**REG15 ：ADC3值
**REG16 ：重合闸状态
**REG17 ：控制分合闸
**REG18 ：软件版本号  写入新的软件版本号进入升级模式
**REG19 ：CRC32高位
**REG20 ：CRC32低位
**REG21 ：设备编号
**REG22 ：设备编号
**REG23 ：设备编号
**REG24 ：设备编号
**REG25 ：设备编号
**REG26 ：设备编号  96bits UID
********************************************************************************************************/


uint8_t Modbus_flash2regs(void)
{
  Qitas.pdata = (*(__IO uint32_t*)MODBUS_CONFIG_ADDR);
  Registers[0].V.LByte=Qitas.slave.addr;
  Registers[1].V.LByte=Qitas.slave.baud;

  Registers[18].ActValue=Qitas.slave.ver;

  Registers[19].ActValue = (*(__IO uint16_t*)CHIP_UID_ADDR);
  Registers[20].ActValue = (*(__IO uint16_t*)(CHIP_UID_ADDR+2));
  Registers[21].ActValue = (*(__IO uint16_t*)(CHIP_UID_ADDR+4));
  Registers[22].ActValue = (*(__IO uint16_t*)(CHIP_UID_ADDR+6));
  Registers[23].ActValue = (*(__IO uint16_t*)(CHIP_UID_ADDR+8)); 
  Registers[24].ActValue = (*(__IO uint16_t*)(CHIP_UID_ADDR+10)); 
  return 0;
} 

uint8_t Modbus_config_update(void)
{
    Modbus_write_word(MODBUS_CONFIG_ADDR,Qitas.pdata);
    return Modbus_flash2regs();
} 

uint8_t Modbus_regs2flash(void)
{ 
    if(Qitas.slave.addr != Registers[0].V.LByte)
    {
        Qitas.slave.addr = Registers[0].V.LByte;
    }
    if(Qitas.slave.baud != Registers[1].V.LByte)
    {
        Qitas.slave.baud = Registers[1].V.LByte;
    }
    if(Qitas.slave.ver != Registers[18].ActValue)
    {
        sys_LED3_flag=1; 
        Qitas.slave.ver = Registers[18].ActValue;
    } 
    Modbus_config_update();
    return 0;
} 



#endif
