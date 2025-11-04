#ifndef _INTERNAL_FLASH_H
#define _INTERNAL_FLASH_H
/*
 * Author: LeHoaiGiang
 * email : lehoaigiangg@gmail.com
 * Contact : 0336379944
 * I am SYNO24X machine
 * Project name: SYNO24X (oligonucleotide synthesis system)
 * Flash internal general for save setting very importance
 *
 */
#include "stm32f4xx_hal.h"
#include "stdint.h"
#include "string.h"
#include "struct.h"

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base address of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base address of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base address of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base address of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base address of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base address of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base address of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base address of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base address of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base address of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base address of Sector 11, 128 Kbytes */

#define ADDR_FLASH_SECTOR_READ_WRITE_MANYTIME    ((uint32_t)0x080E0000) /* Base address of Sector read  write data manytime, save setting  */


void Flash_Check_ReadWrite();
void writeASector(uint32_t u32_address, uint8_t u8_data_input[], uint16_t u16_size_data);
uint32_t GetSector(uint32_t Address);
HAL_StatusTypeDef Flash_WriteArray(uint32_t StartAddress, uint16_t *Data, uint16_t Length);
uint32_t Flash_Write_Data(uint32_t StartSectorAddress, uint16_t *Data, uint16_t numberofwords);
void Flash_ReadArray(uint32_t StartAddress, uint16_t *Data, uint16_t Length);
void readWriteFlashInit(Global_var_t* p_global_variable);
uint32_t Flash_Write_Data_Byte(uint32_t StartAddress, uint8_t *Data, uint16_t numberofbytes);
void Flash_ReadArray_Byte(uint32_t StartAddress, uint8_t *Data, uint16_t Length);
void SYNO_testFlash();
void print_arrayX(int array[12][12]);
void print_arrayY(int array[12][8]);
void verifyAndSaveData(uint8_t data[512]);
#endif
