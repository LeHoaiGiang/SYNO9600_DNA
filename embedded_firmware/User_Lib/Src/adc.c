/*
 * adc.c
 *
 *  Created on: Jun 23, 2023
 *      Author: LeHoaiGiang
 */
#include "adc.h"



float Round_t(float var)
{
    // 37.66666 * 100 =3766.66
    // 3766.66 + .5 =3767.16    for rounding off value
    // then type cast to int so value is 3767
    // then divided by 100 so the value converted into 37.67
    float value = (int)(var * 100 + 0.5);
    return (float)value / 100;
}
/***
 * Chuyen doi gia tri Volt sang gia tri ap suat
 */
float Convert_Volt2PressureValue(float f_Volt_input)
{
	float Pressure_Value = f_Volt_input*3500 / 99 - 15995/495;
	return Round_t(Pressure_Value);
}
/*
 *
 */
//void GET_ADC_CH1(ADC_HandleTypeDef* hadc, uint32_t* ADC_Value, uint32_t* u32_ADC_after_Filter)
//{
//	HAL_ADC_Start(hadc);
//	HAL_ADC_PollForConversion(hadc, 500);
//	*ADC_Value = HAL_ADC_GetValue(hadc);
//	HAL_ADC_Stop(hadc);
//	*u32_ADC_after_Filter = kalman_filter((*ADC_Value));
//}
/**
 *
 */
unsigned long kalman_filter(unsigned long ADC_Value)
 {
     float x_k1_k1,x_k_k1;
     static float ADC_OLD_Value;
     float Z_k;
     static float P_k1_k1;

     static float Q = 0.0001;//Q: Regulation noise, Q increases, dynamic response becomes faster, and convergence stability becomes worse
     static float R = 0.005; //R: Test noise, R increases, dynamic response becomes slower, convergence stability becomes better
     static float Kg = 0;
     static float P_k_k1 = 1;

     float kalman_adc;
     static float kalman_adc_old=0;
     Z_k = ADC_Value;
     x_k1_k1 = kalman_adc_old;

     x_k_k1 = x_k1_k1;
     P_k_k1 = P_k1_k1 + Q;

     Kg = P_k_k1/(P_k_k1 + R);

     kalman_adc = x_k_k1 + Kg * (Z_k - kalman_adc_old);
     P_k1_k1 = (1 - Kg)*P_k_k1;
     P_k_k1 = P_k1_k1;

     ADC_OLD_Value = ADC_Value;
     kalman_adc_old = kalman_adc;

     return kalman_adc;
 }
