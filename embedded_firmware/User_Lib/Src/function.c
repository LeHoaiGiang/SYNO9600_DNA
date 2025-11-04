/*
 * function.c
 *
 *  Created on: Dec 3, 2022
 *      Author: LeHoaiGiang
 */
#include "function.h"
#include "global_extern.h"
#include "dwt_stm32_delay.h"
#include "struct.h"
#include "stepper.h"
#include "macro.h"
#define VALVE_COLS 4 // A, T, G, C
#define ROWS 4       // row 0 - 3
/**
 * Định nghĩa dùng để lấy số valve tồn tại của hóa chất
 */
const uint8_t VALVES_PER_TYPE[MAX_ORDINAL_VALVE] = {
		[A]              = 4,
		[T]              = 4,
		[G]              = 4,
		[C]              = 4,
		[I]              = 1,
		[U]              = 1,
		[Activator]      = 4,
		[TCA_in_DCM]     = 4,
		[WASH_ACN_DCM]   = 4,
		[OXIDATION_IODINE] = 4,
		[CAPPING_CAPB]   = 4,
		[CAPPING_CAPA]   = 4,
		[COUPLING]       = 0, // hoặc = 4 nếu có valve
		[FUNTION_MIXED]  = 0,
		[CAPPING]        = 0,
		[DMT_OFF]        = 0,
};

const uint8_t VALVE_OFFSET_MAP[MAX_ORDINAL_VALVE] = {
		[A]              = 0,   // Valve 0 - 3
		[T]              = 4,   // Valve 4 - 7
		[G]              = 8,   // Valve 8 - 11
		[C]              = 12,  // Valve 12 - 15
		[I]              = 16,  // Valve 16
		[U]              = 17,  // Valve 17
		[Activator]      = 18,  // Valve 18 - 21
		[TCA_in_DCM]     = 22,  // Valve 22 - 25
		[WASH_ACN_DCM]   = 26,  // Valve 26 - 29
		[OXIDATION_IODINE] = 30, // Valve 30 - 33
		[CAPPING_CAPB]   = 34,  // Valve 34 - 37
		[CAPPING_CAPA]   = 38,  // Valve 38 - 41
		[COUPLING]       = 45, // không có valve nào được mở
		[FUNTION_MIXED]  = 45,
		[CAPPING]        = 45,
		[DMT_OFF]        = 45,
};


//// Mảng chứa thông tin của tất cả các nhóm van
const ValveGroupInfo valve_groups[] = {
		{ A1,    A4,    0 }, //
		{ T1,    T4,    1 }, //
		{ G1,	 G4,  	2}, //
		{ C1, 	 C4,    3 }, //
		{ F1, 	 F1, 	4 },//
		{ F2, 	 F2, 	5 },
		{ ACTI1, ACTI4, 6 }, //
		{ TCA1,  TCA4,  7 }, //
		{ WASH1, WASH4, 8}, //
		{ OX1, 	 OX4,   9 }, //
		{ CAPB1, CAPB4, 10}, //
		{ CAPA1, CAPA4, 11 }, //

		// Thêm các nhóm khác ở đây nếu cần
};


// Số lượng nhóm van trong mảng
const uint8_t num_valve_groups = sizeof(valve_groups) / sizeof(valve_groups[0]);
uint8_t idx_debug =0;
#ifdef USING_SYNO96
GPIO_Config GPIO_Extend[] =
{
		{GPIOB, GPIO_PIN_13},//v1
		{GPIOB, GPIO_PIN_11},//v3
		{GPIOE, GPIO_PIN_15},//v5
		{GPIOE, GPIO_PIN_13},//v7
		{GPIOE, GPIO_PIN_11},//v9
		{GPIOE, GPIO_PIN_9},//v11
		{GPIOE, GPIO_PIN_7},//v13
		{GPIOG, GPIO_PIN_0},//v15
		{GPIOF, GPIO_PIN_14},//v17
		{GPIOF, GPIO_PIN_12},//v19

		{GPIOB, GPIO_PIN_14},//v2
		{GPIOB, GPIO_PIN_12},//v4
		{GPIOB, GPIO_PIN_10},//v6
		{GPIOE, GPIO_PIN_14},//v8
		{GPIOE, GPIO_PIN_12},//v10
		{GPIOE, GPIO_PIN_10},//v12
		{GPIOE, GPIO_PIN_8},//v14
		{GPIOG, GPIO_PIN_1},//v16
		{GPIOF, GPIO_PIN_15},//v18
		{GPIOF, GPIO_PIN_13}//v20
		//		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_13); //V20
		//		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_12); //V19
		//		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_14); // V17
		//		HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_15); // V18
		//		HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_0); // V15
		//		HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_1); // V16
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8); // V14
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7); // V13
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_9); // V11
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_10); // V12
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11); // V9
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12); // V10
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13); // V7
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_14); // V8
		//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_15); // V5
		//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_10); // V6
		//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_11); // V3
		//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_12); // V4
		//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_13); // V1
		//		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14); // V2
};

GPIO_Config GPIO_Extend2[] =
{

		{GPIOF, GPIO_PIN_4}, // v1
		{GPIOF, GPIO_PIN_6}, // v3
		{GPIOF, GPIO_PIN_8}, // v5
		{GPIOF, GPIO_PIN_10}, // v7
		{GPIOC, GPIO_PIN_1}, // v9 PH1 khong dung dc cho trung valve 10
		{GPIOC, GPIO_PIN_1}, // v11
		{GPIOC, GPIO_PIN_3}, // v13
		{GPIOC, GPIO_PIN_5}, // v15
		{GPIOB, GPIO_PIN_1}, // v17
		{GPIOF, GPIO_PIN_11}, // 19
		{GPIOF, GPIO_PIN_5}, // V2
		{GPIOF, GPIO_PIN_7}, // V4
		{GPIOF, GPIO_PIN_9},  // V6
		{GPIOC, GPIO_PIN_0},  // v8 không dùng được PH0
		{GPIOC, GPIO_PIN_0},  // v10 PC0
		{GPIOC, GPIO_PIN_2}, // v12 PC2
		{GPIOA, GPIO_PIN_0}, // v14
		{GPIOC, GPIO_PIN_4}, // v16
		{GPIOB, GPIO_PIN_0}, // v18
		{GPIOB, GPIO_PIN_2}, // v20

};
const GPIO_Config VALVE_AMIDITE[VALVE_COLS][ROWS] = {
		// chem_type = 0 (A) -> v0, v1, v2, v3
		{
				{GPIOB, GPIO_PIN_2}, // v1
				{GPIOB, GPIO_PIN_0}, // v3
				{GPIOC, GPIO_PIN_4}, // v5
				{GPIOA, GPIO_PIN_0}, // v7
		},

		// chem_type = 1 (T) -> v4, v5, v6, v7
		{
				{GPIOC, GPIO_PIN_2}, // v9
				{GPIOC, GPIO_PIN_0},  // v11
				{GPIOF, GPIO_PIN_9},  // V15
				{GPIOF, GPIO_PIN_7}, // V17
		},

		// chem_type = 2 (G) -> v8, v9, v10, v11
		{
				{GPIOF, GPIO_PIN_5}, // V19
				{GPIOF, GPIO_PIN_4}, // v20
				{GPIOF, GPIO_PIN_6}, // v18
				{GPIOF, GPIO_PIN_8}, // v16
		},

		// chem_type = 3 (C) -> v12, v13, v14, v15
		{
				{GPIOF, GPIO_PIN_10}, // v14
				//{GPIOC, GPIO_PIN_1}, // v12 PH1 khong dung dc cho trung valve 10
				{GPIOC, GPIO_PIN_1}, // v10
				{GPIOC, GPIO_PIN_3}, // v8
				{GPIOC, GPIO_PIN_5}, // v6
		}
};


const GPIO_Config VALVE_[MAX_NUMBER_VALVE] =
{
		/**
		 * AMD A
		 */
		{GPIOB, GPIO_PIN_2}, // v1
		{GPIOB, GPIO_PIN_0}, // v3
		{GPIOC, GPIO_PIN_4}, // v5
		{GPIOA, GPIO_PIN_0}, // v7
		/**
		 * AMD T
		 */
		{GPIOC, GPIO_PIN_2}, // v9
		{GPIOC, GPIO_PIN_0},  // v11
		{GPIOF, GPIO_PIN_9},  // V15
		{GPIOF, GPIO_PIN_7}, // V17
		/**
		 * AMD G
		 */
		{GPIOF, GPIO_PIN_5}, // V19
		{GPIOF, GPIO_PIN_4}, // v20
		{GPIOF, GPIO_PIN_6}, // v18
		{GPIOF, GPIO_PIN_8}, // v16

		/**
		 * AMD C
		 */
		{GPIOF, GPIO_PIN_10}, // v14
		//{GPIOC, GPIO_PIN_1}, // v12 PH1 khong dung dc cho trung valve 10
		{GPIOC, GPIO_PIN_1}, // v10
		{GPIOC, GPIO_PIN_3}, // v8
		{GPIOC, GPIO_PIN_5}, // v6

		/**
		 * AMD F1 F2
		 */
		{GPIOB, GPIO_PIN_1}, // v4
		{GPIOF, GPIO_PIN_11}, // v2


		/**
		 * ACTIVATOR
		 */
		{VALVE_1_GPIO_Port, VALVE_1_Pin},	// 6
		{VALVE_2_GPIO_Port, VALVE_2_Pin},	// 7
		{VALVE_3_GPIO_Port, VALVE_3_Pin},	// 8
		{VALVE_4_GPIO_Port, VALVE_4_Pin},	// 9
		/**
		 * TCA DEBLOCK
		 */
		{VALVE_5_GPIO_Port, VALVE_5_Pin}, // 10
		{VALVE_6_GPIO_Port, VALVE_6_Pin}, // 11
		{VALVE_7_GPIO_Port, VALVE_7_Pin}, // 12
		{VALVE_8_GPIO_Port, VALVE_8_Pin}, // 13
		/**
		 * WASH
		 */
		{VALVE_9_GPIO_Port, VALVE_9_Pin}, // 14
		{VALVE_10_GPIO_Port, VALVE_10_Pin}, //15
		{VALVE_11_GPIO_Port, VALVE_11_Pin}, //16
		{VALVE_12_GPIO_Port, VALVE_12_Pin}, //17
		/**
		 * OXIDATION
		 */
		{SOLENOID_1_GPIO_Port, SOLENOID_1_Pin}, // 18
		{SOLENOID_2_GPIO_Port, SOLENOID_2_Pin}, // 19
		//{SOLENOID_3_GPIO_Port, SOLENOID_3_Pin}, // 20
		{GPIOF, GPIO_PIN_13},//v20 chuyen tu V21 tren board support ==== tren board khong kich // 20
		{SOLENOID_4_GPIO_Port, SOLENOID_4_Pin}, // 21
		/**
		 * CAP B
		 */
		{CTRL_5_GPIO_Port, CTRL_5_Pin}, 	//	22
		{CTRL6_GPIO_Port, CTRL6_Pin}, 		//	23
		{CTRL7_GPIO_Port, CTRL7_Pin}, 		//	24
		{CTRL_8_GPIO_Port, CTRL_8_Pin},	 	//26
		/**
		 * CAP A
		 */
		{Vacuum_1_GPIO_Port, Vacuum_1_Pin},	//27
		{Vacuum_2_GPIO_Port, Vacuum_2_Pin},	//28
		{Vacuum_3_GPIO_Port, Vacuum_3_Pin}, // 29
		{Vacuum_4_GPIO_Port, Vacuum_4_Pin}, // 30
};
// toạ độ trước ngày 17/05/2024

int Fill_Position_X[12][12] = {
		{1800, 1710, 1620, 1530, 1440, 1350, 1260, 1170, 1080, 990, 900, 810},// A CỘT 2
		{1620, 1530, 1440, 1350, 1260, 1170, 1080, 990, 900, 810, 720, 630}, // T CỘT 0
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990,	900}, // G cột 3
		{1710,	1620,	1530,	1440, 	1350, 	1260, 	1170, 	1080, 	990, 	900, 	810, 	720}, // C cột 1

		{1620,  1530, 	1440, 	1350, 	1260, 	1170, 	1080, 	990, 	900, 	810, 	720, 	630}, // U CỘT 0
		{1620,  1530, 	1440, 	1350, 	1260, 	1170, 	1080, 	990, 	900, 	810, 	720, 	630},  // I CỘT 0


		{1710,	1620,	1530,	1440, 	1350, 	1260, 	1170, 	1080, 	990, 	900, 	810, 	720}, // ACT acitvator C cột 1
		{2070,	1980,	1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080}, // DEBLOCK TCA CỘT 5

		{2070,	1980,	1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080}, // WASH CỘT 5
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990,	900}, // OX CỘT 3

		{1980,	1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990}, // CAP B CỘT 4
		{1980,	1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990}, // CAP A CỘT 4
};

int Fill_Position_Y[12][8]= {
		// BLOCK 1
		// 28-05-2025 thay valve, bo khoanng trong giua cac valve
		// A T G C dang cach nhau 1 ô
		/*
		{570, 660, 750, 840, 930, 1020, 1110, 1200},  //A
		{750, 840, 930, 1020, 1110, 1200, 1290, 1380}, // T
		{930, 1020, 1110, 1200, 1290, 1380, 1470, 1560}, //G
		{1110, 1200, 1290, 1380, 1470, 1560, 1650, 1740}, //C
		 */
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, // AMD A
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, // AMD T
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, // AMD G
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, // AMD C

		// BLOCK 2
		{750, 840, 930, 1020, 1110, 1200 , 1290, 1380}, // U
		{840, 930, 1020, 1110, 1200 , 1290, 1380, 1470}, //I

		{1200, 1560, 1560, 1560, 1560, 1560, 1560, 1560},  // activator
		// BLOCK 5
		{1200, 1560, 1560, 1560, 1560, 1560, 1560, 1560}, // DEBLOCK
		// BLOCK 3
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, //WASH

		{1200, 1560, 1560, 1560, 1560, 1560, 1560, 1560}, // OX
		// BLOCK 4
		{840, 1200, 1200, 1200, 1200, 1200, 1200, 1200}, // CAP B
		{1200, 1560, 1560, 1560, 1560, 1560, 1560, 1560}, // CAP A
};

int Fill_Position_XAmidite[4][15] = {
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990	,900,	810,	720,	630},// A
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990	,900,	810,	720,	630},// A
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990	,900,	810,	720,	630},// A
		{1890,	1800,	1710,	1620,	1530,	1440,	1350,	1260,	1170,	1080,	990	,900,	810,	720,	630},// A
};

int Fill_Position_YAmidite[4][2]= {
		{840, 1200,},	//A
		{840, 1200,}, // T
		{840, 1200,}, //G
		{840, 1200,}, //C

};
int Calibration_Pos_XY[30][2] = {
		{1548,795},
		{1548,795},
		{1548,795},
		{1548,795},
		{1638,795},
		{1638,795},
		{1728,795},
		{1728,795},
		{1818,795},
		{1818,795},
		{1908,795},
		{1908,795},
};

const int idx_amidite_map_gr1[8][12]=
{
		{0,	8,	16,	24,	32,	40,	48,	56,	64,	72,	80,	88,},
		{1,	9,	17,	25,	33,	41,	49,	57,	65,	73,	81,	89,},
		{2, 10, 18, 26,	34,	42,	50,	58,	66,	74,	82,	90,},
		{3, 11 ,19,	27,	35,	43,	51,	59,	67,	75,	83,	91,},
		{4,	12,	20,	28,	36,	44,	52,	60,	68,	76,	84,	92,},
		{5,	13,	21,	29,	37,	45,	53,	61,	69,	77,	85,	93,},
		{6, 14,	22,	30,	38,	46,	54,	62,	70,	78,	86,	94,},
		{7,	15,	23,	31,	39,	47,	55,	63,	71,	79,	87,	95,},
};
#endif
/*
 *
 *  AMD_A = 0, // valve 1
    AMD_T = 1,
    AMD_G = 2,
    AMD_C = 3,
    AMD_I = 4,
    AMD_U = 5,
    AMD_Y = 6,
    AMD_R = 7,
    AMD_W = 8,
    AMD_S = 9,
    AMD_K = 10,
    AMD_M = 11,
    AMD_D = 12,
    AMD_V = 13,
    AMD_N = 14,
 */
const int MIXED_BASE[17][4]=
{
		{AMD_A,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY}, //0
		{AMD_T,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY},//1
		{AMD_G, CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY},//2
		{AMD_C, CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY},//3
		{AMD_I, CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY},//4
		{AMD_U, CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY,CHEMICAL_SUBTANCE_EMPTY},//5
		{AMD_C, AMD_T, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//6 AMD_Y
		{AMD_G, AMD_A, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//7 AMD_R
		{AMD_A, AMD_T, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//8 AMD_W
		{AMD_G, AMD_C, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//9 AMD_S
		{AMD_G, AMD_T, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//10 AMD_K
		{AMD_A, AMD_C, CHEMICAL_SUBTANCE_EMPTY, CHEMICAL_SUBTANCE_EMPTY},//11 AMD_M
		{AMD_G, AMD_A, AMD_T, CHEMICAL_SUBTANCE_EMPTY},//12 AMD_D
		{AMD_G, AMD_A, AMD_C, CHEMICAL_SUBTANCE_EMPTY},//13 AMD_V
		{AMD_G, AMD_A, AMD_C, AMD_T},//14 AMD_N OR AMD_X
		{AMD_A, AMD_C, AMD_T, CHEMICAL_SUBTANCE_EMPTY},//15 AMD_H
		{AMD_G, AMD_C, AMD_T, CHEMICAL_SUBTANCE_EMPTY},//16 AMD_B
};

void Valve_Set(uint8_t u8_idx_valve)
{
	HAL_GPIO_WritePin(VALVE_[u8_idx_valve].Port, VALVE_[u8_idx_valve].Pin, SET);
}
#define ROWS 4
#define VALVE_COLS 4
#define WELL_COLS 12
int sequence1[ROWS][WELL_COLS];
int sequence2[ROWS][WELL_COLS];
//================================================== INIT VALUE POSTION
void Init_position()
{
	int values[12][12] = {
			{1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850, 760},
			{1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850, 760},
			{1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850, 760},
			{1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850, 760},
			{1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850},//4
			{1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940, 850},//5
			{1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940},
			{1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030, 940},
			{2020, 1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030},
			{2020, 1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120, 1030},
			{2110, 2020, 1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120},
			{2110, 2020, 1930, 1840, 1750, 1660, 1570, 1480, 1390, 1300, 1210, 1120}
	};
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 12; j++) {
			Fill_Position_X[i][j] = values[i][j];
		}
	}
	int valuesY[12][8] = {
			{560, 650, 740, 830, 920, 1010, 1100, 1190},
			{740, 830, 920, 1010, 1100, 1190, 1280, 1370},
			{920, 1010, 1100, 1190, 1280, 1370, 1460, 1550},
			{1100, 1190, 1280, 1370, 1460, 1550, 1640, 1730},
			{560, 650, 740, 830, 920, 1010, 1100, 1190},
			{650, 740, 830, 920, 1010, 1100, 1190, 1280},
			{830, 1190, 1190, 1190, 1190, 1190, 1190, 1190},
			{1190, 1550, 1550, 1550, 1550, 1550, 1550, 1550},
			{830, 1190, 1190, 1190, 1190, 1190, 1190, 1190},
			{1190, 1550, 1550, 1550, 1550, 1550, 1550, 1550},
			{830, 1190, 1190, 1190, 1190, 1190, 1190, 1190},
			{1190, 1550, 1550, 1550, 1550, 1550, 1550, 1550}
	};

	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 8; j++) {
			Fill_Position_Y[i][j] = valuesY[i][j];
		}
	}
}
//**********************************************************************************************************
/*
 * uint8_t sulphite ---- loai hoa chat 0-10 -- tổng cộng có 11 loại hóa chất
 * u8_pos_X max vi tri x la 3
 * u8_pos_Y max vi tri y la 8
 * xu ly fill hoa chat STEP
 */
void Chemical_fill_process_gr1valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static int u8_idx_well;
	static uint16_t u16_time_fill;
	u8_idx_well = idx_amidite_map_gr1[u8_row_y][u8_column_x];
	if(p_global_variable->signal_kill[u8_idx_well] == true)
	{
		return;
	}
	if(u16_volume != 0)
	{
		if(p_global_variable->synthetic_oligo.u8_well_sequence[u8_idx_well] != CHEMICAL_SUBTANCE_EMPTY )
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			//Stepper_Z1_Run2Normal();
			Stepper_move_Coordinates_XY(pos_x, pos_y);
			Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			u16_time_fill = valve_calculator_timefill(p_global_variable, type_sulphite, u16_volume);
			chemical_fill_gr1(type_sulphite, u16_time_fill);

		}
	}
}


void Chemical_fill_process_gr2valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_pos_X,  uint8_t u8_pos_Y)
{
	static int pos_x;
	static int pos_y;
	if(u16_volume != 0)
	{
		pos_x = Fill_Position_X[type_sulphite][u8_pos_X];
		pos_y = Fill_Position_Y[type_sulphite][u8_pos_Y];
		//Stepper_Z1_Normal();
		Stepper_move_Coordinates_XY(pos_x, pos_y);
		Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
		//Chemical_substance(type_sulphite, u16_volume);
	}
}

void Chemical_fill_process_gr4valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static bool signal_fill[4];
	static uint8_t idx =0;
	static int u16_time_fill[4];
	static int u8_idx_well = 0;
	//u8_idx_well = idx_amidite_map_gr1[u8_row_y][u8_column_x];
	for(idx = 0; idx < 4 ; idx++)
	{
		// kiem tra xem co kill khong. mo comment xem debug
		if(p_global_variable->signal_kill[u8_row_y*4 + u8_column_x * 8 + idx] == true)
		{
			u8_idx_well = u8_row_y*4 + u8_column_x * 8 + idx;
		}
		if(p_global_variable->synthetic_oligo.u8_well_sequence[u8_row_y * 4 + u8_column_x * 8 + idx] != CHEMICAL_SUBTANCE_EMPTY && p_global_variable->signal_kill[u8_row_y*4 + u8_column_x * 8 + idx] == false)
		{
			signal_fill[idx] = true;
		}
		else
		{
			signal_fill[idx] = false;
		}
	}
	if(signal_fill[0] == true || signal_fill[1] == true || signal_fill[2] == true ||signal_fill[3] == true)
	{

		//u16_time_fill = calculator_timefill_gr4(p_global_variable, type_sulphite, u16_volume);
		//calculator_timefill_gr4();
		//chemical_fill_gr4(type_sulphite, u16_time_fill, &signal_fill[0], p_global_variable);
		if(u16_volume != 0)
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			//Stepper_Z1_Run2Normal();
			Stepper_move_Coordinates_XY(pos_x, pos_y);
			Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			switch(type_sulphite)
			{
			case Activator:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
				break;
			}
			case TCA_in_DCM:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
				break;
			}
			case WASH_ACN_DCM:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
				break;
			}
			case OXIDATION_IODINE:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
				break;
			}

			case CAPPING_CAPB:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
				break;
			}
			case CAPPING_CAPA:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
				break;
			}
			default:
				break;
			}

		}
	}
}


/*
 * Le Hoai Giang
 *  chemical_fill_gr1
 *  thuc hien bom hoa chat, khong nhan vao vi tri, phai tu tinh vi tri truoc khi bom
 *
 */

void chemical_fill_gr1(uint8_t type, uint16_t u16_time_fill)
{
	if(type <= 5) // A = 0 -> FLOAT2 = 5
	{
		HAL_GPIO_WritePin(VALVE_[type].Port, VALVE_[type].Pin, SET);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[type].Port, VALVE_[type].Pin, RESET);
	}
}

/*
 * Le Hoai Giang
 *  chemical_fill_gr4
 *  thuc hien bom hoa chat, khong nhan vao vi tri, phai tu tinh vi tri truoc khi bom
 */

void chemical_fill_gr4(uint8_t type, uint16_t u16_time_fill, bool* signal_fill)
{
	switch(type)
	{
	case Activator:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI1].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI1].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
		break;
	}
	case TCA_in_DCM:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
		break;
	}
	case WASH_ACN_DCM:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
		break;
	}
	case OXIDATION_IODINE:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
		break;
	}

	case CAPPING_CAPB:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
		break;
	}
	case CAPPING_CAPA:
	{
		if(signal_fill[0] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, SET);
		}
		if(signal_fill[1] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, SET);
		}
		if(signal_fill[2] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, SET);
		}
		if(signal_fill[3] == true)
		{
			HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, SET);
		}
		//HAL_Delay(u16_volume);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, RESET);
		HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
		break;
	}
	default:
		break;
	}
}



//**********************************************************************************************************
/*
 * uint8_t sulphite ---- loai hoa chat 0-10 -- tổng cộng có 11 loại hóa chất
 * u8_pos_X max vi tri x la 3
 * u8_pos_Y max vi tri y la 8
 */
void manual_fill_gr1(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static int u8_idx_well;
	u8_idx_well = idx_amidite_map_gr1[u8_row_y][u8_column_x];
	static uint16_t u16_time_fill;
	if(u16_volume != 0)
	{
		if(p_global_variable->manual_run.u8_checked_well[u8_idx_well] == true)
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			Stepper_move_Coordinates_XY(pos_x, pos_y);

			//Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			u16_time_fill = valve_calculator_timefill(p_global_variable, type_sulphite, u16_volume);
			chemical_fill_gr1(type_sulphite, u16_time_fill);
		}
	}
}

//**********************************************************************************************************
/*
 * uint8_t sulphite ---- loai hoa chat 0-10 -- tổng cộng có 11 loại hóa chất
 * u8_pos_X max vi tri x la 3
 * u8_pos_Y max vi tri y la 8
 */

void manual_fill_gr4(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static bool signal_fill[4];
	static idx = 0;
	int16_t u16_time_fill[4];
	for(idx = 0; idx < 4 ; idx++)
	{
		if(p_global_variable->manual_run.u8_checked_well[u8_row_y*4 + u8_column_x * 8 + idx] == true)
		{
			signal_fill[idx] = true;
		}
		else
		{
			signal_fill[idx] = false;
		}
	}
	if(signal_fill[0] == true || signal_fill[1] == true || signal_fill[2] == true ||signal_fill[3] == true)
	{
		if(u16_volume != 0)
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			//Stepper_Z1_Run2Normal();
			Stepper_move_Coordinates_XY(pos_x, pos_y);
			//Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			switch(type_sulphite)
			{
			case A:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, RESET);
				break;
			}
			case T:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T2].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, RESET);
				break;
			}
			case G:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, RESET);
				break;
			}
			case C:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, RESET);
				break;
			}
			case Activator:
			{
				// 17-10 fix lay sai thong so valve, ACT dao nguoc thu tu nen phai dao nguoc cach tinh
				// kiem tra lai
				//16-11-2024 da sua xong activator = dao vi tri tinh toan cac thong so Ok
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
				break;
			}
			case TCA_in_DCM:
			{
				//16-11-2024 da sua xong TCA_in_DCM = dao vi tri tinh toan cac thong so Ok
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
				break;
			}
			case WASH_ACN_DCM:
			{
				//16-11-2024 da sua xong WASH_ACN_DCM = dao vi tri tinh toan cac thong so Ok
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
				break;
			}
			case OXIDATION_IODINE:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
				break;
			}

			case CAPPING_CAPB:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
				break;
			}
			case CAPPING_CAPA:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
					}
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
				break;
			}
			default:
				break;
			}
		}
	}
}

/***
 * GiangLH22
 * ChemicalFillProcessWellDone_gr1valve
 * Find and fill chemical to well done - group only one valve
 */
void ChemicalFillProcessWellDone_gr1valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static int u8_idx_well;
	static uint16_t u16_time_fill;
	u8_idx_well = idx_amidite_map_gr1[u8_row_y][u8_column_x];
	if(p_global_variable->signal_kill[u8_idx_well] == true)
	{
		return;
	}
	if(u16_volume != 0)
	{
		if(p_global_variable->sequenceWellDoneNeedAdd[u8_idx_well] != CHEMICAL_SUBTANCE_EMPTY )
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			//Stepper_Z1_Run2Normal();
			Stepper_move_Coordinates_XY(pos_x, pos_y);
			Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			u16_time_fill = valve_calculator_timefill(p_global_variable, type_sulphite, u16_volume);
			chemical_fill_gr1(type_sulphite, u16_time_fill);

		}
	}
}
/***
 * GiangLH22 - 14-06-2025
 * ChemicalFillProcessWellDone_gr4valve
 * Find and fill chemical to well done - group only four valve
 */
void ChemicalFillProcessWellDone_gr4valve(uint8_t type_sulphite, uint16_t u16_volume, uint8_t u8_column_x,  uint8_t u8_row_y, Global_var_t* p_global_variable)
{
	static int pos_x;
	static int pos_y;
	static bool signal_fill[4];
	static uint8_t idx =0;
	static int u16_time_fill[4];
	static int u8_idx_well = 0;
	//u8_idx_well = idx_amidite_map_gr1[u8_row_y][u8_column_x];
	for(idx = 0; idx < 4 ; idx++)
	{
		// kiem tra xem co kill khong. mo comment xem debug
		if(p_global_variable->signal_kill[u8_row_y*4 + u8_column_x * 8 + idx] == true)
		{
			u8_idx_well = u8_row_y*4 + u8_column_x * 8 + idx;
		}
		if(p_global_variable->sequenceWellDoneNeedAdd[u8_row_y * 4 + u8_column_x * 8 + idx] != CHEMICAL_SUBTANCE_EMPTY)
		{
			signal_fill[idx] = true;
		}
		else
		{
			signal_fill[idx] = false;
		}
	}
	if(signal_fill[0] == true || signal_fill[1] == true || signal_fill[2] == true ||signal_fill[3] == true)
	{

		//u16_time_fill = calculator_timefill_gr4(p_global_variable, type_sulphite, u16_volume);
		//calculator_timefill_gr4();
		//chemical_fill_gr4(type_sulphite, u16_time_fill, &signal_fill[0], p_global_variable);
		if(u16_volume != 0)
		{
			pos_x = Fill_Position_X[type_sulphite][u8_column_x];
			pos_y = Fill_Position_Y[type_sulphite][u8_row_y];
			//Stepper_Z1_Run2Normal();
			Stepper_move_Coordinates_XY(pos_x, pos_y);
			Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
			switch(type_sulphite)
			{
			case A:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[A1].Port, VALVE_[A1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A2].Port, VALVE_[A2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A3].Port, VALVE_[A3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[A4].Port, VALVE_[A4].Pin, RESET);
				break;
			}
			case T:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T2].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[T1].Port, VALVE_[T1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T2].Port, VALVE_[T2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T3].Port, VALVE_[T3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[T4].Port, VALVE_[T4].Pin, RESET);
				break;
			}
			case G:
			{

				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[G1].Port, VALVE_[G1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G2].Port, VALVE_[G2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G3].Port, VALVE_[G3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[G4].Port, VALVE_[G4].Pin, RESET);
				break;
			}
			case C:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3, u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2, u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1, u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0, u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[C1].Port, VALVE_[C1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C2].Port, VALVE_[C2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C3].Port, VALVE_[C3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[C4].Port, VALVE_[C4].Pin, RESET);
				break;
			}
			case Activator:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[ACTI1].Port, VALVE_[ACTI1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI2].Port, VALVE_[ACTI2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI3].Port, VALVE_[ACTI3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[ACTI4].Port, VALVE_[ACTI4].Pin, RESET);
				break;
			}
			case TCA_in_DCM:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[TCA1].Port, VALVE_[TCA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA2].Port, VALVE_[TCA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA3].Port, VALVE_[TCA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[TCA4].Port, VALVE_[TCA4].Pin, RESET);
				break;
			}
			case WASH_ACN_DCM:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[WASH1].Port, VALVE_[WASH1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH2].Port, VALVE_[WASH2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH3].Port, VALVE_[WASH3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[WASH4].Port, VALVE_[WASH4].Pin, RESET);
				break;
			}
			case OXIDATION_IODINE:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[OX1].Port, VALVE_[OX1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX2].Port, VALVE_[OX2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[OX4].Port, VALVE_[OX4].Pin, RESET);
				break;
			}

			case CAPPING_CAPB:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[CAPB1].Port, VALVE_[CAPB1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB2].Port, VALVE_[CAPB2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB3].Port, VALVE_[CAPB3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPB4].Port, VALVE_[CAPB4].Pin, RESET);
				break;
			}
			case CAPPING_CAPA:
			{
				u16_time_fill[0] = calculator_timefill_gr4(p_global_variable, type_sulphite, 3,u16_volume);
				u16_time_fill[1] = calculator_timefill_gr4(p_global_variable, type_sulphite, 2,u16_volume);
				u16_time_fill[2] = calculator_timefill_gr4(p_global_variable, type_sulphite, 1,u16_volume);
				u16_time_fill[3] = calculator_timefill_gr4(p_global_variable, type_sulphite, 0,u16_volume);
				if(signal_fill[3] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, SET);
				}
				if(signal_fill[2] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, SET);
				}
				if(signal_fill[1] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, SET);
				}
				if(signal_fill[0] == true)
				{
					HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, SET);
				}
				while (u16_time_fill[0]> 0 || u16_time_fill[1] >0 || u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
				{
					u16_time_fill[0]--;
					u16_time_fill[1]--;
					u16_time_fill[2]--;
					u16_time_fill[3]--;
					if(u16_time_fill[3] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
					}
					if(u16_time_fill[2] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
					}
					if(u16_time_fill[1] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[OX3].Port, VALVE_[OX3].Pin, RESET);
					}
					if(u16_time_fill[0] <= 0)
					{
						HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
					}
					HAL_Delay(1);
				}
				//HAL_Delay(u16_volume);
				HAL_GPIO_WritePin(VALVE_[CAPA1].Port, VALVE_[CAPA1].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA2].Port, VALVE_[CAPA2].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA3].Port, VALVE_[CAPA3].Pin, RESET);
				HAL_GPIO_WritePin(VALVE_[CAPA4].Port, VALVE_[CAPA4].Pin, RESET);
				break;
			}
			default:
				break;
			}

		}
	}
}

//===============================================================================================
/* amidtie process cu truoc ngay 28-05-2025
 *doi cach di chuyen
 */
/*
void Amidite_process(Global_var_t* p_global_variable, uint8_t u8_idx)
{
	static int position_X;
	static int position_Y;
	static uint8_t chemical;
	uint8_t chemical_reality;
	uint16_t u16_volume;
	uint16_t u16_time_fill;
	//uint8_t u8_counter_formula;
	for(uint8_t column_x = 0; column_x < MAX_COLUMN_X; column_x++ )
	{
		for(uint8_t row_y = 0; row_y < MAX_ROW_Y; row_y++ )
		{
			chemical = p_global_variable->synthetic_oligo.u8_well_sequence[column_x * 8 + row_y];
			// get time fill chemical to well
			if(u8_idx == 0)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
			}
			if(u8_idx == 1)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
			}
			if(u8_idx == 2)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
			}
			calculator_volume_avg(p_global_variable , chemical, u16_volume);
			// process amidite mixed base
			if(chemical != CHEMICAL_SUBTANCE_EMPTY)
			{
				for(uint8_t u8_mix_idx = 0; u8_mix_idx < 4; u8_mix_idx++)
				{
					// chuyen doi sang hoa chat that = truy cap mang du lieu
					chemical_reality = MIXED_BASE[chemical][u8_mix_idx];
					// tinh toan do can di chuyen
					if(chemical_reality != CHEMICAL_SUBTANCE_EMPTY)
					{
						position_X = Fill_Position_X[chemical_reality][column_x];
						position_Y = Fill_Position_Y[chemical_reality][row_y];
						// tinh thoi gian bomm
						u16_time_fill = valve_calculator_timefill(p_global_variable, chemical_reality, p_global_variable->mixed_base.u16_volumme_avg);
						//Stepper_Z1_Run2Normal();
						Stepper_move_Coordinates_XY(position_X, position_Y);
						Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
						HAL_Delay(20);// cũ 50
						chemical_fill_gr1(chemical_reality, u16_time_fill);
						// 04-06-2024 them delay cho amidite khi chay tranh  vang hoa chat
						// 17/07 Dung lau qua, khong dung nua
						// HAL_Delay(100);
					}
				}
			} // chemical != CHEMICAL_SUBTANCE_EMPTY
		} // chay well tren 1 hang
	}// chay
}
 */
/* 28-05-2025 doi cach di chuyen ziczac
 * giam thoi gian
 * 26-06-2025 thay doi cach di chuyen tu gieng 96 -> 1
 * */

void Amidite_process(Global_var_t* p_global_variable, uint8_t u8_idx)
{
	static int position_X;
	static int position_Y;
	static uint8_t u8_position_y;
	static uint8_t chemical;
	uint8_t chemical_reality;
	uint16_t u16_volume;
	uint16_t u16_time_fill;
	//uint8_t u8_counter_formula;
	for(uint8_t column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // truoc ngay 26-06-2025
		//for(int8_t column_x = MAX_COLUMN_X - 1; column_x >= 0; column_x-- ) // thay doi huong di ngay 26-06-2025
	{
		if(column_x % 2 == 0)
		{
			for(uint8_t row_y = 0; row_y < MAX_ROW_Y; row_y++ )
			{
				chemical = p_global_variable->synthetic_oligo.u8_well_sequence[column_x * 8 + row_y];
				// get time fill chemical to well
				if(u8_idx == 0)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
				}
				if(u8_idx == 1)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
				}
				if(u8_idx == 2)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
				}
				calculator_volume_avg(p_global_variable , chemical, u16_volume);
				// process amidite mixed base
				if(chemical != CHEMICAL_SUBTANCE_EMPTY)
				{
					for(uint8_t u8_mix_idx = 0; u8_mix_idx < 4; u8_mix_idx++)
					{
						// chuyen doi sang hoa chat that = truy cap mang du lieu
						chemical_reality = MIXED_BASE[chemical][u8_mix_idx];
						// tinh toan do can di chuyen
						if(chemical_reality != CHEMICAL_SUBTANCE_EMPTY)
						{
							position_X = Fill_Position_X[chemical_reality][column_x];
							position_Y = Fill_Position_Y[chemical_reality][row_y];
							// tinh thoi gian bomm
							u16_time_fill = valve_calculator_timefill(p_global_variable, chemical_reality, p_global_variable->mixed_base.u16_volumme_avg);
							//Stepper_Z1_Run2Normal();
							Stepper_move_Coordinates_XY(position_X, position_Y);
							Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
							HAL_Delay(20);// cũ 50
							chemical_fill_gr1(chemical_reality, u16_time_fill);
							// 04-06-2024 them delay cho amidite khi chay tranh  vang hoa chat
							// 17/07 Dung lau qua, khong dung nua
							// HAL_Delay(100);
						}
					}
				} // chemical != CHEMICAL_SUBTANCE_EMPTY
			} // chay well tren 1 hang
		}
		else
		{
			for(int8_t row_y = 7; row_y >= 0; row_y-- )
			{
				// = 7 - row_y;
				chemical = p_global_variable->synthetic_oligo.u8_well_sequence[column_x * 8 + row_y];
				// get time fill chemical to well
				if(u8_idx == 0)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
				}
				if(u8_idx == 1)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
				}
				if(u8_idx == 2)
				{
					u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
				}
				calculator_volume_avg(p_global_variable , chemical, u16_volume);
				// process amidite mixed base
				if(chemical != CHEMICAL_SUBTANCE_EMPTY)
				{
					for(uint8_t u8_mix_idx = 0; u8_mix_idx < 4; u8_mix_idx++)
					{
						// chuyen doi sang hoa chat that = truy cap mang du lieu
						chemical_reality = MIXED_BASE[chemical][u8_mix_idx];
						// tinh toan do can di chuyen
						if(chemical_reality != CHEMICAL_SUBTANCE_EMPTY)
						{
							position_X = Fill_Position_X[chemical_reality][column_x];
							position_Y = Fill_Position_Y[chemical_reality][row_y];
							// tinh thoi gian bomm
							u16_time_fill = valve_calculator_timefill(p_global_variable, chemical_reality, p_global_variable->mixed_base.u16_volumme_avg);
							//Stepper_Z1_Run2Normal();
							Stepper_move_Coordinates_XY(position_X, position_Y);
							Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
							HAL_Delay(20);// cũ 50
							chemical_fill_gr1(chemical_reality, u16_time_fill);
							// 04-06-2024 them delay cho amidite khi chay tranh  vang hoa chat
							// 17/07 Dung lau qua, khong dung nua
							// HAL_Delay(100);
						}
					}
				} // chemical != CHEMICAL_SUBTANCE_EMPTY
			} // chay well tren 1 hang
		}

	}// chay
	// open for pause debug
	//	if(p_global_variable->signal_running.u16_counter_base_finished == 1)
	//	{
	//		//Stepper_Z1_Run2Normal();
	//		Stepper_Z1_move(Z_POSITION_NORMAL);
	//		Stepper_AutoHome_SYN024();
	//		buzzer_blink();
	//	}
	//
	//	if(p_global_variable->signal_running.u16_counter_base_finished == 4)
	//	{
	//		//Stepper_Z1_Run2Normal();
	//		Stepper_Z1_move(Z_POSITION_NORMAL);
	//		Stepper_AutoHome_SYN024();
	//		buzzer_blink();
	//	}

}

/*
void Amidite_process(Global_var_t* p_global_variable, uint8_t u8_idx)
{
	static int position_X;
	static int position_Y;
	static uint8_t chemical;
	uint8_t chemical_reality;
	uint16_t u16_volume;
	uint16_t u16_time_fill;
	//uint8_t u8_counter_formula;
	for(uint8_t row_y = 0; row_y < MAX_ROW_Y; row_y++ )
	{
		for(uint8_t column_x = 0; column_x < MAX_COLUMN_X; column_x++ )
		{
			chemical = p_global_variable->synthetic_oligo.u8_well_sequence[column_x * 8 + row_y];
			// get time fill chemical to well
			if(u8_idx == 0)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
			}
			if(u8_idx == 1)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
			}
			if(u8_idx == 2)
			{
				u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
			}
			calculator_volume_avg(p_global_variable , chemical, u16_volume);
			// process amidite mixed base
			if(chemical != CHEMICAL_SUBTANCE_EMPTY)
			{
				for(uint8_t u8_mix_idx = 0; u8_mix_idx < 4; u8_mix_idx++)
				{
					// chuyen doi sang hoa chat that = truy cap mang du lieu
					chemical_reality = MIXED_BASE[chemical][u8_mix_idx];
					// tinh toan do can di chuyen
					if(chemical_reality != CHEMICAL_SUBTANCE_EMPTY)
					{
						position_X = Fill_Position_X[chemical_reality][column_x];
						position_Y = Fill_Position_Y[chemical_reality][row_y];
						// tinh thoi gian bomm
						u16_time_fill = valve_calculator_timefill(p_global_variable, chemical_reality, p_global_variable->mixed_base.u16_volumme_avg);
						//Stepper_Z1_Run2Normal();
						Stepper_move_Coordinates_XY(position_X, position_Y);
						Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
						HAL_Delay(5);
						chemical_fill_gr1(chemical_reality, u16_time_fill);
						HAL_Delay(75);
						// 04-06-2024 them delay cho amidite khi chay tranh  vang hoa chat
						// 17/07 Dung lau qua, khong dung nua
					}
				}
			} // chemical != CHEMICAL_SUBTANCE_EMPTY
		} // chay well tren 1 hang
	}// chay
}
 */

/*
 * Le Hoai Giang
 * Xu ly fill chemical cho cac cot da ket thuc sequence tai cac step
 * cho phep chon step va chon hoa chat fill
 */

void FillChemistryWellDone(Global_var_t* p_global_variable)
{
	static int position_X;
	static int position_Y;
	static uint8_t chemical;
	static uint8_t column_x = 0;
	static uint8_t row_y = 0;
	static uint8_t u8_position_x =0;
	uint8_t chemical_reality;//
	uint16_t u16_volume;
	uint16_t u16_time_fill;
	uint8_t signalFillEnable = false;
	static uint16_t u8_volume_temp;
	uint8_t sequenceWellDoneNeedAdd[24][4] = {false};         // Mảng đích
	column_x = 0;
	row_y = 0;
	u8_position_x = 0;
	if(p_global_variable->advanced_setting.FillChemistryDone.EnableFillWellDone)
	{
		if(p_global_variable->advanced_setting.FillChemistryDone.En_Deblock == true &&
				p_global_variable->synthetic_oligo.fill_chemical.u8_first_type_chemical == TCA_in_DCM)
		{
			signalFillEnable = true;
		}
		if(p_global_variable->advanced_setting.FillChemistryDone.En_Coupling == true
				&& p_global_variable->synthetic_oligo.fill_chemical.u8_first_type_chemical == COUPLING)
		{
			signalFillEnable = true;
		}
		if(p_global_variable->advanced_setting.FillChemistryDone.En_Cap == true &&
				p_global_variable->synthetic_oligo.fill_chemical.u8_first_type_chemical == CAPPING)
		{
			signalFillEnable = true;
		}
		if(p_global_variable->advanced_setting.FillChemistryDone.En_Ox == true &&
				p_global_variable->synthetic_oligo.fill_chemical.u8_first_type_chemical == OXIDATION_IODINE)
		{
			signalFillEnable = true;
		}
		if(p_global_variable->advanced_setting.FillChemistryDone.En_WASH == true &&
				p_global_variable->synthetic_oligo.fill_chemical.u8_first_type_chemical == WASH_ACN_DCM)
		{
			signalFillEnable = true;
		}
		if(signalFillEnable)
		{
			// chay kiem tra cot nao can add hoa chat
			for(uint8_t column_x = 0; column_x < MAX_COLUMN_X; column_x++ )
			{
				for(uint8_t row_y = 0; row_y < MAX_ROW_Y; row_y++ )
				{
					if(p_global_variable->synthetic_oligo.u8_well_sequence[column_x * 8 + row_y] == CHEMICAL_SUBTANCE_EMPTY
							&& p_global_variable->synthetic_oligo.wellFirstsequence[column_x * 8 + row_y] != CHEMICAL_SUBTANCE_EMPTY
							&& p_global_variable->signal_kill[column_x * 8 + row_y] != true)
					{
						p_global_variable->sequenceWellDoneNeedAdd[column_x * 8 + row_y] = true;
					}
					else
					{
						p_global_variable->sequenceWellDoneNeedAdd[column_x * 8 + row_y] = false;
					}
				}
			}

			transferSquenceData(p_global_variable->sequenceWellDoneNeedAdd, sequenceWellDoneNeedAdd);
			switch(p_global_variable->advanced_setting.FillChemistryDone.typeReagent)
			{

			case U:
			case I:
				for(row_y = 0; row_y < 8; row_y++ ) // chay 4 hang
				{
					if(row_y % 2 == 0)
					{
						for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
						{
							ChemicalFillProcessWellDone_gr1valve(p_global_variable->advanced_setting.FillChemistryDone.typeReagent, u8_volume_temp, column_x, row_y, p_global_variable);
						}
					}
					else
					{
						for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
						{
							u8_position_x = MAX_COLUMN_X - 1 - column_x;
							ChemicalFillProcessWellDone_gr1valve(p_global_variable->advanced_setting.FillChemistryDone.typeReagent, u8_volume_temp, u8_position_x, row_y, p_global_variable);
						}
					}
				}
				break;
			case A:
			case T:
			case G:
			case C:
			case Activator:
			case TCA_in_DCM:
			case WASH_ACN_DCM:
			case OXIDATION_IODINE:
			case CAPPING_CAPA:
			case CAPPING_CAPB:
				for(row_y = 0; row_y < 2; row_y++ ) // chay 4 hang
				{
					if(row_y % 2 == 0)
					{
						for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
						{
							//Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
							ChemicalFillProcessWellDone_gr4valve(p_global_variable->advanced_setting.FillChemistryDone.typeReagent, u8_volume_temp, column_x, row_y, p_global_variable);
						}
					}
					else
					{
						for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
						{
							u8_position_x = MAX_COLUMN_X - 1 - column_x;
							ChemicalFillProcessWellDone_gr4valve(p_global_variable->advanced_setting.FillChemistryDone.typeReagent, u8_volume_temp, u8_position_x, row_y, p_global_variable);
							//Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
						}
					}
				}
				break;
			}
		}
	}
}

void Valve_EnaAll()
{
	for(uint8_t i = 0; i< MAX_NUMBER_VALVE; i++)
	{
		HAL_GPIO_WritePin(VALVE_[i].Port, VALVE_[i].Pin, SET);
	}
}

void Valve_DisAll()
{
	for(uint8_t i = 0; i< MAX_NUMBER_VALVE; i++)
	{
		HAL_GPIO_WritePin(VALVE_[i].Port, VALVE_[i].Pin, RESET);
	}
}



/****
 * valve_calculator_timefill
 * tinh toan the tich can bom cua valve
 */

uint16_t valve_calculator_timefill(Global_var_t* global_variable, uint8_t type_sulphite, uint16_t u16_Volume)
{
	if(u16_Volume  <= 0)
	{
		return 0;
	}
	else
	{
		double db_time = u16_Volume * global_variable->valve_setting[type_sulphite].f_a.Data + global_variable->valve_setting[type_sulphite].f_b.Data;
		return (uint16_t)(db_time);
	}
}

uint16_t calculator_timefill_gr4(Global_var_t* global_variable, uint8_t type_sulphite, uint8_t idx_valve, uint16_t u16_Volume)
{
	double db_time;
	if(u16_Volume <=  0)
	{
		return 0;
	}
	else
	{
		switch(type_sulphite)
		{
		case A:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 0].f_a.Data +global_variable->valve_setting[idx_valve  + 0].f_b.Data;
			break;
		}
		case T:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 4].f_a.Data +global_variable->valve_setting[idx_valve  + 4].f_b.Data;
			break;
		}
		case G:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 8].f_a.Data +global_variable->valve_setting[idx_valve  + 8].f_b.Data;
			break;
		}
		case C:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 12].f_a.Data +global_variable->valve_setting[idx_valve  + 12].f_b.Data;
			break;
		}
		case Activator:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 18].f_a.Data +global_variable->valve_setting[idx_valve  + 18].f_b.Data;
			break;
		}
		case TCA_in_DCM:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 22].f_a.Data +global_variable->valve_setting[idx_valve + 22].f_b.Data;
			break;
		}
		case WASH_ACN_DCM:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 26].f_a.Data +global_variable->valve_setting[idx_valve + 26].f_b.Data;
			break;
		}
		case OXIDATION_IODINE:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 30].f_a.Data +global_variable->valve_setting[idx_valve + 30].f_b.Data;
			break;
		}
		case CAPPING_CAPB:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 34].f_a.Data +global_variable->valve_setting[idx_valve + 34].f_b.Data;
			break;
		}
		case CAPPING_CAPA:
		{
			db_time = u16_Volume * global_variable->valve_setting[idx_valve + 38].f_a.Data +global_variable->valve_setting[idx_valve + 38].f_b.Data;
			break;
		}

		default:
		{
			break;
		}
		}

		return (uint16_t)(db_time);
	}
}

uint16_t calculator_volume_avg( Global_var_t* global_variable, uint8_t u8_mixed_base_code, uint16_t u16_volumme_sum)
{

	switch(u8_mixed_base_code)
	{
	case AMD_A:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_T:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_G:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_C:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_I:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_U:
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum;
		break;
	}
	case AMD_Y: // C & T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_R:	// G & 	A
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_W: // A & T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_S: //G & C
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_K: // G & T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_M: // A & C
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 2;
		break;
	}
	case AMD_D: // G & A & T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 3;
		break;
	}
	case AMD_V:// G & A & C
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 3;
		break;
	}
	case AMD_N: // G A C T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 4;
		break;
	}
	case AMD_H: // G A C T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 3;
		break;
	}
	case AMD_B: // G A C T
	{
		global_variable->mixed_base.u16_volumme_avg = u16_volumme_sum / 3;
		break;
	}
	}
	// ngan chặn các giá trị
	if(global_variable->mixed_base.u16_volumme_avg  < 10 && global_variable->mixed_base.u16_volumme_avg > 0)
	{
		global_variable->mixed_base.u16_volumme_avg = 10;
	}
	else
	{
		if(global_variable->mixed_base.u16_volumme_avg < 0)
		{
			global_variable->mixed_base.u16_volumme_avg = 0;
		}
	}
	return global_variable->mixed_base.u16_volumme_avg;
}



void Calib_volume(uint8_t type_sulphite, uint16_t u16_time_fill, uint16_t u8_pos_X,  uint16_t u8_pos_Y)
{

	if(u16_time_fill != 0)
	{
		Stepper_Z1_Run2Normal();
		Stepper_move_Coordinates_XY(u8_pos_X, u8_pos_Y);
		Stepper_Z1_move(Z_POSITION_CALIB);
		HAL_GPIO_WritePin(VALVE_[type_sulphite].Port, VALVE_[type_sulphite].Pin, SET);
		while (u16_time_fill > 0)
		{
			u16_time_fill--;
			HAL_Delay(1);
		}
		HAL_GPIO_WritePin(VALVE_[type_sulphite].Port, VALVE_[type_sulphite].Pin, RESET);
	}
}


void Calib_process(Global_var_t* p_global_variable)
{
	uint16_t X_Pos = 0;
	int8_t index  = 0;
	index = get_x_index_for_valve(p_global_variable->primming_control.u8_valve_sellect);
	switch(p_global_variable->primming_control.u8_valve_sellect)
	{
	case A1:
	case A2:
	case A3:
	case A4:
		X_Pos = Fill_Position_X[index][0] + OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case T1:
	case T2:
	case T3:
	case T4:
		X_Pos = Fill_Position_X[index][0] + OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case G1:
	case G2:
	case G3:
	case G4:
		X_Pos = Fill_Position_X[index][0] + OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case C1:
	case C2:
	case C3:
	case C4:
		X_Pos = Fill_Position_X[index][0] + OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case F1:
	case F2:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case ACTI1:
	case ACTI2:
	case ACTI3:
	case ACTI4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case TCA1:
	case TCA2:
	case TCA3:
	case TCA4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case WASH1:
	case WASH2:
	case WASH3:
	case WASH4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case OX1:
	case OX2:
	case OX3:
	case OX4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case CAPB1:
	case CAPB2:
	case CAPB3:
	case CAPB4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case CAPA1:
	case CAPA2:
	case CAPA3:
	case CAPA4:
		X_Pos = Fill_Position_X[index][0]+ OFFSET_X_CALIBRATION;
		Calib_volume(p_global_variable->primming_control.u8_valve_sellect,
				p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	default:
		break;
	}
}




void auto_primming( Global_var_t* p_global_variable, uint16_t volume, VALVE__CHEMICAL start_valve, VALVE__CHEMICAL end_valve)
{
	uint16_t X_Pos = 0;
	uint16_t Y_Pos = Y_CALIB_POS;
	uint8_t idxGetValve = 0;
	idxGetValve = get_x_index_for_valve((VALVE__CHEMICAL)(start_valve));
	uint32_t timeFill = valve_calculator_timefill(p_global_variable, start_valve, volume);
	X_Pos = Fill_Position_X[idxGetValve][0] + OFFSET_X_PRIMMING;
	Y_Pos = Y_CALIB_POS;
	Stepper_move_Coordinates_XY(X_Pos, Y_Pos); //
	Stepper_Z1_move(Z_POSITION_PRIMMING);          //
	for (uint8_t i = start_valve; i <= end_valve; i++)
	{
		Valve_Set(i); // Kích hoạt van
	}
	// Chờ theo thời gian cấu hình
	HAL_Delay(timeFill);
	Valve_DisAll();
	HAL_Delay(100);
}


/**
 * Author: LeHoaiGiang
 * @brief Di chuyển đến vị trí, kích hoạt các van cần thiết trong nhóm, chờ và tắt van.
 * @param start_valve Van bắt đầu của nhóm cần xử lý.
 * @param end_valve Van kết thúc của nhóm cần xử lý.
 * @param x_pos_index Chỉ số vị trí X trong mảng Fill_Position_X.
 */
void process_valve_group(Global_var_t* p_global_variable, VALVE__CHEMICAL start_valve, VALVE__CHEMICAL end_valve, uint8_t x_pos_index)
{
	// --- Di chuyển đến vị trí ---

	uint16_t X_Pos = 0;
	uint16_t Y_Pos = Y_CALIB_POS;
	uint8_t idxGetValve = 0;

	// --- Kích hoạt các van được chọn trong nhóm ---
	// Tạo một biến cờ để kiểm tra xem có van nào được bật không (tránh delay và tắt van vô ích)
	bool valve_activated = false;
	idxGetValve = x_pos_index;
	X_Pos = Fill_Position_X[idxGetValve][0] + OFFSET_X_PRIMMING;
	Y_Pos = Y_CALIB_POS;
	Stepper_move_Coordinates_XY(X_Pos, Y_Pos); //
	Stepper_Z1_move(Z_POSITION_PRIMMING);          //
	for (uint8_t i = start_valve; i <= end_valve; i++)
	{

		if (p_global_variable->primming_control.valve[i]) // Kiểm tra trạng thái từ biến global
		{
			Valve_Set((VALVE__CHEMICAL)i); // Kích hoạt van
			valve_activated = true;
		}
	}

	// --- Chờ và tắt van nếu có van được kích hoạt ---
	if (valve_activated)
	{
		// Chờ theo thời gian cấu hình
		DWT_Delay_ms(p_global_variable->primming_control.u8_time_primming_control * 100);
		// Tắt tất cả các van
		Valve_DisAll();
	}
	// Nếu không có van nào trong nhóm được kích hoạt, không cần delay và không cần gọi Valve_DisAll()
}

/**
 * @brief Kiểm tra xem có van nào cần hoạt động trong một khoảng xác định không.
 * @param start_valve Van bắt đầu của khoảng.
 * @param end_valve Van kết thúc của khoảng.
 * @retval true Nếu có ít nhất một van đang hoạt động.
 * @retval false Nếu không có van nào đang hoạt động.
 */
bool is_any_valve_active_in_range(Global_var_t* p_global_variable, VALVE__CHEMICAL start_valve, VALVE__CHEMICAL end_valve)
{
	for (uint8_t i = start_valve; i <= end_valve; i++)
	{
		if (p_global_variable->primming_control.valve[i])
		{
			return true; // Tìm thấy một van cần hoạt động
		}
	}
	return false; // Không có van nào hoạt động trong khoảng này
}


void execute_chemical_primming(Global_var_t* p_global_variable)
{
	// Duyệt qua từng nhóm van đã định nghĩa
	for (uint8_t i = 0; i < num_valve_groups; i++)
	{
		// Kiểm tra xem có van nào trong nhóm hiện tại đang được yêu cầu bật không
		if (is_any_valve_active_in_range(p_global_variable, valve_groups[i].start_valve, valve_groups[i].end_valve))
		{
			// Nếu có, xử lý nhóm van này
			process_valve_group(p_global_variable,
					valve_groups[i].start_valve,
					valve_groups[i].end_valve,
					valve_groups[i].x_pos_index
			);
			//break;
		}
	}
	// Có thể thêm các logic khác sau khi hoàn tất việc bơm (ví dụ: di chuyển về vị trí an toàn)
	Stepper_Z1_Run2Normal(); // Ví dụ
}


// Hàm trợ giúp: Tìm chỉ số X cho một van cụ thể
// Trả về chỉ số (0, 4, 6, ...) hoặc -1 nếu không tìm thấy
int8_t get_x_index_for_valve(VALVE__CHEMICAL valve_to_find) {
	for (uint8_t i = 0; i < num_valve_groups; i++) {
		if (valve_to_find >= valve_groups[i].start_valve && valve_to_find <= valve_groups[i].end_valve) {
			return valve_groups[i].x_pos_index;
		}
	}
	return -1; // Không tìm thấy van trong bất kỳ nhóm nào đã định nghĩa
}

// Hàm Calib_process đã tối ưu
void execute_valve_calibration(Global_var_t* p_global_variable) {
	// Lấy thông tin cần thiết từ biến global
	VALVE__CHEMICAL selected_valve = (VALVE__CHEMICAL)p_global_variable->primming_control.u8_valve_sellect;
	uint32_t calib_time_ms = p_global_variable->primming_control.u32fb_time_primming_calib.Data; // Dùng uint32_t

	// Tìm chỉ số X tương ứng với van được chọn
	int8_t x_index = get_x_index_for_valve(selected_valve);

	if (x_index != -1) { // Nếu tìm thấy van trong một nhóm cấu hình
		uint16_t X_Pos = Fill_Position_X[x_index][0] + OFFSET_X_CALIBRATION;

		// Gọi hàm Calib_volume với kiểu dữ liệu chính xác
		Calib_volume(selected_valve, calib_time_ms, X_Pos, Y_CALIB_POS); // Y_CALIB_POS giả sử là hằng số
	} else {
		// không có valve hoặc nhóm nào được chọn
	}
	// Stepper_Z1_move(Z_POSITION_PRIMMING); // Di chuyển Z về vị trí ban đầu nếu cần sau khi calib xong?
}

// transferArrays
void transferSquenceData(uint8_t *array_raw, uint8_t (*array_Data)[4]) {
	for (int i = 0; i < 12; i++) {
		// Lấy 8 phần tử từ array_raw[i*8 ... i*8+7]

		// Hàng đầu tiên của nhóm (0 -> 11)
		array_Data[i][0] = array_raw[i * 8 + 3];
		array_Data[i][1] = array_raw[i * 8 + 2];
		array_Data[i][2] = array_raw[i * 8 + 1];
		array_Data[i][3] = array_raw[i * 8 + 0];

		// Hàng thứ hai của nhóm (12 -> 23)
		array_Data[i + 12][0] = array_raw[i * 8 + 7];
		array_Data[i + 12][1] = array_raw[i * 8 + 6];
		array_Data[i + 12][2] = array_raw[i * 8 + 5];
		array_Data[i + 12][3] = array_raw[i * 8 + 4];
	}
}
//======================================================= 1.0.0.4 Cap nhat thuat toan bom hoa chat amidite =================================

// chuyen doi sequence tư software sang mang 2 chieu // Hàm chuyển đổi dữ liệu không đảo ngược
void transferSquenceDataSimple(uint8_t *array_raw, int sequence[4][12], int sequence2[4][12]) {
	for (int i = 0; i < 12; i++) {
		// 4 byte đầu
		sequence[0][i] = array_raw[i * 8 + 3];
		sequence[1][i] = array_raw[i * 8 + 2];
		sequence[2][i] = array_raw[i * 8 + 1];
		sequence[3][i] = array_raw[i * 8 + 0];

		// 4 byte sau
		sequence2[0][i] = array_raw[i * 8 + 7];
		sequence2[1][i] = array_raw[i * 8 + 6];
		sequence2[2][i] = array_raw[i * 8 + 5];
		sequence2[3][i] = array_raw[i * 8 + 4];
	}
}
//
void activate_valveAmidite(int row, int chem_type) {
	// row quyết định valve thứ mấy sẽ mở
	// chem_type quyết định hóa chất nào mở
	// well_col cot cua gieng nao se dc bom

	if (chem_type < 0 || chem_type >= VALVE_COLS || row < 0 || row >= ROWS) {

		return;
	}
	const GPIO_Config* valve = &VALVE_AMIDITE[chem_type][row];
	HAL_GPIO_WritePin(valve->Port, valve->Pin, GPIO_PIN_SET);
}

void deactivate_valveAmidite(int row, int chem_type) {
	if (chem_type < 0 || chem_type >= VALVE_COLS || row < 0 || row >= ROWS) {
		//printf("Lỗi: chem_type hoặc row không hợp lệ!\n");
		return;
	}

	const GPIO_Config* valve = &VALVE_AMIDITE[chem_type][row];

	// Tắt valve (giả sử active-high)
	HAL_GPIO_WritePin(valve->Port, valve->Pin, GPIO_PIN_RESET);
}

void deactivate_all_valvesAmidite(void) {
	for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
		for (int row = 0; row < ROWS; row++) {
			const GPIO_Config* valve = &VALVE_AMIDITE[chem_type][row];
			HAL_GPIO_WritePin(valve->Port, valve->Pin, GPIO_PIN_RESET);
		}
	}
}
/*	LeHoaiGiang 03-07-2025
 * viet ham bom amidite theo kieu 4 valve
 *
 *
 * */
/*
void Amidite_process_update(Global_var_t* p_global_variable, uint8_t u8_idx)
{
	uint16_t u16_volume;

	// get time fill chemical to well
	if(u8_idx == 0)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
	}
	if(u8_idx == 1)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
	}
	if(u8_idx == 2)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
	}
	uint32_t u16_time_fill[ROWS][VALVE_COLS];

	//	for(uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++)
	//	{
	//stransferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);
	// ====== BƯỚC 1: BƠM NGƯỢC THEO SEQUENCE ————-
	for (int step = 0; step < WELL_COLS + 3; step++) {
		//printf("=== BƯỚC %2d ===============================\n", step);
		// duyệt qua
		uint16_t PosX = Fill_Position_XAmidite[0][step];
		uint16_t PosY = Fill_Position_YAmidite[0][0];
		Stepper_move_Coordinates_XY(PosX, PosY);
		for(uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++)
		{
			transferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);
			for (int row = 0; row < ROWS; row++) {
				for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
					int well_col = step - chem_type;

					if (well_col >= 0 && well_col < WELL_COLS) {
						if (sequence1[row][well_col] == chem_type) {
							u16_time_fill[row][chem_type] = valve_calculator_timefill(p_global_variable, chem_type, u16_volume);
							//activate_valve(row, chem_type, well_col);
						} else {
							u16_time_fill[row][chem_type] = 0;
						}
					} else {
						u16_time_fill[row][chem_type] = 0;
					}
				}
			}

			//printf("  [+] Bật tất cả valve cần bơm...\n");
			// ====== BƯỚC 1: BƠM THEO SEQUENCE ————-
			uint8_t pumping = 1;
			while (pumping) {
				pumping = 0;
				for (int row = 0; row < ROWS; row++) {
					for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
						if (u16_time_fill[row][chem_type] > 0) {
							if (u16_time_fill[row][chem_type] > 0) {
								//printf("      -> Valve[%d][%d] bắt đầu bơm...\n", row, chem_type);

								activate_valveAmidite(row, chem_type);
							}
							u16_time_fill[row][chem_type]--;
							if (u16_time_fill[row][chem_type] <= 0) {
								//printf("      <- Valve[%d][%d] kết thúc bơm.\n", row, chem_type);
								deactivate_valveAmidite(row, chem_type);
							}
							pumping = 1;
						}
					}
				}
				HAL_Delay(1);
			} // END while pumping
			deactivate_all_valvesAmidite();
		}

		//printf("\n");
	} // end for step 1 ket thuc BƯỚC 1
	// ====== BƯỚC 2: BƠM NGƯỢC THEO SEQUENCE2 ————-

	for (int step = 0 ; step < WELL_COLS + 3; step++) {
		//printf("=== BƯỚC %2d ===============================\n", step);
		// duyệt qua MẢNG copy dữ liệu
		uint16_t PosX = Fill_Position_XAmidite[0][step];
		uint16_t PosY = Fill_Position_YAmidite[0][1];
		Stepper_move_Coordinates_XY(PosX, PosY);
		for(uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++)
		{
			transferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);
			for (int row = 0; row < ROWS; row++) {
				for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
					int well_col = step - chem_type;

					if (well_col >= 0 && well_col < WELL_COLS) {
						if (sequence2[row][well_col] == chem_type) {
							u16_time_fill[row][chem_type] = valve_calculator_timefill(p_global_variable, chem_type, u16_volume);
							//activate_valve(row, chem_type, well_col);
						} else {
							u16_time_fill[row][chem_type] = 0;
						}
					} else {
						u16_time_fill[row][chem_type] = 0;
						//printf("  [Lỗi index]\n");
					}
				}
			}// end for ROWS

			//printf("  [+] Bật tất cả valve cần bơm...\n");
			//
			uint8_t pumping = 1;
			while (pumping) {
				pumping = 0;
				for (int row = 0; row < ROWS; row++) {
					for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
						if (u16_time_fill[row][chem_type] > 0) {
							if (u16_time_fill[row][chem_type] > 0) {
								//printf("      -> Valve[%d][%d] bắt đầu bơm...\n", row, chem_type);
								activate_valveAmidite(row, chem_type);
							}
							u16_time_fill[row][chem_type]--;
							if (u16_time_fill[row][chem_type] <= 0) {
								//printf("      <- Valve[%d][%d] kết thúc bơm.\n", row, chem_type);
								deactivate_valveAmidite(row, chem_type);
							}
							pumping = 1;  // valve nao con can bom thi van tron while
						}
					}
				}// end for ROWS
				HAL_Delay(1);
			}// end while (pumping)
			//printf("\n"); // Cách dòng giữa các bước
			deactivate_all_valvesAmidite();
		}

	}
	//} //  end for mixed
}

 */
void Amidite_process_update(Global_var_t* p_global_variable, uint8_t u8_idx) {
	uint16_t u16_volume;

	// get time fill chemical to well
	if(u8_idx == 0)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[0].Data;
	}
	if(u8_idx == 1)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[1].Data;
	}
	if(u8_idx == 2)
	{
		u16_volume = p_global_variable->synthetic_oligo.u16_volume_func_mix_well[2].Data;
	}
	//uint32_t u16_time_fill[ROWS][VALVE_COLS];

	uint32_t u16_time_fill[ROWS][VALVE_COLS];

	// Mảng định nghĩa thứ tự mới: G - A - C - T
	// Giả sử: G=2, A=0, C=3, T=1
	const int valve_order[VALVE_COLS] = {2, 0, 3, 1};

	// ====== BƯỚC 1: BƠM NGƯỢC THEO SEQUENCE1 ————-
	for (int step = 0; step < WELL_COLS + 3; step++) {
		uint16_t PosX = Fill_Position_XAmidite[0][step];
		uint16_t PosY = Fill_Position_YAmidite[0][0];


		for (uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++) {
			transferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);

			for (int row = 0; row < ROWS; row++) {
				// Thay đổi ở đây: Lặp qua thứ tự van mới
				for (int i = 0; i < VALVE_COLS; i++) {
					int chem_type = valve_order[i]; // Lấy chem_type theo thứ tự mới
					int well_col = step - i; // Vị trí giếng giờ phụ thuộc vào chỉ số 'i' của mảng thứ tự

					if (well_col >= 0 && well_col < WELL_COLS) {
						if (sequence1[row][well_col] == chem_type) {
							u16_time_fill[row][chem_type] = valve_calculator_timefill(p_global_variable, chem_type, u16_volume);
						} else {
							u16_time_fill[row][chem_type] = 0;
						}
					} else {
						u16_time_fill[row][chem_type] = 0;
					}
				}
			}

			// ... (Phần code bơm và delay giữ nguyên) ...
			// Vòng lặp while(pumping) sẽ hoạt động đúng vì nó duyệt qua tất cả chem_type
			uint8_t pumping = 1;
			while (pumping) {
				pumping = 0;
				for (int row = 0; row < ROWS; row++) {
					for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) { // Vòng lặp này giữ nguyên
						if (u16_time_fill[row][chem_type] > 0) {
							Stepper_move_Coordinates_XY(PosX, PosY);
							activate_valveAmidite(row, chem_type);
							u16_time_fill[row][chem_type]--;
							if (u16_time_fill[row][chem_type] <= 0) {
								deactivate_valveAmidite(row, chem_type);
							}
							pumping = 1;
						}
					}
				}
				HAL_Delay(1);
			}
			deactivate_all_valvesAmidite();
		}
	}

	// ====== BƯỚC 1: BƠM NGƯỢC THEO SEQUENCE1 ————-
	for (int step = 0; step < WELL_COLS + 3; step++) {
		uint16_t PosX = Fill_Position_XAmidite[0][step];
		uint16_t PosY = Fill_Position_YAmidite[0][1];


		for (uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++) {
			transferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);

			for (int row = 0; row < ROWS; row++) {
				// Thay đổi ở đây: Lặp qua thứ tự van mới
				for (int i = 0; i < VALVE_COLS; i++) {
					int chem_type = valve_order[i]; // Lấy chem_type theo thứ tự mới
					int well_col = step - i; // Vị trí giếng giờ phụ thuộc vào chỉ số 'i' của mảng thứ tự

					if (well_col >= 0 && well_col < WELL_COLS) {
						if (sequence2[row][well_col] == chem_type) {
							u16_time_fill[row][chem_type] = valve_calculator_timefill(p_global_variable, chem_type, u16_volume);
						} else {
							u16_time_fill[row][chem_type] = 0;
						}
					} else {
						u16_time_fill[row][chem_type] = 0;
					}
				}
			}

			// ... (Phần code bơm và delay giữ nguyên) ...
			// Vòng lặp while(pumping) sẽ hoạt động đúng vì nó duyệt qua tất cả chem_type
			uint8_t pumping = 1;
			while (pumping) {
				pumping = 0;
				for (int row = 0; row < ROWS; row++) {
					for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) { // Vòng lặp này giữ nguyên
						if (u16_time_fill[row][chem_type] > 0) {
							Stepper_move_Coordinates_XY(PosX, PosY);
							activate_valveAmidite(row, chem_type);
							u16_time_fill[row][chem_type]--;
							if (u16_time_fill[row][chem_type] <= 0) {
								deactivate_valveAmidite(row, chem_type);
							}
							pumping = 1;
						}
					}
				}
				HAL_Delay(1);
			}
			deactivate_all_valvesAmidite();
		}
	}
	// ====== BƯỚC 2: BƠM NGƯỢC THEO SEQUENCE2 ————-
	// Áp dụng thay đổi tương tự cho Bước 2
	/*
	for (int step = 0; step < WELL_COLS + 3; step++) {
		uint16_t PosX = Fill_Position_XAmidite[0][step];
		uint16_t PosY = Fill_Position_YAmidite[0][1];
		Stepper_move_Coordinates_XY(PosX, PosY);

		for(uint8_t mixedIdx = 0; mixedIdx < MAX_MIXED_COMPONENTS; mixedIdx++) {
			transferSquenceDataSimple(p_global_variable->mixed_sequence[mixedIdx].component, sequence1, sequence2);
			for (int row = 0; row < ROWS; row++) {
				// Thay đổi ở đây: Lặp qua thứ tự van mới
				for (int i = 0; i < VALVE_COLS; i++) {
					int chem_type = valve_order[i]; // Lấy chem_type theo thứ tự mới
					int well_col = step - i; // Vị trí giếng giờ phụ thuộc vào chỉ số 'i'

					if (well_col >= 0 && well_col < WELL_COLS) {
						if (sequence2[row][well_col] == chem_type) {
							u16_time_fill[row][chem_type] = valve_calculator_timefill(p_global_variable, chem_type, u16_volume);
						} else {
							u16_time_fill[row][chem_type] = 0;
						}
					} else {
						u16_time_fill[row][chem_type] = 0;
					}
				}
			}
			// ... (Phần code bơm và delay giữ nguyên) ...
			uint8_t pumping = 1;
			while (pumping) {
				pumping = 0;
				for (int row = 0; row < ROWS; row++) {
					for (int chem_type = 0; chem_type < VALVE_COLS; chem_type++) {
						if (u16_time_fill[row][chem_type] > 0) {
							activate_valveAmidite(row, chem_type);
							u16_time_fill[row][chem_type]--;
							if (u16_time_fill[row][chem_type] <= 0) {
								deactivate_valveAmidite(row, chem_type);
							}
							pumping = 1;
						}
					}
				}
				HAL_Delay(1);
			}
			deactivate_all_valvesAmidite();
		}
	}// END FOR for (int step = 0; step < WELL_COLS + 3; step++)
	 */
}

void process_well_sequences(Global_var_t* p_global_variable,
		uint8_t u8_well_sequence[MAX_WELL_AMIDITE],
		uint16_t default_volume)
{
	// Khởi tạo toàn bộ dữ liệu cho mixed_sequence
	for (int i = 0; i < MAX_MIXED_COMPONENTS; i++) {
		for (int pos = 0; pos < MAX_WELL_AMIDITE; pos++) {
			p_global_variable->mixed_sequence[i].component[pos] = CHEMICAL_SUBTANCE_EMPTY;
			p_global_variable->mixed_sequence[i].volume[pos] = 0;
			p_global_variable->mixed_sequence[i].used_in_well[pos] = 0;
		}
	}

	// Xử lý từng well
	for (int pos = 0; pos < MAX_WELL_AMIDITE; pos++)
	{
		uint8_t base_code = u8_well_sequence[pos];
		const int *components = MIXED_BASE[base_code];

		// Đếm số lượng thành phần thực tế
		int count = 0;
		while (count < MAX_MIXED_COMPONENTS && components[count] != CHEMICAL_SUBTANCE_EMPTY) {
			count++;
		}

		if (count == 0) continue; // Không có thành phần → bỏ qua

		uint16_t avg_volume = default_volume / count;

		// Set ngưỡng tối thiểu là 10
		if (avg_volume < 10) {
			avg_volume = 10;
		}

		// Gán từng thành phần vào mixed_sequence
		for (int i = 0; i < count; i++)
		{
			p_global_variable->mixed_sequence[i].component[pos] = components[i];
			p_global_variable->mixed_sequence[i].volume[pos] = avg_volume;
			p_global_variable->mixed_sequence[i].used_in_well[pos] = 1;
		}

		// Gán các ô còn lại là rỗng (nếu có)
		for (int i = count; i < MAX_MIXED_COMPONENTS; i++) {
			p_global_variable->mixed_sequence[i].component[pos] = CHEMICAL_SUBTANCE_EMPTY;
			p_global_variable->mixed_sequence[i].volume[pos] = 0;
			p_global_variable->mixed_sequence[i].used_in_well[pos] = 0;
		}
	}
}


//======================================================================================== OPEN VALVE THE HE MOI ===================================================================================
// ####################################################################################### CAP NHAT 15-07-2025 #####################################################################################
/**
 * Author: LeHoaiGiang
 * @brief Mo Valve theo vi tri va signal
 * @param start_valve Van bắt đầu của nhóm cần xử lý.
 * @param end_valve Van kết thúc của nhóm cần xử lý.
 * @param x_pos_index Chỉ số vị trí X trong mảng Fill_Position_X.
 */
void open_valves(ORDINAL_VALVE type, bool signal_fill[4], int16_t u16_time_fill[4])
{
	uint8_t num_valves = VALVES_PER_TYPE[type];
	uint8_t base_index = VALVE_OFFSET_MAP[type];

	for(int i = 0; i < num_valves; i++)
	{
		if (base_index + i >= MAX_NUMBER_VALVE)
			continue; // Tránh truy xuất ngoài mảng

		if(signal_fill[i])
			HAL_GPIO_WritePin(VALVE_[base_index + i].Port, VALVE_[base_index + i].Pin, SET);
	}

	while(u16_time_fill[0] > 0 || u16_time_fill[1] > 0 ||
			u16_time_fill[2] > 0 || u16_time_fill[3] > 0)
	{
		for(int i = 0; i < num_valves; i++)
		{
			if(u16_time_fill[i] > 0) u16_time_fill[i]--;
			else {
				if (base_index + i < MAX_NUMBER_VALVE)
					HAL_GPIO_WritePin(VALVE_[base_index + i].Port, VALVE_[base_index + i].Pin, RESET);
			}
		}
		HAL_Delay(1);
	}

	for(int i = 0; i < num_valves; i++)
	{
		if (base_index + i < MAX_NUMBER_VALVE)
			HAL_GPIO_WritePin(VALVE_[base_index + i].Port, VALVE_[base_index + i].Pin, RESET);
	}
}


void ManualFill_ChemicalGroup4(uint8_t chemicalType, uint16_t volume, uint8_t columnIndex, uint8_t rowIndex, Global_var_t* globalData)
{
	bool signal_fill[4] = {false};
	int16_t u16_time_fill[4] = {0};

	// Kiểm tra từng well có cần fill không
	for(int idx = 0; idx < 4; idx++)
	{
		uint8_t well_index = rowIndex * 4 + columnIndex * 8 + idx;
		if(globalData->manual_run.u8_checked_well[well_index])
		{
			signal_fill[idx] = true;
		}
	}

	if(!(signal_fill[0] || signal_fill[1] || signal_fill[2] || signal_fill[3]))
		return;

	if(volume == 0) return;

	int pos_x = Fill_Position_X[chemicalType][columnIndex];
	int pos_y = Fill_Position_Y[chemicalType][rowIndex];

	Stepper_move_Coordinates_XY(pos_x, pos_y);

	// Tính thời gian cho từng valve theo kiểu đảo ngược (valve 3 -> 2 -> 1 -> 0)
	for(int i = 0; i < 4; i++)
	{
		if(signal_fill[3 - i])
		{
			u16_time_fill[i] = Valve_CalculateOpenTime(globalData, (ORDINAL_VALVE)chemicalType, 3 - i, volume);
		}
	}

	open_valves(chemicalType, signal_fill, u16_time_fill);
}

/*
 * Author: LeHoaiGiang
 * @brief tinh toan thoi gian mo valve
 * @param
 * @param
 * uint16_t time = valve_calculate_fill_time(global_data, A, 1, 100);
// Tính thời gian cho valve thứ 2 của loại hóa chất A với thể tích 100
 */


uint16_t Valve_CalculateOpenTime(Global_var_t* global, ORDINAL_VALVE type, uint8_t valve_idx, uint16_t volume)
{
	if (volume == 0 || type >= MAX_ORDINAL_VALVE)
		return 0;

	uint8_t base_index = VALVE_OFFSET_MAP[type];
	uint8_t max_valves = VALVES_PER_TYPE[type];

	if (valve_idx >= max_valves)
		return 0;

	float a = global->valve_setting[base_index + valve_idx].f_a.Data;
	float b = global->valve_setting[base_index + valve_idx].f_b.Data;

	float db_time = volume * a + b;

	return (uint16_t)(db_time);
}
