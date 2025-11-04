/*
 * struct.h
 *
 *  Created on: Sep 16, 2022
 *      Author: LeHoaiGiang
 */

#ifndef INC_STRUCT_H_
#define INC_STRUCT_H_
#include "macro.h"
#include "stdbool.h"
#define SECTOR2_SIZE					1024
#define SECTOR3_SIZE                    1024
#define MAX_MIXED_COMPONENTS    4
typedef union
{
	uint16_t  Data;
	uint8_t Byte[2];
}TwoByte_to_u16;
typedef union
{
	uint32_t Data;
	uint8_t Byte[4];
}FourByte_to_u32;
typedef union
{
	float Data;
	char Byte[4];
}float_to_u8_t;
typedef struct
{
	float a;
	float b;
	float_to_u8_t f_a;
	float_to_u8_t f_b;
}Parameter_valve_t;// struct nay chua thong tin thoi gian bom hoa chat sau khi da calib
//=========================================
typedef struct
{
	uint8_t u8_Valve[MAX_NUMBER_VALVE];
	TwoByte_to_u16 u16tb_Time2Fill[MAX_NUMBER_VALVE];
}Valve_control_t;
typedef struct
{
	TwoByte_to_u16 u16tb_X_Distance;
	TwoByte_to_u16 u16tb_Y_Distance;
	TwoByte_to_u16 u16tb_Z1_Distance;
	TwoByte_to_u16 u16tb_Z2_Distance;
	uint8_t u8_Init_FW_Completed;
	uint8_t stateControlValve[MAX_NUMBER_VALVE];
	uint8_t stateControlSolenoid[8];
}Signal_control_t;
/***
 *
 */
typedef struct
{
	volatile uint8_t u8_Data_Rx_Buffer[UART_LENGTH_COMMAND_RX];
	volatile uint8_t u8_Data_Tx_Buffer[UART_LENGTH_COMMAND_TX];
	//volatile uint8_t u8_Data_sensor[UART_LENGTH_COMMAND];
	volatile bool b_FW_Rx_Command_Flag; // Flag = 1 have command from uart Flag = 0 Don't Have
}UART_Communication_t;
typedef struct
{
	volatile bool b_signal_update_status; // tin hieu nay true thi gui du lieu ve cho Software
	volatile bool b_signal_connect_software;
	volatile bool b_signal_runing_oligo;
	volatile bool b_signal_last_state_trityl;
	volatile uint16_t u16_counter_base_finished;
	volatile uint16_t u16_counter_step;

}signal_running_t;
typedef struct
{
	volatile uint16_t u16_counter_2second;
	volatile long u16_counter_30second;
	volatile bool  b_five_minute;
	volatile bool  b_one_minute;
	volatile bool  b_two_minute;
	volatile bool  b_auto_control_NITO;
	volatile bool  b_enable_clean_box;
}control_air_t;
//============================================================= STEP RUN OLIGO ====================================================
typedef struct
{
	char u8_type_chemical[3];
	TwoByte_to_u16 u16tb_Volume[3];
}mix_function_chemical_t;

typedef struct
{
	TwoByte_to_u16 u16tb_procs_time[10];
	TwoByte_to_u16 u16tb_waitting_after_time[10];
	uint8_t u8_option_pressure[10];
}control_pressure_t;
typedef struct
{
	char u8_first_type_chemical;
	mix_function_chemical_t mix_funtion;
	TwoByte_to_u16 u16tb_Volume;
	TwoByte_to_u16 u16tb_wait_after_fill;
}fill_chemical_t;

// Cấu trúc lưu thông tin của mỗi thành phần amidite tại từng well
typedef struct {
    uint8_t component[MAX_WELL_AMIDITE];    // Loại amidite (AMD_A, AMD_T,...)
    uint16_t volume[MAX_WELL_AMIDITE];       // Thể tích cần bơm
    uint8_t used_in_well[MAX_WELL_AMIDITE]; // Có được sử dụng tại well này không
} MixedComponent;

typedef struct
{
	// khai bao state 2 đên state  11
	bool b_douple_coupling_first_base;
	control_pressure_t control_pressure;
	fill_chemical_t fill_chemical;
	uint8_t u8_well_sequence[MAX_WELL_AMIDITE];
	uint8_t wellFirstsequence[MAX_WELL_AMIDITE]; //
	TwoByte_to_u16 u16_volume_func_mix_well[3]; // 17-04-24 thay doi so voi syno24 cho nay la volume - firmware tu tinh ra thoi gian bom
	uint8_t u8_enable_fill_buffer_N[MAX_WELL_AMIDITE];//  flag cho phép phun hóa chất đầy các cột còn trống
	TwoByte_to_u16 u16tb_volume_N_buffer;
	TwoByte_to_u16 u16_scale_volume;
	TwoByte_to_u16 u16_scale_time;
	bool isSpecialBase;
}Step_process_parameter_t;

//==================================================================
typedef struct
{
	uint8_t u8_valve_sellect; // sellect valve for calibration
	uint8_t valve[MAX_NUMBER_VALVE];
	FourByte_to_u32 u32fb_time_primming_calib;// sellect valve for calibration
	uint8_t u8_time_primming_control;
	bool b_custom_position;
}primming_control_t;

typedef struct
{
	TwoByte_to_u16  u16tb_humidity_Preset;
	bool flag_enable_auto_control_air_Nito;
	uint8_t u8_high_limit_humidity;
	uint8_t u8_low_limit_humidity;
	TwoByte_to_u16 u16tb_Pressure_sensor;
	TwoByte_to_u16 u16_temperature;
	TwoByte_to_u16 u16_humidity;
	TwoByte_to_u16 u16tb_X_Position;
	TwoByte_to_u16 u16tb_Y_Position;
	TwoByte_to_u16 u16tb_Z1_Position;
	double f_temperature;
	double f_humidity;
	float f_pressure_sensor;
	uint32_t Tcounter_error;
	uint32_t Hcounter_error;

}status_and_sensor_t;
typedef struct
{
	uint8_t u8_checked_well[MAX_WELL_AMIDITE];
	uint8_t u8_typeof_chemical;
	TwoByte_to_u16 u16_volume;
	uint8_t u8_option_pressure[4];
	TwoByte_to_u16 u16tb_procs_time[4];
	TwoByte_to_u16 u16tb_waitting_after_time[4];
	uint8_t U8_TASK_CONTROL;

}manual_run_t;
typedef struct  {
	bool EnableFillWellDone;
	uint8_t typeReagent;
	uint8_t En_WASH;
	uint8_t En_Deblock;
	uint8_t En_Coupling;
	uint8_t En_Cap;
	uint8_t En_Ox;
    TwoByte_to_u16 volumeWASH;
    TwoByte_to_u16 volumeDeblock;
    TwoByte_to_u16 volumeCoupling;
    TwoByte_to_u16 volumeCap;
    TwoByte_to_u16 volumeOx;
}FillChemistryDone_t; // tinh nang fill hoa chat vao cac cot bi trong


typedef struct  {
	bool Enablefeature;
	uint8_t En_WASH;
	uint8_t En_Deblock;
	uint8_t En_Coupling;
	uint8_t En_Cap;
	uint8_t En_Ox;
	TwoByte_to_u16 time;
}VacuumBox_t; // tinh nang fill hoa chat vao cac cot bi trong


typedef struct
{
	bool flag_autocheck_pressure; // cờ kiểm tra áp suất
	bool flag_auto_clean_box; // cờ xả hóa chất
	bool flag_auto_primming_chemical;
	bool flag_vacumm_waste;
	bool flag_exhaustFan; // Trạng thái quạt (bật/tắt)
	bool flag_speacial_volume_trityl;
	TwoByte_to_u16 volume_trityl_collection;
	TwoByte_to_u16 u16tb_autoPrim_volume_amidite;
	TwoByte_to_u16 u16tb_autoPrim_volume_Activator;
	TwoByte_to_u16 u16tb_time_auto_clean;
	TwoByte_to_u16 u16tb_timeExhaustFan;
	float f_pressure_setting;
	FillChemistryDone_t FillChemistryDone;
	VacuumBox_t VacuumBox;
}advanced_setting_t;
typedef struct
{
	uint16_t u16_volumme_sum ;
	uint16_t u16_volumme_avg ; // average value
}mixed_base_t;


typedef struct
{
	UART_Communication_t UART_Command;
	Signal_control_t signal_control;
	Valve_control_t Fill_control;
	control_air_t control_air;
	signal_running_t signal_running;
	Step_process_parameter_t synthetic_oligo;
	primming_control_t primming_control;
	status_and_sensor_t status_and_sensor;
	manual_run_t manual_run;
	advanced_setting_t advanced_setting;
	Parameter_valve_t valve_setting[MAX_NUMBER_VALVE];
	mixed_base_t mixed_base;
	// 07-07-2025 dinh nghia nay dung de trien khai con thuc amidite dung cho mixedbase
	// Mảng chứa tất cả các thành phần hỗn hợp tại 96 well
	MixedComponent mixed_sequence[MAX_MIXED_COMPONENTS];
	//Coupling2Setting_t Coupling2Setting;
	uint8_t sequenceWellDoneNeedAdd[MAX_WELL_AMIDITE]; // signal add well done
	uint8_t signal_kill[MAX_WELL_AMIDITE]; // true la bi kill. false la khong kill // gianglh22
}Global_var_t;

typedef struct {
	GPIO_TypeDef *Port;
	uint16_t Pin;
} GPIO_Config;

//******************************************************** struct Flash*******************************************************************************
typedef struct
{
	int Fill_Position_X[12][12];
	int Fill_Position_Y[12][8];
	int XYZ_POSITION_PUSH_DOWN[3];
	int XY_INSERT_PLATE[2];
	int X_OFSET_PRIMMING;
	int X_OFSET_CALBRATION;
}flash_on_RAM_factory_reset_t;

typedef struct
{
	int Fill_Position_X[12][12];
	int Fill_Position_Y[12][8];
	int XYZ_POSITION_PUSH_DOWN[3];
	int XY_INSERT_PLATE[2];
	int X_OFSET_PRIMMING;
	int X_OFSET_CALBRATION;

}flash_on_RAM_many_times_t;
typedef struct
{
	uint8_t	u8_dataInternalSector2[SECTOR2_SIZE];// sector write onetime - factory reset
	uint8_t	u8_dataInternalSector3[SECTOR3_SIZE];// sector write many time - factory reset
	uint8_t	u8_dataInternalSector4[SECTOR3_SIZE];// backup sector 3
}dataFlash_onRam_t;

#endif /* INC_STRUCT_H_ */
