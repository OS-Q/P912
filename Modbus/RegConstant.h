#ifndef __MODBUS_CONST__H
#define __MODBUS_CONST__H

#include "HAL_device.h"
#include "HAL_conf.h"
#include "Modbus.h"
#include "ModbusPort.h"

#define CHIP_UID_ADDR           (0x1FFFF7E8)
#define FLASH_MAX_LEN               (0x8000)
#define FLASH_MB_BAIS               (0x4000)

#define MODBUS_CONFIG_ADDR      (0x08006000)



uint16_t Modbus_write_reg(uint32_t addr,uint16_t Data);
uint32_t Modbus_write_word(uint32_t addr,uint32_t Data);

#if MODBUS_REGISTERS

uint8_t Modbus_flash2regs(void);
uint8_t Modbus_regs2flash(void);

#endif

uint8_t Modbus_config_update(void);

#endif
