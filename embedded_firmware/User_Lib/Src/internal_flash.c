#include "internal_flash.h"
#include "math.h"
#include "struct.h"
#include "macro.h"
#include "global_extern.h"
#include "dwt_stm32_delay.h"
#include "stm32f4xx_hal_flash_ex.h"
#include "uartPrintDebug.h"
#include "function.h"
#define STLINK_DEBUG_FLASH
uint16_t DataReadFlash[2048];
uint8_t DataArrayByte[2048];
uint16_t DataWriteFlash[256];
FLASH_OBProgramInitTypeDef OBInit;
uint32_t SECTORError = 0xFFFFFFFF;
__IO uint32_t SectorsWRPStatus = 0xFFF;
FLASH_EraseInitTypeDef EraseInitStruct;
flash_on_RAM_factory_reset_t flash_on_RAM_factory_reset;
flash_on_RAM_many_times_t flash_on_RAM_many_times;
void Flash_Check_ReadWrite()
{
	for (uint8_t idx_sensor = 0; idx_sensor < 100; idx_sensor++) {
		DataWriteFlash[idx_sensor] =  idx_sensor  + 10 + idx_sensor *2;
	}
	if (Flash_Write_Data( ADDR_FLASH_SECTOR_READ_WRITE_MANYTIME, &DataWriteFlash, 100) == HAL_OK)
	{
		Flash_ReadArray(  ADDR_FLASH_SECTOR_READ_WRITE_MANYTIME,  &DataReadFlash, 100);
	}
	else
	{
		return;
	}
}
// Hàm mở khóa flash
void Flash_Unlock(void) {
	HAL_FLASH_Unlock();
}

// Hàm khóa flash
void Flash_Lock(void) {
#ifdef STLINK_DEBUG_FLASH

#else
	HAL_FLASH_Lock();
#endif
}

void writeASector(uint32_t u32_address, uint8_t u8_data_input[], uint16_t u16_size_data)
{
	//UnlockFlash();
	eraseSector(u32_address);
	for(uint8_t flash_index = 0; flash_index < u16_size_data; flash_index++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, u32_address + flash_index, u8_data_input[flash_index]) != HAL_OK)
		{
			//LockFlash();
			//DELAY_SEND_USB;
			//printing_signal.u8_dataUSBSend[1] = WRITE_FLASH_ERROR;
			//USB_SEND_DATA(2);
		}
	}
	//LockFlash();
}

uint32_t GetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
	{
		sector = FLASH_SECTOR_0;
	}
	else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
	{
		sector = FLASH_SECTOR_1;
	}
	else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
	{
		sector = FLASH_SECTOR_2;
	}
	else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
	{
		sector = FLASH_SECTOR_3;
	}
	else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
	{
		sector = FLASH_SECTOR_4;
	}
	else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
	{
		sector = FLASH_SECTOR_5;
	}
	else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
	{
		sector = FLASH_SECTOR_6;
	}
	else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
	{
		sector = FLASH_SECTOR_7;
	}

	else if((Address <ADDR_FLASH_SECTOR_9) && (Address >=  ADDR_FLASH_SECTOR_8))
	{
		sector = FLASH_SECTOR_8;
	}
	else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
	{
		sector = FLASH_SECTOR_9;
	}
	else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
	{
		sector = FLASH_SECTOR_10;
	}
	else if(Address >= ADDR_FLASH_SECTOR_11)
	{
		sector = FLASH_SECTOR_11;
	}
	return sector;
}

// Hàm ghi một mảng uint16_t vào flash
uint32_t Flash_Write_Data(uint32_t StartSectorAddress, uint16_t *Data, uint16_t numberofwords)
{

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError;
	int sofar = 0;


	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Erase the user Flash area */

	/* Get the number of sector to erase from 1st sector */

	uint32_t StartSector = GetSector(StartSectorAddress);
	uint32_t EndSectorAddress = StartSectorAddress + numberofwords*2;
	uint32_t EndSector = GetSector(EndSectorAddress);

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector        = StartSector;
	EraseInitStruct.NbSectors     = (EndSector - StartSector) + 1;

	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	     you have to make sure that these data are rewritten before they are accessed during code
	     execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	     DCRST and ICRST bits in the FLASH_CR register. */
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	{
		return HAL_FLASH_GetError ();
	}

	/* Program the user Flash area word by word
	    (area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	while (sofar<numberofwords)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, StartSectorAddress, Data[sofar]) == HAL_OK)
		{
			StartSectorAddress += 2;  // use StartPageAddress += 2 for half word and 8 for double word
			sofar++;
		}
		else
		{
			/* Error occurred while writing data in Flash memory*/
			return HAL_FLASH_GetError ();
		}
	}

	/* Lock the Flash to disable the flash control register access (recommended
	     to protect the FLASH memory against possible unwanted operation) *********/
	HAL_FLASH_Lock();

	return 0;
}

// Hàm đọc một mảng uint16_t từ flash
void Flash_ReadArray(uint32_t StartAddress, uint16_t *Data, uint16_t Length) {
	while (1)
	{
		*Data = *(__IO uint16_t *)StartAddress;
		StartAddress += 2;
		Data++;
		if (!(Length--)) break;
	}
}


void readWriteFlashInit(Global_var_t* p_global_variable)
{
	Flash_ReadArray(ADDR_FLASH_SECTOR_READ_WRITE_MANYTIME, DataReadFlash, 100);
}



uint32_t Flash_Write_Data_Byte(uint32_t StartAddress, uint8_t *Data, uint16_t numberofbytes) {
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError;
	uint16_t sofar = 0;

	// Mở khóa Flash để có quyền ghi
	HAL_FLASH_Unlock();

	// Xác định sector bắt đầu và kết thúc cần xóa
	uint32_t StartSector = GetSector(StartAddress);
	uint32_t EndSectorAddress = StartAddress + numberofbytes;
	uint32_t EndSector = GetSector(EndSectorAddress);

	// Cấu hình để xóa sector
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector        = StartSector;
	EraseInitStruct.NbSectors     = (EndSector - StartSector) + 1;

	// Xóa sector trong Flash
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK) {
		return HAL_FLASH_GetError();
	}

	// Ghi dữ liệu từng byte vào Flash
	while (sofar < numberofbytes) {
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, StartAddress, Data[sofar]) == HAL_OK) {
			StartAddress++;  // Tiến tới byte tiếp theo
			sofar++;
		} else {
			// Xảy ra lỗi khi ghi dữ liệu
			return HAL_FLASH_GetError();
		}
	}

	// Khóa lại Flash sau khi ghi
	//HAL_FLASH_Lock();

	return HAL_OK; // Ghi thành công
}


void Flash_ReadArray_Byte(uint32_t StartAddress, uint8_t *Data, uint16_t Length) {
	// Mở khóa Flash để có quyền ghi
	HAL_FLASH_Unlock();
	while (Length--) {
		*Data = *(__IO uint8_t *)StartAddress; // Đọc dữ liệu từ địa chỉ Flash
		StartAddress++;  // Tiến tới byte tiếp theo
		Data++;
	}
	// Khóa lại Flash sau khi ghi
	//HAL_FLASH_Lock();
}


void SYNO_testFlash(){
	DEBUG_PRINT("Start Read Flash");
	uint8_t flag_positionXneed_Init = 0;
	// ghi dữ liệu vào flash write onetime // tạm thời dùng sector 9 sau. dự án thực tế dùng sector1
	// Sao chép dữ liệu từ Fill_Position_X (bên ngoài) vào Fill_Position_X trong struct
//	Init_position();
//	memcpy(&flash_on_RAM_factory_reset.Fill_Position_X, Fill_Position_X, sizeof(Fill_Position_X));
//	memcpy(&flash_on_RAM_factory_reset.Fill_Position_Y, Fill_Position_Y, sizeof(Fill_Position_Y));
//	memcpy(dataFlash_onRam.u8_dataInternalSector2,&flash_on_RAM_factory_reset,sizeof(flash_on_RAM_factory_reset_t));
//	Flash_Write_Data_Byte(ADDR_FLASH_SECTOR_11, dataFlash_onRam.u8_dataInternalSector2, sizeof(flash_on_RAM_factory_reset_t));

	Flash_ReadArray_Byte(ADDR_FLASH_SECTOR_11, &DataArrayByte[0], sizeof(flash_on_RAM_many_times_t)); // doc du lieu tu FLash

	memcpy(&flash_on_RAM_many_times, &DataArrayByte,sizeof(flash_on_RAM_many_times_t));// copy data to struct
	//memcpy(dataFlash_onRam.u8_dataInternalSector2,&flash_on_RAM_many_times,sizeof(flash_on_RAM_many_times_t)); //
	memcpy(&Fill_Position_X, &flash_on_RAM_many_times.Fill_Position_X,sizeof(Fill_Position_X));
	memcpy(&Fill_Position_Y, &flash_on_RAM_many_times.Fill_Position_Y,sizeof(Fill_Position_Y));

	// In mảng ra giao diện debug
	print_arrayX(Fill_Position_X);
	print_arrayY(Fill_Position_Y);
	//
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			if (Fill_Position_X[i][j] == 255) {
				flag_positionXneed_Init = true; // cần khởi tạo giá trị cho X
			}
		}
	}
	if(flag_positionXneed_Init) // cần sinh tọa dộ mặc định vì không đọc được tọa độ
	{
		DEBUG_PRINT("Flash don't have postion Init Data");
		Init_position();
	}
	//Init_position();
	//ADDR_FLASH_SECTOR_9

}


// Hàm in mảng theo định dạng bảng
void print_arrayX(int array[12][12]) {
	DEBUG_PRINT("\nArray Fill_Position_X:");
	for (int i = 0; i < 12; i++) {
		DEBUG_PRINT("Hang %2d: ", i);
		for (int j = 0; j < 12; j++) {
			printf("%4d ", array[i][j]);  // In từng phần tử cách nhau 4 khoảng trống
		}
		printf("\n");  // Xuống dòng sau mỗi hàng
	}
	printf("\n");
}

// Hàm in mảng theo định dạng bảng
void print_arrayY(int array[12][8]) {
	DEBUG_PRINT("\nArray Fill_Position_Y:============================================");
	for (int i = 0; i < 12; i++) {
		//DEBUG_PRINT("Hang %2d: ", i);
		for (int j = 0; j < 8; j++) {
			printf("%4d ", array[i][j]);  // In từng phần tử cách nhau 4 khoảng trống
		}
		printf("\n");  // Xuống dòng sau mỗi hàng
	}
	printf("\n");
}


void verifyAndSaveData(uint8_t data[512])
{
	volatile int index = 2; // Bắt đầu từ Command_send[2]
	int value = 0;
	//  Fill_Position_X[col][row];
	// Lấy dữ liệu X từ  Command
	for (int col = 0; col < 12; ++col) {
		for (int row = 0; row < 12; ++row) {
			// Tách byte cao và thấp của Fill_Position_X[col][row]
			value = 0;
			value = (int)(data[index ] << 8) | data[index + 1];
			Fill_Position_X[col][row] = value;
			index = index + 2; // tăng lên 2 offset
		}
	}
	// Lấy dữ liệu Y từ  Command
	for (int col = 0; col < 12; col++) {
		for (int row = 0; row < 8; row++) {
			// Tách byte cao và thấp của Fill_Position_Y[col][row]
			value =0;
			value = (int)(data[index] << 8) | data[index + 1];
			Fill_Position_Y[col][row] = value;
			index = index + 2; // tăng lên 2 offset cho mỗi vị trí
		}
	}
	// task ghi data vào flash
	memcpy(&flash_on_RAM_factory_reset.Fill_Position_X, Fill_Position_X, sizeof(Fill_Position_X));
	memcpy(&flash_on_RAM_factory_reset.Fill_Position_Y, Fill_Position_Y, sizeof(Fill_Position_Y));
	memcpy(dataFlash_onRam.u8_dataInternalSector2,&flash_on_RAM_factory_reset,sizeof(flash_on_RAM_factory_reset_t));
	if(Flash_Write_Data_Byte(ADDR_FLASH_SECTOR_11, dataFlash_onRam.u8_dataInternalSector2, sizeof(flash_on_RAM_factory_reset_t)) == HAL_OK)
	{
		// ghi thành công
		DEBUG_PRINT(" Flash save thanh cong data");
	}
	print_arrayX(Fill_Position_X);
	print_arrayY(Fill_Position_Y);
}
