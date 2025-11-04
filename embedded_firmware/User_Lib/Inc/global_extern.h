/*
 * global_extern.h
 *
 *  Created on: Sep 2, 2024
 *      Author: LeHoaiGiang
 */

#ifndef GLOBAL_EXTERN_H_
#define GLOBAL_EXTERN_H_
#include "macro.h"
#include "stdbool.h"
#include "struct.h"
#include "function.h"
#include "gpio_timer.h"
extern GPIO_Config GPIO_Extend[];
//extern int Fill_Position_X[12][12];
extern dataFlash_onRam_t dataFlash_onRam;
extern flash_on_RAM_factory_reset_t flash_on_RAM_factory_reset;
extern int Fill_Position_X[12][12];
extern int Fill_Position_Y[12][8];
extern const ValveGroupInfo valve_groups[];
// Số lượng nhóm van trong mảng
extern const uint8_t num_valve_groups;
extern GPIO_Timer_Object FanInBox;

#endif /* GLOBAL_EXTERN_H_ */
