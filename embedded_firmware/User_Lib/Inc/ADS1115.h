/*
 * ADS1115.h
 *
 *  Created on: Dec 22, 2022
 *      Author: LeHoaiGiang
 *      modify 09/08/24 các hàm getvalue từ adc nếu giao tiếp lỗi thì thoát ra và báo lỗi
 */

#ifndef INC_ADS1115_H_
#define INC_ADS1115_H_
#include "main.h"
extern I2C_HandleTypeDef hi2c1;  // change your handler here accordingly
#define ADS1115_ADDRESS 0x48
typedef struct
{
	unsigned char ADSwrite[6];
	int16_t u16_ADC_Value;
	float f_Volt_ADC;
}ADS1115_ADC_t;


// Các giá trị cấu hình
#define ADS1115_CONVERSION_DELAY 8

// Enum cho các kênh ADC
typedef enum {
    ADS1115_CHANNEL_0 = 0xC1,
    ADS1115_CHANNEL_1 = 0xD1,
    ADS1115_CHANNEL_2 = 0xE1,
    ADS1115_CHANNEL_3 = 0xF1
} ADS1115_Channel_t;

// Cấu trúc để lưu trữ trạng thái ADC
typedef struct {
    I2C_HandleTypeDef *i2cHandle;
    uint8_t i2cAddress;
} ADS1115_Handle_t;

#define OFFSET_HUMIDITY_PAUSE_SYSTEM	8
HAL_StatusTypeDef ADS1115_Read_A0(float* Volt);
HAL_StatusTypeDef ADS1115_Read_A1(float* Volt);
float ADS1115_ReadADC_Single(I2C_HandleTypeDef* hi2c, uint8_t u8_CHANNEL_ADC);
void ADC_All();
#endif /* INC_ADS1115_H_ */
