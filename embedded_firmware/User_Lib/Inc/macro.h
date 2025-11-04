/*
 * macro.h
 *
 *  Created on: Sep 26, 2022
 *      Author: LeHoaiGiang
 */

#ifndef INC_MACRO_H_
#define INC_MACRO_H_
//#define SYNO24_PNA
#ifdef SYNO24_PNA
#define UART_LENGTH_COMMAND 		256
#define UART_LENGTH_DATA_OLIGO 		256
#define UART_LENGTH_COMMAND_TX 		40
#else
#define UART_LENGTH_COMMAND_RX 		512
#define UART_LENGTH_COMMAND_TX 		50
#endif


#define CMD_CONNECT_LINK						0x01
#define CMD_RUNSTEPPER							0x03
#define CMD_RUN2HOME							0x02
#define CMD_PRIMMING							0x04
#define CMD_START_OLIGO_STEP					0x05
#define CMD_DATA_OLIGO							0x06
#define CMD_ASK_VENDOR_ID						0x07
#define CMD_SIGNAL_START_OLIGO      			0x08
#define CMD_SEND_STOPED_OLIGO					0x09
#define CMD_FIRMWARE_END_OLIGO_STEP 			0x0A
#define CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR	0x0B
#define CMD_CONTROL_AIR_START					0x0C
#define CMD_CONTROL_MIXED_AIR					0x0D  // command ket hop day khi xuong va thoi khi len lam sach vien frit
#define CMD_REQUEST_PAUSE                       0x0F
#define CMD_RECIVED_SETTING						0x10
#define CMD_POSITION_STEPPER					0x11
#define CMD_ALARM								0x12
#define CMD_CALIBRATION_VALVE   				0x64
#define CMD_EXHAUSTED_CHEMICAL					0x65
#define CHEMICAL_SUBTANCE_EMPTY    				0x7F
#define CMD_MANUAL_RUN          				0x66
#define CMD_PRESURE_TESTING     				0x19
#define CMD_TRITYL_START						0x20
#define CMD_TRITYL_STOP							0x21
#define CMD_CONTROL_OUTPUT						0x22
#define CMD_SEUQENCE_AND_KILL       			0x25
// synchronize input output control
#define CMD_CONTROL_SYNCHRONIZE_IO				0x23

//Coordinates config feature
#define CMD_WRITE_COORDINATES_SYSTEM			0x69
#define CMD_REQUEST_COORDINATES_SW				0x70
#define CMD_FEEDBACK_STATUS_RUN					0x67
#define CMD_STOP_SYSTHETIC_OLIGO                0x99





//#define	X_PRIMMING_POS				60
//#define	Y_PRIMMING_POS				65
//===========================================
//#define UART_DEBUG
//===========================================
#define MAX_WELL_AMIDITE			96
#define MAX_NUMBER_VALVE			42
#define MAX_ORDINAL_VALVE  16 // dùng để chứa tổng số hóa chất trong đây
typedef enum  // cái này dùng để định nghĩa STEP run của một Step luôn và cả thứ tự để truy xuất valve
{
    A = 0, // valve 1
    T = 1,
    G = 2,
    C = 3,
    I = 4,
    U = 5,
    Activator = 6,
    TCA_in_DCM = 7,
    WASH_ACN_DCM = 8,
    OXIDATION_IODINE = 9,
    CAPPING_CAPA = 11,
    CAPPING_CAPB = 10,
    COUPLING = 12,
    FUNTION_MIXED = 13,
    CAPPING = 14,
	DMT_OFF = 15,
}ORDINAL_VALVE;

enum COUPLING_FNC
{
    A_ = 0, // valve 1
    T_ = 1,
    G_ = 2,
    X_ = 3,
    I_ = 4,
    U_ = 5,
    Activator_ = 6,
    TCA_in_DCM_ = 7,
    WASH_ACN_DCM_ = 8,
    OXIDATION_IODINE_ = 9,
    CAPPING_CAPA_ = 10,
    CAPPING_CAPB_ = 11,
    AMIDITE = 12,
};
enum OPTION_CONTROL_PRESSURE
{
    LOW_PUSH = 0,
    HIGH_PUSH = 1,
    HIGH_VACUUM = 2,
    LOW_VACUUM = 3,
};
enum MIX_AMIDITE
{
    AMD_A = 0, // valve 1
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
    AMD_H = 15,
    AMD_B = 16,
};

enum CONTROL_OUTPUT_SYSTEM
{
	FAN = 1,
	LAMP = 2,

};
enum SOLENOID_NAME
{
	FAN_SV = 0, // 33 xa khi nito giam do am
	LED_RED_SV  = 1, // 34
	LED_GREEN_SV  = 2, // 35
	FAN_VACUUM_BOX  = 3, // V3 IO BOARD GIANG AD 05-06-2035
	LOW_PUSH_SV  = 4,// V37
	HIGH_PUSH_SV = 5,
	V39_EMPTY = 6,
	OPEN_NITOR_SV  = 7,
};

enum SUBFUNCTION_STT_FB
{
	WAIT_AFTERFILL = 0,
	PUSHDOWN_FNC = 1,
	WAIT_FNC = 2,
};
#endif /* INC_MACRO_H_ */
