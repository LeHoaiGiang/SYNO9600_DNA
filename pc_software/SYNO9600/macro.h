#ifndef MACRO_H
#define MACRO_H
#include "qstring.h"
#define LENGTH_COMMAND_RECEIVED     50
#define LENGTH_COMMAND_SEND         512
#define BAUDRATE_UART               115200
//#define UART_LENGTH_COMMAND 		256
//#define UART_LENGTH_DATA_OLIGO 		256
#define CMD_CONNECT_LINK			0x01
#define CMD_RUNSTEPPER				0x03
#define CMD_RUN2HOME				0x02
#define CMD_PRIMMING				0x04
#define CMD_START_OLIGO_STEP		0x05
#define CMD_DATA_OLIGO				0x06
#define CMD_FIRMWARE_END_OLIGO_STEP 0x0A
#define CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR	0x0B
#define CMD_CONTROL_AIR_START       0x0C
#define CMD_CONTROL_MIXED_AIR		0x0D
#define CMD_RECIVED_SETTING			0x10
#define CMD_ASK_VENDOR_ID			0x07
#define CMD_SIGNAL_START_OLIGO      0x08
#define CMD_SEND_STOPED				0x09
#define CMD_CALIBRATION_VALVE   	0x64
#define CMD_EXHAUSTED_CHEMICAL		0x65
#define CMD_MANUAL_RUN          	0x66
#define CMD_PRESURE_TESTING     	0x19
#define CMD_STOP_SYSTHETIC_OLIGO    0x99
#define CMD_TRITYL_START			0x20
#define CMD_TRITYL_STOP				0x21
#define CMD_SEUQENCE_AND_KILL       0x25
// synchronize input output control
#define CMD_CONTROL_SYNCHRONIZE_IO	0x23
#define CMD_FEEDBACK_STATUS_RUN         0x67
//============================================================= AMIDITE & VALVE MACRO ==========================================
#define CHEMICAL_SUBTANCE_EMPTY    	0x7F
#define CHEMICAL_SUBTANCE_HAVE      0x02
#define MAX_WELL_AMIDITE			96
#define MAX_NUMBER_VALVE			42
#define MAX_SEQUENCE_OF_WELL        127
#define MAX_SUB_OF_PROTOCOL         10
#define MAX_STEP_OF_SUB             12

#define INTERVAL_TIMEFILL_CALIB_1ST         800
#define INTERVAL_TIMEFILL_CALIB_2ND         2000
#define VOLUME_FILL_50UL                    50
//==============================================================================================================================
//#define START_PROCESS_SYNTHETIC_OLIGO                   1
//#define STOP_PROCESS_SYNTHETIC_OLIGO                    0
//#define MANUAL_PROCESS_SYNTHETIC_OLIGO                  1
//#define STOP_MANUAL_PROCESS_SYNTHETIC_OLIGO             0
enum ORDINAL_VALVE
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
    DMT_OFF = 15
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

enum OPTION_CHEMICAL_STEP
{
    DEBLOCK_FNC = 0,
    WASH_FNC = 1,
    COUPLING_FNC = 2,
    OXICATION_FNC = 3,
    CAP_FNC = 4,
    MIXED_FNC = 5
};
enum OPTION_DOUPLE_COUPLING
{
    NONE_DOUBLE_COUPLING = 0,
    DOUBLE_COUPLING_FIRSTBASE = 1,
    DOUBLE_COUPLING_FIRST_SECOND_BASE = 2,
    DOUBLE_COUPLING_ALL_BASE = 3
};
enum ERROR_CODE_SYSTEM
{
    CONNECT_COM_ERR = 0,
    SENSOR_HUM_ERR = 1,
    NO_RESPONESE_ERR = 2,
    LIMIT_XSW_ERR = 3,
    LIMIT_YSW_ERR = 4,
    LIMIT_ZSW_ERR = 5,
};
//QString const PATH_SAVE_AMIDITE = "C:/software/00_Data/Amidite.json";
enum OLIGO_WELL_STATUS
{
    Sequence_InProgress = 0,
    Sequence_Completed = 1,
    Sequence_Off = 2,
    Sequence_Empty = 3
};

enum TAB_UI
{
    SYSTEM_TAB = 0,
    SEQUENCE_TAB= 1,
    VOLUME_MANAGER_TAB = 2,
    PROTOCOL_TAB = 3,
    TRITYL_TAB = 4,
    RUN_TAB = 5,
    UPDATE_FW_TAB  = 6
};

enum SUBFUNCTION_STT_FB
{
    WAIT_AFTERFILL =0,
    PUSHDOWN_FNC = 1,
    WAIT_FNC = 2,
};
#endif // MACRO_H
