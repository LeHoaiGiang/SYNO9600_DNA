/*
 * function.h
 *
 *  Created on: Dec 3, 2022
 *      Author: LeHoaiGiang
 */

#ifndef INC_FUNCTION_H_
#define INC_FUNCTION_H_
#include "main.h"
#include "struct.h"
//========================================== POSITION SYSTEM

#define Z_POSITION_NORMAL 330
#define Z_POSITION_CLEAN_AIR 330

#define Z_POSITION_FILL_CHEMICAL 330

#define X_POSITION_PUSH_DOWN  2660
#define Y_POSITION_PUSH_DOWN  1190
#define Z_POSITION_PUSH_DOWN  388

//#define	X_PRIMMING_POS_1			2072
//#define	X_PRIMMING_POS_2			2072
#define	X_PRIMMING_POS_1			(uint16_t)(X_POSITION_PUSH_DOWN - 528)
#define	X_PRIMMING_POS_2			(uint16_t)(X_POSITION_PUSH_DOWN - 528)
#define	Y_PRIMMING_POS				1200
#define Z_POSITION_PRIMMING 		330


//#define	X_CALIB_POS_1				((uint16_t)2340)
#define	Y_CALIB_POS					((uint16_t)1190)
#define Z_POSITION_CALIB 			320
#define MAX_COLUMN_X		12
#define MAX_ROW_Y			8

#define	X_INSERT_PLATE			2600
#define Y_INSERT_PLATE 			0

//#define OFFSET_X_PRIMMING 		580
//#define OFFSET_X_CALIBRATION    452
#define OFFSET_X_PRIMMING 		452
#define OFFSET_X_CALIBRATION    452
//extern const int Fill_Position_X[12][12];
extern const uint8_t VALVES_PER_TYPE[MAX_ORDINAL_VALVE];
extern const uint8_t VALVE_OFFSET_MAP[MAX_ORDINAL_VALVE];

typedef enum
{
	A1 = 0,
	A2 = 1,
	A3 = 2,
	A4 = 3,

	T1 = 4,
	T2 = 5,
	T3 = 6,
	T4 = 7,

	G1 = 8,
	G2 = 9,
	G3 = 10,
	G4 = 11,

	C1 = 12,
	C2 = 13,
	C3 = 14,
	C4 = 15,

	F1 = 16,
	F2 = 17,
	ACTI1 = 18,
	ACTI2 = 19,
	ACTI3 = 20,
	ACTI4 = 21,
	TCA1 = 22,
	TCA2 = 23,
	TCA3 = 24,
	TCA4 = 25,
	WASH1 = 26,
	WASH2 = 27,
	WASH3 = 28,
	WASH4 = 29,
	OX1 = 30,
	OX2= 31,
	OX3 = 32,
	OX4 = 33,
	CAPB1 = 34,
	CAPB2 = 35,
	CAPB3 = 36,
	CAPB4 = 37,
	CAPA1 = 38,
	CAPA2 = 39,
	CAPA3 = 40,
	CAPA4 = 41,
}VALVE__CHEMICAL;



// Cấu trúc lưu thông tin về một nhóm van
typedef struct {
    VALVE__CHEMICAL start_valve; // Van đầu tiên trong nhóm
    VALVE__CHEMICAL end_valve;   // Van cuối cùng trong nhóm
    uint8_t x_pos_index;         // Chỉ số trong mảng Fill_Position_X[][0]
} ValveGroupInfo;
int8_t get_x_index_for_valve(uint8_t valve_to_find) ;
void Calib_process(Global_var_t* p_global_variable);
void Valve_Set(uint8_t u8_idx_valve);
void Init_position();
void Chemical_fill_process(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_pos_X,  uint8_t u8_pos_Y);
void Valve_EnaAll();
void Valve_DisAll();
void Amidite_process(Global_var_t* p_global_variable, uint8_t u8_idx);
uint16_t valve_calculator_timefill(Global_var_t* global_variable, uint8_t type_sulphite, uint16_t u16_Volume);
uint16_t calculator_volume_avg(Global_var_t* global_variable, uint8_t u8_mixed_base_code, uint16_t u16_volumme_sum);
void Calib_volume(uint8_t type_sulphite, uint16_t u16_time_fill, uint16_t u8_pos_X,  uint16_t u8_pos_Y);
void buzzer_blink();
float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh);
void chemical_fill_gr1(uint8_t type, uint16_t u16_time_fill);
void chemical_fill_gr4(uint8_t type, uint16_t u16_time_fill, bool* signal_fill);
void Chemical_fill_process_gr1valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable);
void Chemical_fill_process_gr2valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_pos_X,  uint8_t u8_pos_Y);
void Chemical_fill_process_gr4valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y,Global_var_t* p_global_variable);
uint16_t calculator_timefill_gr4(Global_var_t* global_variable, uint8_t type_sulphite, uint8_t idx_valve, uint16_t u16_Volume);
void manual_fill_gr1(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable);
void manual_fill_gr4(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable);
void auto_primming( Global_var_t* p_global_variable, uint16_t volume, VALVE__CHEMICAL start_valve, VALVE__CHEMICAL end_valve);
void transferSquenceData(uint8_t *array_raw, uint8_t (*array_Data)[4]);
void process_well_sequences(Global_var_t* p_global_variable,
                            uint8_t u8_well_sequence[MAX_WELL_AMIDITE],
                            uint16_t default_volume);
void deactivate_all_valvesAmidite(void);
void execute_chemical_primming(Global_var_t* p_global_variable);
//-================================================= update new gianglh22 15-07-2025
uint16_t Valve_CalculateOpenTime(Global_var_t* global, ORDINAL_VALVE type, uint8_t valve_idx, uint16_t volume);
void open_valves(ORDINAL_VALVE type, bool signal_fill[4], int16_t u16_time_fill[4]);
void ManualFill_ChemicalGroup4(uint8_t chemicalType, uint16_t volume, uint8_t columnIndex, uint8_t rowIndex, Global_var_t* globalData);
#endif /* INC_FUNCTION_H_ */
