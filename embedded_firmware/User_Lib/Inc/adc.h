/*
 * adc.h
 *
 *  Created on: Jun 23, 2023
 *      Author: LeHoaiGiang
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
float Round_t(float var);
float Convert_Volt2PressureValue(float f_Volt_input);
unsigned long kalman_filter(unsigned long ADC_Value);

#endif /* INC_ADC_H_ */
