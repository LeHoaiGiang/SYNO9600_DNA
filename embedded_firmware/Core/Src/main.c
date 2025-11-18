/* USER CODE BEGIN Header */
// SYNO9600 update 28-03-2025 v1.0.0.3 thay doi vi tri valve
//
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "dwt_stm32_delay.h"
#include "stepper.h"
#include "struct.h"
#include <stdio.h>

#include "function.h"
#include "hdc1080.h"
#include<stdarg.h>
#include "hmp110.h"
#include "ads1115x.h"
#include "global_extern.h"
//#include "sht2x_for_stm32_hal.h"
//#include "ADS1115.h"
#include "i2c-lcd.h"
#include "control_air.h"
#include "internal_flash.h"
#include "stdio.h"
#include "uartPrintDebug.h"
#include "gpio_timer.h"
#define LCD_DEBUG
#define HORN_TOGGLE_INTERVAL 150  // Th�?i gian đảo trạng thái (150ms)
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//#define DEBUG_SOFTWARE
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_VALVE 40
GPIO_Config SOLENOID[8] =
{
		{GPIOG, GPIO_PIN_0},// FAN_SV
		{GPIOE, GPIO_PIN_14},//v8 //LED_RED_SV
		{GPIOE, GPIO_PIN_12},//v10//LED_GREEN_SV
		{GPIOB, GPIO_PIN_11},//v3 io board Gianglh 05-06-2025
		{GPIOB, GPIO_PIN_14},//LOW_PUSH_SV
		{GPIOB, GPIO_PIN_10},//HIGH_PUSH_SV
		{GPIOF, GPIO_PIN_15},//v18 EMPTY
		{GPIOB, GPIO_PIN_12},//OPEN_NITOR_SV
};



/*
 * 		{GPIOE, GPIO_PIN_14},//v8
		{GPIOE, GPIO_PIN_12},//v10
		{GPIOE, GPIO_PIN_10},//v12
		{GPIOE, GPIO_PIN_8},//v14
		{GPIOG, GPIO_PIN_1},//v16
		{GPIOF, GPIO_PIN_15},//v18
 *
 */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim12;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
void Syno24_stepper_Init();
void UART_Send_Command_SW();
void uint8ToHexString(uint8_t num, char* hexString);
void START_OLIGO_SYNTHETIC();
void STOP_OLIGO();
void PNA_wait_and_process_sync(uint16_t u16_intervaltime_process);
void update_status_and_sensor_to_SW();
void Get_sensor();
void DNA_wait_time(uint16_t u16_intervaltime_process);
void MANUAL_RUN();
void pushdown_LowPressure_Enable();
void pushdown_LowPressure_Disable();
void pushdown_HighPressure_Enable();
void pushdown_HighPressure_Disable();
void EnableVacuumBox(uint16_t timeOpen);
void FeatureVacuumBox();
void syno24_fill_air2well();
void uart_send_Feedback_Status_Run(uint8_t u8_function_count, uint8_t u8_subfunction_run);
void Syno24_get_and_auto_control_Humidity();
void syno24_Control_Air_Humidity();
void send_status_machine();
void autoPrimming_beforeCoupling();
void Process_Trityl_Collection();
void Finish_Trityl_Collection();
void buzzer_mute();
void HORN_OFF();
void HORN_ON();
void hornControl();
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM14_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM12_Init(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len) {
	CDC_Transmit_FS((uint8_t*) ptr, len); return len;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const uint8_t idx_start_opt_vaccum = 50;
const uint8_t idx_start_time_process = 60;
const uint8_t idx_start_time_wait = 80; // 20byte tuu 80 den 99
const uint8_t idx_start_time_fill_mixfunction_1 = 100; // 20byte tu
const uint8_t idx_start_time_fill_mixfunction_2 = 150; // 20byte tu
const uint8_t idx_start_time_fill_mixfunction_3 = 200; // 20byte tu
const uint8_t idx_start_sequence_amidite = 250;
const uint16_t idx_VacuumBox = 350;
volatile float Temperature;
volatile float Humidity;
// Luu command tam thoi, tranh bi thay doi data khi dang chay
uint8_t Command[UART_LENGTH_COMMAND_RX];
uint8_t	u8_index_well = 0;
Global_var_t global_variable = {0};
uint16_t u16_intervaltime = 4000;
uint16_t u16_currentMillis;
uint32_t u32_ADC_temp = 0;
uint32_t u32_ADC_AVG = 0;
int i_size;
float f_Volt_T;
float f_Volt_H;
float f_Pressure;
char hexString[6]; // Chuỗi kết quả có thể chứa "0xXY" và ký tự null-terminator
char hexString_rx[6];
char hexString_delivery[6];
extern int Fill_Position_X[12][12];
dataFlash_onRam_t dataFlash_onRam={0};
uint32_t hornStartTime;
bool CONTROL_STATE[2];
// millis define
uint32_t previousMillis_print = 0;
uint32_t currentMillis_print = 0;
GPIO_Timer_Object FanInBox;

// ====================================
uint16_t array_A[96];
uint16_t array_B[24][4];
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	MX_TIM2_Init();
	MX_TIM14_Init();
	MX_I2C1_Init();
	MX_TIM5_Init();
	MX_TIM1_Init();
	MX_TIM12_Init();
	MX_USB_DEVICE_Init();
	/* USER CODE BEGIN 2 */
	Syno24_stepper_Init();
	DWT_Delay_Init(); // init dwt delay
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);
	HAL_TIM_Base_Start(&htim2);
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
	//HAL_TIM_OnePulse_Start(&htim1, TIM_CHANNEL_3);
	__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0); // TIMER5 CHANNEL 3 timer 10khz
	__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0); // TIMER5 CHANNEL 3 timer 10khz
	HAL_UART_Receive_DMA(&huart1, &global_variable.UART_Command.u8_Data_Rx_Buffer[0], UART_LENGTH_COMMAND_RX);
	global_variable.UART_Command.b_FW_Rx_Command_Flag = 0;
	global_variable.signal_running.b_signal_update_status = 0;
	global_variable.signal_running.b_signal_connect_software = 0;
	global_variable.signal_running.b_signal_runing_oligo = false;
	global_variable.control_air.u16_counter_2second = 0;
	global_variable.status_and_sensor.u16_temperature.Data = 0;
	global_variable.status_and_sensor.u16_humidity.Data = 0;
	Get_sensor();
	HAL_GPIO_WritePin(Z2_EN_GPIO_Port, Z2_EN_Pin, SET);
	// Khoi dong GPIO cua quat
	GPIO_Timer_init(&FanInBox, SOLENOID[FAN_VACUUM_BOX].Port, SOLENOID[FAN_VACUUM_BOX].Pin);
#ifdef DEBUG_SOFTWARE

#else
	Stepper_AutoHome_SYN024();
#endif
	global_variable.control_air.b_auto_control_NITO = false;
	// 02.09.2024 tinh nang autocheck hoa chat amidite va ACTIVATOR
	// tinh nang vacumm waste hoa chat do
	global_variable.advanced_setting.flag_auto_primming_chemical = false;
	global_variable.advanced_setting.flag_vacumm_waste = false;
	global_variable.control_air.b_enable_clean_box = false;// tinh nang tu dong hut khi bang quat ra ngoai true = enable
	__HAL_TIM_SetCounter(&htim2, 0);
	global_variable.status_and_sensor.flag_enable_auto_control_air_Nito = RESET;
	//HAL_Delay(10000);
	//SYNO_testFlash();
	HAL_Delay(50);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		//currentMillis_print = HAL_GetTick();
		//		if ((currentMillis_print - previousMillis_print) >= 1000) {
		//			DEBUG_PRINT("TEST PRINT DEBUG");
		//			previousMillis_print = currentMillis_print;
		//		}
		hornControl();
		if(global_variable.UART_Command.b_FW_Rx_Command_Flag == true)
		{
			global_variable.UART_Command.b_FW_Rx_Command_Flag = false; // disable flag have command uart
			memcpy(&Command[0], &global_variable.UART_Command.u8_Data_Rx_Buffer[0], UART_LENGTH_COMMAND_RX);
			switch(Command[0])
			{
			case CMD_ASK_VENDOR_ID:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_ASK_VENDOR_ID;
				global_variable.signal_running.b_signal_connect_software = true;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_RECIVED_SETTING:
			{
				for(uint8_t index_valve = 0; index_valve < MAX_NUMBER_VALVE; index_valve++)
				{
					global_variable.valve_setting[index_valve].f_a.Byte[0] = Command[index_valve*8 + 1];
					global_variable.valve_setting[index_valve].f_a.Byte[1] = Command[index_valve*8 + 2];
					global_variable.valve_setting[index_valve].f_a.Byte[2] = Command[index_valve*8 + 3];
					global_variable.valve_setting[index_valve].f_a.Byte[3] = Command[index_valve*8 + 4];
					global_variable.valve_setting[index_valve].f_b.Byte[0] = Command[index_valve*8 + 5];
					global_variable.valve_setting[index_valve].f_b.Byte[1] = Command[index_valve*8 + 6];
					global_variable.valve_setting[index_valve].f_b.Byte[2] = Command[index_valve*8 + 7];
					global_variable.valve_setting[index_valve].f_b.Byte[3] = Command[index_valve*8 + 8];
				}
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_RECIVED_SETTING;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_PRESURE_TESTING:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_PRESURE_TESTING;
				UART_Send_Command_SW();
				break;
			}
			case CMD_CONTROL_MIXED_AIR: // command nay ket hop push up va push down
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_CONTROL_MIXED_AIR;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR:
			{
				Get_sensor();
				if(Command[1] == CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR) // neu yeu cau doi do am thi bat khi vao ngay lap tuc
				{
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
					global_variable.status_and_sensor.flag_enable_auto_control_air_Nito = SET;
					global_variable.status_and_sensor.u16tb_humidity_Preset.Byte[0] = Command[2];
					global_variable.status_and_sensor.u16tb_humidity_Preset.Byte[1] = Command[3];
					global_variable.status_and_sensor.u8_high_limit_humidity = 	global_variable.status_and_sensor.u16tb_humidity_Preset.Data + OFFSET_LIMIT_VALUE;
					global_variable.status_and_sensor.u8_low_limit_humidity = global_variable.status_and_sensor.u16tb_humidity_Preset.Data;
				}
				// 29-10-24 GiangLH them gui vi tri cua stepper len software == software can xu ly de lay gia tri nay
				global_variable.status_and_sensor.u16_temperature.Data = global_variable.status_and_sensor.f_temperature * 100;
				global_variable.status_and_sensor.u16_humidity.Data = global_variable.status_and_sensor.f_humidity * 100;
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_FIRMWARE_UPDATE_STATUS_AND_SENSOR;
				global_variable.UART_Command.u8_Data_Tx_Buffer[1] = global_variable.status_and_sensor.u16_temperature.Byte[0];
				global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.status_and_sensor.u16_temperature.Byte[1];
				global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.status_and_sensor.u16_humidity.Byte[0];
				global_variable.UART_Command.u8_Data_Tx_Buffer[4] = global_variable.status_and_sensor.u16_humidity.Byte[1];
				global_variable.UART_Command.u8_Data_Tx_Buffer[5] = (getPositionX()) & 0xFF; // byte thap
				global_variable.UART_Command.u8_Data_Tx_Buffer[6] = (getPositionX() >> 8)&0xFF; // byte cao
				global_variable.UART_Command.u8_Data_Tx_Buffer[7] = (getPositionY()) & 0xFF; //  byte thap
				global_variable.UART_Command.u8_Data_Tx_Buffer[8] = (getPositionY() >> 8)&0xFF;
				global_variable.UART_Command.u8_Data_Tx_Buffer[9] = (getPositionZ1()) & 0xFF;
				global_variable.UART_Command.u8_Data_Tx_Buffer[10] = (getPositionZ1() >> 8)&0xFF;
				//global_variable.status_and_sensor.u16tb_X_Position.Data = getPositionX();
				//global_variable.status_and_sensor.u16tb_Y_Position.Data = getPositionY();
				//global_variable.status_and_sensor.u16tb_Z1_Position.Data = getPositionZ1();
				//buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_RUN2HOME:
			{
				//Stepper_Z1_move(Z_POSITION_NORMAL);
#ifdef DEBUG_SOFTWARE
#else
				Stepper_AutoHome_SYN024();
				Stepper_move_Coordinates_XY(1500,0);
#endif
				// ==========================	send command to Software	==================================
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_RUN2HOME;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_CONTROL_AIR_START:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_CONTROL_AIR_START;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_RUNSTEPPER:
			{
				global_variable.signal_control.u16tb_X_Distance.Byte[0] = Command[1];
				global_variable.signal_control.u16tb_X_Distance.Byte[1] = Command[2];
				global_variable.signal_control.u16tb_Y_Distance.Byte[0] = Command[3];
				global_variable.signal_control.u16tb_Y_Distance.Byte[1] = Command[4];
				global_variable.signal_control.u16tb_Z1_Distance.Byte[0] = Command[5];
				global_variable.signal_control.u16tb_Z1_Distance.Byte[1] = Command[6];
				Stepper_Z1_move(global_variable.signal_control.u16tb_Z1_Distance.Data);
				Stepper_move_Coordinates_XY( global_variable.signal_control.u16tb_X_Distance.Data, global_variable.signal_control.u16tb_Y_Distance.Data);
				// ==========================	 send command software		=========================================================
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_RUNSTEPPER;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_CALIBRATION_VALVE: // software tinh toan thoi gian phun cua Valve gui xuong - firmware chi can phun dung thoi gian
			{
				//Stepper_Z1_Run2Normal(); // di chuyen
				global_variable.primming_control.u8_valve_sellect = Command[1];
				global_variable.primming_control.u32fb_time_primming_calib.Byte[0] = Command[2];
				global_variable.primming_control.u32fb_time_primming_calib.Byte[1] = Command[3];
				global_variable.primming_control.u32fb_time_primming_calib.Byte[2] = Command[4];
				global_variable.primming_control.u32fb_time_primming_calib.Byte[3] = Command[5];
				Stepper_Z1_move(Z_POSITION_CALIB);
				execute_valve_calibration(&global_variable);
				//Calib_process(&global_variable);
				//==================== send command software  =======================================================================
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_CALIBRATION_VALVE;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_PRIMMING: // XA BOT KHI -- Cap Nhat tinh nang nay cho tung valve 04/01/2023
			{
				uint16_t idx_valve = 0;
				for(idx_valve = 0; idx_valve < MAX_NUMBER_VALVE; idx_valve++)
				{
					global_variable.primming_control.valve[idx_valve] = Command[idx_valve + 1];
				}
				idx_valve++;
				global_variable.primming_control.u8_time_primming_control = Command[idx_valve];
				idx_valve++;
				global_variable.primming_control.b_custom_position = 		Command[idx_valve];
				if(global_variable.primming_control.b_custom_position == true)
				{

				}
				else
				{
					execute_chemical_primming(&global_variable);
				}
				// lay xong data
				/*
				if(global_variable.primming_control.b_custom_position == true)
				{

				}
				else
				{
					uint16_t X_Pos = 0;
					const uint16_t offsetX = OFFSET_X_PRIMMING;
					if(global_variable.primming_control.valve[A1] == true  || global_variable.primming_control.valve[T1] == true ||
							global_variable.primming_control.valve[G1] == true  || global_variable.primming_control.valve[C1] == true	)
					{

						X_Pos = Fill_Position_X[0][0] + offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = A1; i<= C1; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}
					if(global_variable.primming_control.valve[F1] == true  || global_variable.primming_control.valve[F2] == true)
					{
						X_Pos = Fill_Position_X[4][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = F1; i<= F2; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}

					if(global_variable.primming_control.valve[ACTI1] == true  || global_variable.primming_control.valve[ACTI2] == true||
							global_variable.primming_control.valve[ACTI3] == true  || global_variable.primming_control.valve[ACTI4] == true)
					{

						X_Pos = Fill_Position_X[4][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = ACTI1; i<= ACTI4; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}

					if(		global_variable.primming_control.valve[TCA1] == true  || global_variable.primming_control.valve[TCA2] == true||
							global_variable.primming_control.valve[TCA3] == true  || global_variable.primming_control.valve[TCA4] == true)
					{

						X_Pos = Fill_Position_X[7][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = TCA1; i<= TCA4; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}
					if(global_variable.primming_control.valve[WASH1] == true  || global_variable.primming_control.valve[WASH2] == true||
							global_variable.primming_control.valve[WASH3] == true  || global_variable.primming_control.valve[WASH4] == true||
							global_variable.primming_control.valve[OX1] == true  || global_variable.primming_control.valve[OX2] == true||
							global_variable.primming_control.valve[OX3] == true  || global_variable.primming_control.valve[OX4] == true)
					{

						X_Pos = Fill_Position_X[8][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = WASH1; i<= OX4; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}
					if(global_variable.primming_control.valve[CAPB1] == true  || global_variable.primming_control.valve[CAPB2] == true||
							global_variable.primming_control.valve[CAPB3] == true  || global_variable.primming_control.valve[CAPB4] == true||
							global_variable.primming_control.valve[CAPA1] == true  || global_variable.primming_control.valve[CAPA2] == true||
							global_variable.primming_control.valve[CAPA3] == true  || global_variable.primming_control.valve[CAPA4] == true)
					{
						X_Pos = Fill_Position_X[10][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = CAPB1; i<= CAPA4; i++)
						{
							if(global_variable.primming_control.valve[i])
							{
								Valve_Set(i);
							}
						}
						DWT_Delay_ms(global_variable.primming_control.u8_time_primming_control* 100);
						Valve_DisAll();
					}
				}
				 */
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_PRIMMING;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_SEUQENCE_AND_KILL:
			{
				int idx_killsequence = 97;
				for(int idx = 0; idx < 96; idx++)
				{
					global_variable.signal_kill[idx] = Command[idx_killsequence];
					idx_killsequence++;
				}
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_SEUQENCE_AND_KILL;
				UART_Send_Command_SW();
				break;
			}
			case CMD_DATA_OLIGO: // receive data run oligo
			{
				// Get Data Oligo from software
				//==================== send command software	==============================================
				global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical = Command[1];
				global_variable.synthetic_oligo.fill_chemical.u16tb_Volume.Byte[0] = Command[2];
				global_variable.synthetic_oligo.fill_chemical.u16tb_Volume.Byte[1] = Command[3];
				global_variable.synthetic_oligo.b_douple_coupling_first_base = Command[4];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[0].Byte[0] = Command[5];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[0].Byte[1] = Command[6];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[1].Byte[0] = Command[7];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[1].Byte[1] = Command[8];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[2].Byte[0] = Command[9];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[2].Byte[1] = Command[10];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u8_type_chemical[0] = Command[11];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u8_type_chemical[1] = Command[12];
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u8_type_chemical[2] = Command[13];
				global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Byte[0] = Command[14];
				global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Byte[1] = Command[15];
				global_variable.status_and_sensor.flag_enable_auto_control_air_Nito = Command[16]; // CONTROL NITOR HAY KHONG
				global_variable.synthetic_oligo.isSpecialBase = Command[17]; // isSpecialBase
				global_variable.synthetic_oligo.u16_scale_volume.Byte[0] = Command[18];
				global_variable.synthetic_oligo.u16_scale_volume.Byte[1] = Command[19];
				global_variable.synthetic_oligo.u16_scale_time.Byte[0] = Command[20];
				global_variable.synthetic_oligo.u16_scale_time.Byte[1] = Command[21];

				global_variable.signal_running.u16_counter_base_finished = ((Command[23]<<8) | Command[22]);

				global_variable.signal_running.u16_counter_step = Command[26]; // 07/09/2024 them gui step dang chay cho autocheck washing
				global_variable.advanced_setting.flag_auto_primming_chemical = Command[27];
				global_variable.advanced_setting.u16tb_autoPrim_volume_amidite.Byte[0] = Command[28];
				global_variable.advanced_setting.u16tb_autoPrim_volume_amidite.Byte[1] = Command[29];
				global_variable.advanced_setting.u16tb_autoPrim_volume_Activator.Byte[0] = Command[30];
				global_variable.advanced_setting.u16tb_autoPrim_volume_Activator.Byte[1] = Command[31];
				// 11-07-2025 add function
				global_variable.advanced_setting.FillChemistryDone.EnableFillWellDone = Command[32];
				global_variable.advanced_setting.FillChemistryDone.En_WASH= Command[33];
				global_variable.advanced_setting.FillChemistryDone.En_Deblock= Command[34];
				global_variable.advanced_setting.FillChemistryDone.En_Coupling= Command[35];
				global_variable.advanced_setting.FillChemistryDone.En_Deblock= Command[36];
				global_variable.advanced_setting.FillChemistryDone.En_Coupling= Command[37];
				global_variable.advanced_setting.FillChemistryDone.typeReagent= Command[38];
				global_variable.advanced_setting.FillChemistryDone.volumeWASH.Byte[0]= Command[39];
				global_variable.advanced_setting.FillChemistryDone.volumeWASH.Byte[1]= Command[40];
				global_variable.advanced_setting.FillChemistryDone.volumeDeblock.Byte[0]= Command[41];
				global_variable.advanced_setting.FillChemistryDone.volumeDeblock.Byte[1]= Command[42];
				global_variable.advanced_setting.FillChemistryDone.volumeCoupling.Byte[0]= Command[43];
				global_variable.advanced_setting.FillChemistryDone.volumeCoupling.Byte[1]= Command[44];
				global_variable.advanced_setting.FillChemistryDone.volumeCap.Byte[0]= Command[45];
				global_variable.advanced_setting.FillChemistryDone.volumeCap.Byte[1]= Command[46];
				global_variable.advanced_setting.FillChemistryDone.volumeOx.Byte[0]= Command[47];
				global_variable.advanced_setting.FillChemistryDone.volumeOx.Byte[1]= Command[48];

				if(global_variable.signal_running.u16_counter_base_finished % 5 == 0 && global_variable.control_air.b_enable_clean_box != true &&
						global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == TCA_in_DCM)
				{
					//global_variable.control_air.b_enable_clean_box = true;
					global_variable.control_air.u16_counter_30second = 10;
				}
				else
				{
					//global_variable.control_air.b_enable_clean_box = false;
				}
				for(uint8_t u8_idx_well = 0; u8_idx_well < MAX_WELL_AMIDITE; u8_idx_well++)
				{
					global_variable.synthetic_oligo.u8_well_sequence[u8_idx_well] = Command[idx_start_sequence_amidite + u8_idx_well];
					if(global_variable.signal_running.u16_counter_base_finished == 0) // 22-05-2025 cho tinh nang tu dong bom well da hoan thanh hoac trong
					{
						global_variable.synthetic_oligo.wellFirstsequence[u8_idx_well] = global_variable.synthetic_oligo.u8_well_sequence[u8_idx_well];
					}
				}
				//// 17-04-24 thay doi so voi syno24 cho nay la volume - firmware tu tinh ra thoi gian bom
				global_variable.synthetic_oligo.u16_volume_func_mix_well[0].Byte[0] =  Command[idx_start_time_fill_mixfunction_1 ];
				global_variable.synthetic_oligo.u16_volume_func_mix_well[0].Byte[1] =  Command[idx_start_time_fill_mixfunction_1 + 1];
				global_variable.synthetic_oligo.u16_volume_func_mix_well[1].Byte[0] =  Command[idx_start_time_fill_mixfunction_1 + 2];
				global_variable.synthetic_oligo.u16_volume_func_mix_well[1].Byte[1] =  Command[idx_start_time_fill_mixfunction_1 + 3];
				global_variable.synthetic_oligo.u16_volume_func_mix_well[2].Byte[0] =  Command[idx_start_time_fill_mixfunction_1 + 4];
				global_variable.synthetic_oligo.u16_volume_func_mix_well[2].Byte[1] =  Command[idx_start_time_fill_mixfunction_1 + 5];
				for(uint8_t idx_process = 0; idx_process < 10; idx_process++)
				{
					global_variable.synthetic_oligo.control_pressure.u8_option_pressure[idx_process] = Command[idx_start_opt_vaccum + idx_process];
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[idx_process].Byte[0] = Command[idx_start_time_process + idx_process*2];
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[idx_process].Byte[1] = Command[idx_start_time_process + idx_process*2 + 1];
					global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[idx_process].Byte[0]  = Command[idx_start_time_wait + idx_process*2];
					global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[idx_process].Byte[1] = Command[idx_start_time_wait + idx_process*2 + 1];
				}
				//global_variable.advanced_setting.flag_autocheck_pressure = Command[201];
				int currentIdx = idx_VacuumBox; // Biến theo dõi chỉ số hiện tại
				global_variable.advanced_setting.VacuumBox.Enablefeature = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.En_WASH = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.En_Deblock = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.En_Coupling = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.En_Ox = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.En_Cap = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.time.Byte[0] = Command[currentIdx];
				currentIdx++;
				global_variable.advanced_setting.VacuumBox.time.Byte[1]  = Command[currentIdx];
				currentIdx++;
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_DATA_OLIGO;
				UART_Send_Command_SW();
				break;
			}
			case CMD_START_OLIGO_STEP: // start synthetic oligo
			{
				HORN_OFF();
				global_variable.signal_running.b_signal_runing_oligo = true;
				//==================== send command software ==============================
				Stepper_Z1_move(Z_POSITION_NORMAL);
				START_OLIGO_SYNTHETIC();
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_START_OLIGO_STEP;
				global_variable.signal_running.b_signal_runing_oligo = false;
				buzzer_blink();
				UART_Send_Command_SW();
				break;
			}
			case CMD_FIRMWARE_END_OLIGO_STEP:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_FIRMWARE_END_OLIGO_STEP;
				STOP_OLIGO();
				UART_Send_Command_SW();
				buzzer_blink();
				break;
			}
			case CMD_MANUAL_RUN:
			{
				// 26-03-2024 sua tinh nang va cau truc cua manual run nhung chua test
				// FIRMWARE VA SOFTWARE CHUA TEST
				Stepper_Z1_move(Z_POSITION_NORMAL);
				global_variable.manual_run.U8_TASK_CONTROL = Command[1];
				global_variable.manual_run.u8_typeof_chemical = Command[2];
				global_variable.manual_run.u16_volume.Byte[0] = Command[3];
				global_variable.manual_run.u16_volume.Byte[1] = Command[4];
				global_variable.manual_run.u8_option_pressure[0] = Command[29];
				global_variable.manual_run.u16tb_procs_time[0].Byte[0] = Command[30];
				global_variable.manual_run.u16tb_procs_time[0].Byte[1] = Command[31];
				global_variable.manual_run.u16tb_waitting_after_time[0].Byte[0] = Command[32];
				global_variable.manual_run.u16tb_waitting_after_time[0].Byte[1] = Command[33];
				global_variable.manual_run.u8_option_pressure[1] = Command[34];
				global_variable.manual_run.u16tb_procs_time[1].Byte[0] = Command[35];
				global_variable.manual_run.u16tb_procs_time[1].Byte[1] = Command[36];
				global_variable.manual_run.u16tb_waitting_after_time[1].Byte[0] = Command[37];
				global_variable.manual_run.u16tb_waitting_after_time[1].Byte[1] = Command[38];
				global_variable.manual_run.u8_option_pressure[2] = Command[39];
				global_variable.manual_run.u16tb_procs_time[2].Byte[0] = Command[40];
				global_variable.manual_run.u16tb_procs_time[2].Byte[1] = Command[41];
				global_variable.manual_run.u16tb_waitting_after_time[2].Byte[0] = Command[42];
				global_variable.manual_run.u16tb_waitting_after_time[2].Byte[1] = Command[43];
				global_variable.manual_run.u8_option_pressure[3] = Command[44];
				global_variable.manual_run.u16tb_procs_time[3].Byte[0] = Command[45];
				global_variable.manual_run.u16tb_procs_time[3].Byte[1] = Command[46];
				global_variable.manual_run.u16tb_waitting_after_time[3].Byte[0] = Command[47];
				global_variable.manual_run.u16tb_waitting_after_time[3].Byte[1] = Command[48];
				for(uint8_t idx_valve = 0; idx_valve < MAX_WELL_AMIDITE; idx_valve++)
				{
					global_variable.manual_run.u8_checked_well[idx_valve] = Command[idx_valve + 49];
				}
				MANUAL_RUN();
				buzzer_blink();
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_MANUAL_RUN;
				UART_Send_Command_SW();
				break;
			}
			case CMD_STOP_SYSTHETIC_OLIGO:
			{
				Valve_DisAll();
				//global_variable.control_air.b_enable_clean_box = true;
				//global_variable.control_air.u16_counter_30second = 30;
				//syno24_Control_Air_Humidity();
				global_variable.advanced_setting.flag_exhaustFan = Command[1];
				global_variable.advanced_setting.u16tb_timeExhaustFan.Byte[0] = Command[2];
				global_variable.advanced_setting.u16tb_timeExhaustFan.Byte[1] = Command[3];
				// mo qua day khi ra khoi thung, tranh bi hoi phong
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, SET);
				if(global_variable.advanced_setting.flag_exhaustFan)
				{
					HAL_Delay(global_variable.advanced_setting.u16tb_timeExhaustFan.Data * 60 * 1000); // software phut
				}
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);
				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, RESET);
				global_variable.signal_running.b_signal_runing_oligo = false;
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_STOP_SYSTHETIC_OLIGO;
				UART_Send_Command_SW();
				break;
			}
			case CMD_REQUEST_PAUSE:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_REQUEST_PAUSE;
				UART_Send_Command_SW();
				break;
			}
			case CMD_TRITYL_START:
			{
				uint8_t subcommand = 0;
				subcommand = Command[1];
				// sub command 1 là hệ thống đến vị trí save state đè chặt trong hộp || 2 là chạy đến vị trí đưa plate vào
				if(subcommand == 1)
				{
					Stepper_Z1_Run2Normal();
					Stepper_move_Coordinates_XY( X_POSITION_PUSH_DOWN, Y_POSITION_PUSH_DOWN);
					Stepper_Z1_move(Z_POSITION_PUSH_DOWN);
					//ON HORN
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);
					HAL_Delay(2000);
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);
					HORN_ON();
				}
				else
				{
					if(subcommand == 2)
					{
						Process_Trityl_Collection();
						// HORN OFF
						HORN_OFF();
					}
					else
					{
						if(subcommand == 3)
						{
							HORN_ON();
						}
						else
						{
							HORN_OFF();
						}
					}
				}
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_TRITYL_START;
				// volume dac biet dung cho trityl add them 12-02-2025 chua test
				global_variable.advanced_setting.flag_speacial_volume_trityl = Command[2]; // = 1 la bom trityl volume
				global_variable.advanced_setting.volume_trityl_collection.Byte[0] = Command[3];
				global_variable.advanced_setting.volume_trityl_collection.Byte[1] = Command[4];
				UART_Send_Command_SW();
				break;
			}
			case CMD_TRITYL_STOP:
			{
				// Gianglh thay doi chu trinh thu DMT
				// 14-02-2024 chuyen tu vi tri gan cua de import plate -> save state plate
				Stepper_move_Coordinates_XY( X_POSITION_PUSH_DOWN, Y_POSITION_PUSH_DOWN);
				Stepper_Z1_move(Z_POSITION_PUSH_DOWN);
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_TRITYL_STOP;
				UART_Send_Command_SW();
				break;
			}
			case CMD_ALARM:
			{


				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_ALARM;
				break;
			}
			case CMD_WRITE_COORDINATES_SYSTEM:
			{

				verifyAndSaveData(Command);
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_WRITE_COORDINATES_SYSTEM;

				UART_Send_Command_SW();
				break;
			}
			case CMD_REQUEST_COORDINATES_SW:
			{
				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_REQUEST_COORDINATES_SW;
				UART_Send_Command_SW();
				break;
			}
			case CMD_CONTROL_SYNCHRONIZE_IO:
			{

				uint8_t index = 1;
				for(uint8_t i = 0; i < MAX_NUMBER_VALVE; i++)
				{
					global_variable.signal_control.stateControlValve[i] = Command[index];
					//HAL_GPIO_WritePin(VALVE[i].Port, VALVE[i].Pin, global_variable.signal_control.stateControlValve[i]);
					index++;
				}

				// have 8 solenoid
				index = 31;
				for(uint8_t i = 0; i < 8; i++)
				{
					global_variable.signal_control.stateControlSolenoid[i] = Command[index];
					HAL_GPIO_WritePin(SOLENOID[i].Port, SOLENOID[i].Pin, global_variable.signal_control.stateControlSolenoid[i]);
					index++;
				}
				// response data software


				global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_CONTROL_SYNCHRONIZE_IO;
				UART_Send_Command_SW();
				break;
			}
			default:
			{
				break;
			}
			} // end switch check command
			//buzzer_mute();
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}



	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 63000;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 300;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 7;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OnePulse_Init(&htim1, TIM_OPMODE_SINGLE) != HAL_OK)
	{
		Error_Handler();
	}
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
	sSlaveConfig.InputTrigger = TIM_TS_ITR1;
	if (HAL_TIM_SlaveConfigSynchro(&htim1, &sSlaveConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 41999;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 4294967295;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief TIM5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM5_Init(void)
{

	/* USER CODE BEGIN TIM5_Init 0 */

	/* USER CODE END TIM5_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	/* USER CODE BEGIN TIM5_Init 1 */

	/* USER CODE END TIM5_Init 1 */
	htim5.Instance = TIM5;
	htim5.Init.Prescaler = 83;
	htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim5.Init.Period = 99;
	htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM5_Init 2 */

	/* USER CODE END TIM5_Init 2 */

}

/**
 * @brief TIM12 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM12_Init(void)
{

	/* USER CODE BEGIN TIM12_Init 0 */

	/* USER CODE END TIM12_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = {0};

	/* USER CODE BEGIN TIM12_Init 1 */

	/* USER CODE END TIM12_Init 1 */
	htim12.Instance = TIM12;
	htim12.Init.Prescaler = 50000;
	htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim12.Init.Period = 300;
	htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim12) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim12, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM12_Init 2 */

	/* USER CODE END TIM12_Init 2 */

}

/**
 * @brief TIM14 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM14_Init(void)
{

	/* USER CODE BEGIN TIM14_Init 0 */

	/* USER CODE END TIM14_Init 0 */

	/* USER CODE BEGIN TIM14_Init 1 */

	/* USER CODE END TIM14_Init 1 */
	htim14.Instance = TIM14;
	htim14.Init.Prescaler = 8399;
	htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim14.Init.Period = 29999;
	htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM14_Init 2 */

	/* USER CODE END TIM14_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA2_Stream5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
	/* DMA2_Stream7_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOE, LED_Pin|Vacuum_3_Pin|Z2_EN_Pin|VALVE_10_Pin
			|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
			|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
			|GPIO_PIN_15, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
			|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
			|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
			|GPIO_PIN_4|GPIO_PIN_5|VALVE_8_Pin|SOLENOID_1_Pin
			|VALVE_9_Pin|VALVE_12_Pin|X_STEP_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|Vacuum_2_Pin|Vacuum_1_Pin|Vacuum_4_Pin
			|VALVE_11_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
			|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
			|buzzer_Pin|Z1_STEP_Pin|Z1_DIR_Pin|Z1_ENA_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0|GPIO_PIN_1|CTRL_5_Pin|VALVE_5_Pin
			|SOLENOID_4_Pin|VALVE_6_Pin|SOLENOID_3_Pin|VALVE_7_Pin
			|SOLENOID_2_Pin|Y_STEP_Pin|Y_DIR_Pin|Y_EN_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, VALVE_1_Pin|CTRL_8_Pin|VALVE_2_Pin|CTRL7_Pin
			|VALVE_3_Pin|CTRL6_Pin|VALVE_4_Pin|X_DIR_Pin
			|X_EN_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : LED_Pin Vacuum_3_Pin Z2_EN_Pin VALVE_10_Pin
                           PE7 PE8 PE9 PE10
                           PE11 PE12 PE13 PE14
                           PE15 */
	GPIO_InitStruct.Pin = LED_Pin|Vacuum_3_Pin|Z2_EN_Pin|VALVE_10_Pin
			|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
			|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
			|GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/*Configure GPIO pins : Z2_LIMIT_Pin Z1_LIMIT_Pin */
	GPIO_InitStruct.Pin = Z2_LIMIT_Pin|Z1_LIMIT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	/*Configure GPIO pins : PF4 PF5 PF6 PF7
                           PF8 PF9 PF10 PF11
                           PF12 PF13 PF14 PF15 */
	GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
			|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
			|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

	/*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC4 PC5 VALVE_8_Pin SOLENOID_1_Pin
                           VALVE_9_Pin VALVE_12_Pin */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
			|GPIO_PIN_4|GPIO_PIN_5|VALVE_8_Pin|SOLENOID_1_Pin
			|VALVE_9_Pin|VALVE_12_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : PA0 Vacuum_2_Pin Vacuum_1_Pin Vacuum_4_Pin
                           VALVE_11_Pin */
	GPIO_InitStruct.Pin = GPIO_PIN_0|Vacuum_2_Pin|Vacuum_1_Pin|Vacuum_4_Pin
			|VALVE_11_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB11 PB12 PB13 PB14
                           buzzer_Pin Z1_DIR_Pin Z1_ENA_Pin */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
			|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
			|buzzer_Pin|Z1_DIR_Pin|Z1_ENA_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : PG0 PG1 CTRL_5_Pin VALVE_5_Pin
                           SOLENOID_4_Pin VALVE_6_Pin SOLENOID_3_Pin VALVE_7_Pin
                           SOLENOID_2_Pin Y_DIR_Pin Y_EN_Pin */
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|CTRL_5_Pin|VALVE_5_Pin
			|SOLENOID_4_Pin|VALVE_6_Pin|SOLENOID_3_Pin|VALVE_7_Pin
			|SOLENOID_2_Pin|Y_DIR_Pin|Y_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	/*Configure GPIO pins : VALVE_1_Pin CTRL_8_Pin VALVE_2_Pin CTRL7_Pin
                           VALVE_3_Pin CTRL6_Pin VALVE_4_Pin X_DIR_Pin
                           X_EN_Pin */
	GPIO_InitStruct.Pin = VALVE_1_Pin|CTRL_8_Pin|VALVE_2_Pin|CTRL7_Pin
			|VALVE_3_Pin|CTRL6_Pin|VALVE_4_Pin|X_DIR_Pin
			|X_EN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pin : X_STEP_Pin */
	GPIO_InitStruct.Pin = X_STEP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(X_STEP_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : X_LIMIT_Pin */
	GPIO_InitStruct.Pin = X_LIMIT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(X_LIMIT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : Y_STEP_Pin */
	GPIO_InitStruct.Pin = Y_STEP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(Y_STEP_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : Y_LIMIT_Pin */
	GPIO_InitStruct.Pin = Y_LIMIT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(Y_LIMIT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : Z1_STEP_Pin */
	GPIO_InitStruct.Pin = Z1_STEP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(Z1_STEP_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void MANUAL_RUN()
{
	Stepper_Z1_move(Z_POSITION_NORMAL);
	static uint8_t column_x;
	static uint8_t row_y;
	static uint8_t u8_position_y = 0;
	static uint8_t u8_chemical_temp;
	static uint16_t u16_volume_temp;
	static uint8_t u8_position_x;
	column_x = 0;
	row_y = 0;
	u8_position_y = 0;
	// 	global_variable.manual_run.u8_checked_well[column_x * 8 + row_y] == 1
	//	global_variable.manual_run.u8_typeof_chemical
	//	global_variable.manual_run.u16_volume.Data
	Stepper_Z1_move(Z_POSITION_FILL_CHEMICAL);
	if(global_variable.manual_run.U8_TASK_CONTROL == 1)
	{
		column_x = 0;
		row_y = 0;
		u8_chemical_temp = CHEMICAL_SUBTANCE_EMPTY;
		u16_volume_temp = 0;
		// kiem tra loai hoa chat
		u8_chemical_temp = global_variable.manual_run.u8_typeof_chemical;
		u16_volume_temp = global_variable.manual_run.u16_volume.Data;
		switch(u8_chemical_temp)
		{
		case A:
		case T:
		case G:
		case C:
			// 11-07-25 GiangLH chuyen thanh chay 4 valve
			for(row_y = 0; row_y < 2; row_y++ ) // chay 4 hang
			{
				if(row_y % 2 == 0)
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						manual_fill_gr4(u8_chemical_temp, u16_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						manual_fill_gr4(u8_chemical_temp, u16_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
			// comment vi cho nay chuyen thanh su dung 1 valve ==  khi can su dung 2 valve thi mơ lai
			//					for(row_y = 0; row_y < 4; row_y++ ) // chay 4 hang
			//					{
			//						if(row_y % 2 == 0)
			//						{
			//							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
			//							{
			//								Chemical_fill_process_gr2valve(u8_chemical_temp, u8_volume_temp, column_x, row_y);
			//							}
			//						}
			//						else
			//						{
			//							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
			//							{
			//								u8_position_x = MAX_COLUMN_X - 1 - column_x;
			//								Chemical_fill_process_gr2valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y);
			//							}
			//						}
			//
			//					}
			//					break;
		case U:
		case I:
			for(row_y = 0; row_y < 8; row_y++ ) // chay 4 hang
			{
				if(row_y % 2 == 0)
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ )
					{
						manual_fill_gr1(u8_chemical_temp, u16_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ )
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						manual_fill_gr1(u8_chemical_temp, u16_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
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
						manual_fill_gr4(u8_chemical_temp, u16_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						manual_fill_gr4(u8_chemical_temp, u16_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
		}
		// tro ve vi tri binh thuong
		Stepper_Z1_Run2Normal();
	}
	else
	{
		for(uint8_t u8_counter_state = 0 ; u8_counter_state < 4; u8_counter_state++)
		{
			switch(global_variable.manual_run.u8_option_pressure[u8_counter_state])
			{
			case HIGH_VACUUM:
			{
				if((global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data != 0) &&
						global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data != 0)
				{
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // TAT DUONG XA KHI
					HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, SET); // MƠ KHI CB
					HAL_GPIO_WritePin(SOLENOID_3_GPIO_Port, SOLENOID_3_Pin, RESET);
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 99);
					HAL_Delay(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // waitting time
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, SET); // MO DUONG XA KHI
					HAL_Delay(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // waitting time
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // DONG DUONG XA KHI
					HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, RESET); // DONG KHI CB VACUUM
				}
				break;
			}
			case LOW_VACUUM:
			{
				if((global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data != 0) &&
						global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data != 0)
				{
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // TAT DUONG XA KHI
					HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, SET); // MƠ KHI CB
					HAL_GPIO_WritePin(SOLENOID_3_GPIO_Port, SOLENOID_3_Pin, RESET);
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 20);
					HAL_Delay(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // waitting time
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, SET); // MO DUONG XA KHI
					HAL_Delay(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // waitting time
					HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // DONG DUONG XA KHI
					HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, RESET); // DONG KHI CB VACUUM
				}
				break;
			}
			case LOW_PUSH:
			{
				if((global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data != 0) &&
						global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data != 0)
				{
					//==================================================================================
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
					pushdown_LowPressure_Enable();
					DWT_Delay_ms(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // process time
					pushdown_LowPressure_Disable();
#ifdef SYNO24_PNA
					global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Byte[0];
					global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Byte[1];
					uart_send_Feedback_Status_Run(1);
					// DELAY for pressure stable
					PNA_wait_and_process_sync(global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data);
#else
					DWT_Delay_ms(global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data); // waitting time
#endif
				}
				break;
			}
			case HIGH_PUSH:
			{
				if((global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data != 0) &&
						global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data != 0)
				{
					//==================================================================================
					__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
					pushdown_HighPressure_Enable();
					DWT_Delay_ms(global_variable.manual_run.u16tb_procs_time[u8_counter_state].Data); // process time
					pushdown_HighPressure_Disable();
#ifdef SYNO24_PNA
					global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Byte[0];
					global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Byte[1];
					uart_send_Feedback_Status_Run(1);
					// DELAY for pressure stable
					PNA_wait_and_process_sync(global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data);
#else
					DWT_Delay_ms(global_variable.manual_run.u16tb_waitting_after_time[u8_counter_state].Data); // waitting time
#endif
				}
				break;
			}
			default:
			{
				break;
			}
			} // end switch
		}// end for
	} // end else
	Stepper_Z1_move(Z_POSITION_NORMAL);
}

//==================================================================================================
void buzzer_blink()
{
	HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, SET);
	DWT_Delay_ms(60);
	HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, RESET);
	DWT_Delay_ms(50);
	HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, SET);
	DWT_Delay_ms(60);
	HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, RESET);
}
void HORN_ON()
{
	CONTROL_STATE[0] = 1;
}

void HORN_OFF()
{
	CONTROL_STATE[0] = 0;
	HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, RESET);
}
void hornControl()
{
	if (CONTROL_STATE[0] == 1) {
		if ((HAL_GetTick() - hornStartTime) >= HORN_TOGGLE_INTERVAL) {
			HAL_GPIO_TogglePin(buzzer_GPIO_Port, buzzer_Pin);
			hornStartTime = HAL_GetTick();
		}

	} else {
		HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, RESET);
	}
}

void buzzer_play()
{
	//	//HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, SET);
	//	__HAL_TIM_SetCompare(&htim12,TIM_CHANNEL_2, 50);
	//	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);

}
void buzzer_mute()
{
	HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_2);
	__HAL_TIM_SetCompare(&htim12,TIM_CHANNEL_2, 0);
	//HAL_GPIO_WritePin(buzzer_GPIO_Port, buzzer_Pin, RESET);
}
//================================================================================================================
void UART_Send_Command_SW()
{
	HAL_TIM_Base_Stop_IT(&htim14);
	memset(&global_variable.UART_Command.u8_Data_Rx_Buffer[0], 0, UART_LENGTH_COMMAND_RX);
	HAL_UART_Transmit(&huart1, &global_variable.UART_Command.u8_Data_Tx_Buffer[0], UART_LENGTH_COMMAND_TX, 5000);
	HAL_TIM_Base_Start_IT(&htim14);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		global_variable.UART_Command.b_FW_Rx_Command_Flag  = true; // have command from software
		HAL_UART_Receive_DMA(&huart1, &global_variable.UART_Command.u8_Data_Rx_Buffer[0], UART_LENGTH_COMMAND_RX); // copy data to buffer
	}
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == htim14.Instance)
	{
		if(global_variable.signal_running.b_signal_runing_oligo)
		{
			global_variable.control_air.u16_counter_2second ++;
			if(global_variable.control_air.u16_counter_2second >= 60)
			{
				global_variable.signal_running.b_signal_update_status = true;
			}
		}
		if(global_variable.control_air.u16_counter_30second >= 0)
		{
			global_variable.control_air.u16_counter_30second--;
		}
	}
}


void START_OLIGO_SYNTHETIC()
{
	Syno24_get_and_auto_control_Humidity();
	uint16_t X_Pos = 0;
	uint16_t u16_time_fill = 100;
	const uint16_t offsetX = OFFSET_X_PRIMMING; //offet primming
	static int8_t column_x;
	static int8_t row_y;
	static uint8_t u8_idx_fnc_mix;
	static uint8_t u8_position_x; // bien tam de doi chieu bơm hoa chat
	static uint8_t u8_position_y;
	static uint8_t u8_chemical_temp;
	static uint16_t u8_volume_temp;
	u8_position_y = 0;
	column_x = 0;
	row_y = 0;
	u8_idx_fnc_mix = 0;
	u8_position_y = 0;
	// BAT DAU BOM HOA CHAT
	Stepper_Z1_move(Z_POSITION_NORMAL);
	Stepper_Z1_Run2Normal();
	// tat ngat nhan
	//HAL_TIM_Base_Stop_IT(&htim14);
	if(global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == COUPLING ||
			global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == CAPPING ||
			global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == DMT_OFF)
	{
#ifdef SYNO24_PNA

#else
		// enable primming bo hoa chat loi
		// primming amidite 02.09.2024 tinh nang nay tranh hoa chat hu hong
		// 07/09/2024
		if(global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == COUPLING)
		{
			// tu dong primming base 0
			if(global_variable.signal_running.u16_counter_base_finished == 0)
			{
				// 11-07-2025 GiangLH auto primming thay doi funtionc xong
				auto_primming(&global_variable, 30, A1, A4);
				auto_primming(&global_variable, 30, T1, T4);
				auto_primming(&global_variable, 30, G1, G4);
				auto_primming(&global_variable, 30, C1, C4);
				auto_primming(&global_variable, 30, ACTI1, ACTI4);
			}

			Finish_Trityl_Collection();
			autoPrimming_beforeCoupling();
			Stepper_Z1_Run2Normal();
			//Finish_Trityl_Collection();
		}
		// truoc ngay 02.09.2024
		// xu ly special base
		if(global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical == COUPLING  && global_variable.synthetic_oligo.isSpecialBase == true)
		{
			// Special base Scale volume and scale time
			for( u8_idx_fnc_mix = 0; u8_idx_fnc_mix < 3; u8_idx_fnc_mix++)
			{
				global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[u8_idx_fnc_mix].Data
				= global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[u8_idx_fnc_mix].Data * global_variable.synthetic_oligo.u16_scale_volume.Data / 100;
			}
			// tang thoi gian doi sau khi bom hoa chat
			global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data =
					global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data * global_variable.synthetic_oligo.u16_scale_time.Data / 100;
			// tang thoi gian xu ly
			for(uint8_t counter_state = 0; counter_state < 10; counter_state++)
			{
				global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data =
						global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data * global_variable.synthetic_oligo.u16_scale_time.Data /100 ;
			}
		}
#endif
		u8_index_well = 0;
		u8_idx_fnc_mix = 0;
		for( u8_idx_fnc_mix = 0; u8_idx_fnc_mix < 3; u8_idx_fnc_mix++)
		{
			if(global_variable.synthetic_oligo.fill_chemical.mix_funtion.u8_type_chemical[u8_idx_fnc_mix] == AMIDITE)
			{
				// XU LY AMIDITE
				process_well_sequences(&global_variable, &global_variable.synthetic_oligo.u8_well_sequence,
						global_variable.synthetic_oligo.u16_volume_func_mix_well[0].Data);
				//Amidite_process(&global_variable, u8_idx_fnc_mix);
				Amidite_process_update(&global_variable, u8_idx_fnc_mix);
			}
			else // MIXED FUNTION khong phai amidite
			{
				column_x = 0;
				row_y = 0;
				u8_chemical_temp = CHEMICAL_SUBTANCE_EMPTY;
				u8_volume_temp = 0;
				// kiem tra loai hoa chat
				u8_chemical_temp = global_variable.synthetic_oligo.fill_chemical.mix_funtion.u8_type_chemical[u8_idx_fnc_mix];
				u8_volume_temp = global_variable.synthetic_oligo.fill_chemical.mix_funtion.u16tb_Volume[u8_idx_fnc_mix].Data;
				switch(u8_chemical_temp)
				{
				case A:
				case T:
				case G:
				case C:
				case U:
				case I:
					for(row_y = 0; row_y < MAX_ROW_Y; row_y++ ) // chay 4 hang
					{
						if(row_y % 2 == 0)
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								Chemical_fill_process_gr1valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
							}
						}
						else
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								u8_position_x = MAX_COLUMN_X - 1 - column_x;
								Chemical_fill_process_gr1valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
							}
						}
					}
					break;
				case Activator:
					// GiangLh22 27-06-2025 chinh sua lai cach di chuyen back lai ngay 28-06-2025
					row_y = 1;
					while(row_y >= 0)
					{
						if(row_y % 2 == 0)
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								// 12-03-25 comment thu cach chay moi
								Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
								// 04-06-2024 them delay cho Activator khi chay tranh  vang hoa chat
							}
						}
						else
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								u8_position_x = MAX_COLUMN_X - 1 - column_x;
								// 12-03-25 comment thu cach chay moi
								Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
								// 04-06-2024 them delay cho Activator khi chay tranh  vang hoa chat
							}
						}
						row_y--;
					}


					// debug pause in fill chemical
					//					if(global_variable.signal_running.u16_counter_base_finished == 1)
					//					{
					//						//Stepper_Z1_Run2Normal();
					//						Stepper_Z1_move(Z_POSITION_NORMAL);
					//						Stepper_AutoHome_SYN024();
					//						buzzer_blink();
					//					}
					//					if(global_variable.signal_running.u16_counter_base_finished == 4)
					//					{
					//						//Stepper_Z1_Run2Normal();
					//						Stepper_Z1_move(Z_POSITION_NORMAL);
					//						Stepper_AutoHome_SYN024();
					//						buzzer_blink();
					//					}
					break;
				case TCA_in_DCM:
				case WASH_ACN_DCM:
				case OXIDATION_IODINE:
				case CAPPING_CAPA:
				case CAPPING_CAPB:
					// 07/09/2024 tinh nang autocheck WASHING -
					if(global_variable.advanced_setting.flag_auto_primming_chemical == true
							&& u8_chemical_temp == WASH_ACN_DCM
							&& global_variable.signal_running.u16_counter_step == 1)
					{
						X_Pos = Fill_Position_X[8][0]+ offsetX;
						Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
						Stepper_Z1_move(Z_POSITION_PRIMMING);
						for(uint8_t i = WASH1; i<= WASH4; i++)
						{
							Valve_Set(i);
						}
						u16_time_fill = valve_calculator_timefill(&global_variable, WASH1 , 60);
						DWT_Delay_ms(u16_time_fill);
						Valve_DisAll();
					}
					// bom hoa chat theo chu trinh tong hop
					for(row_y = 0; row_y < 2; row_y++ ) // chay 4 hang
					{
						if(row_y % 2 == 0)
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
							}
						}
						else
						{
							for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
							{
								u8_position_x = MAX_COLUMN_X - 1 - column_x;
								Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
							}
						}
					}
					break;
				default:
					break;
				} // end switch
			} // END IF Khong  phai  AMIDITE
		} // ket thuc function mix
	}// neu lua chon hoa chat la coupling va Cap
	else // KHONG PHAI COUPLING va CAP // DMT OFF
	{
		column_x = 0;
		row_y = 0;
		u8_chemical_temp = CHEMICAL_SUBTANCE_EMPTY;
		u8_volume_temp = 0;
		// kiem tra loai hoa chat
		u8_chemical_temp = global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical;
		u8_volume_temp = global_variable.synthetic_oligo.fill_chemical.u16tb_Volume.Data;
		switch(u8_chemical_temp)
		{
		case A:
		case T:
		case G:
		case C:
		case U:
		case I:
			for(row_y = 0; row_y < 8; row_y++ ) // chay 4 hang
			{
				if(row_y % 2 == 0)
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						Chemical_fill_process_gr1valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						Chemical_fill_process_gr1valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
		case Activator:
		case TCA_in_DCM:
			if(global_variable.advanced_setting.flag_speacial_volume_trityl== true)
			{
				global_variable.advanced_setting.flag_speacial_volume_trityl = false;// resset flag
				u8_volume_temp = global_variable.advanced_setting.volume_trityl_collection.Data;// lay volume do nguoi dung cai dat
				//HORN_OFF();
			} // neu khong thi da lay volume o tren roi
			HORN_OFF();
			for(row_y = 0; row_y < 2; row_y++ ) // chay 4 hang
			{
				if(row_y % 2 == 0)
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
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
						Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, column_x, row_y, &global_variable);
					}
				}
				else
				{
					for(column_x = 0; column_x < MAX_COLUMN_X; column_x++ ) // chay 3 COT -- toa độ X
					{
						u8_position_x = MAX_COLUMN_X - 1 - column_x;
						Chemical_fill_process_gr4valve(u8_chemical_temp, u8_volume_temp, u8_position_x, row_y, &global_variable);
					}
				}
			}
			break;
		}
		//
		if(global_variable.advanced_setting.flag_vacumm_waste == true && u8_chemical_temp == WASH_ACN_DCM)
		{
			HAL_GPIO_WritePin(GPIO_Extend[1].Port, GPIO_Extend[1].Pin, SET);
			HAL_Delay(5000);
			HAL_GPIO_WritePin(GPIO_Extend[1].Port, GPIO_Extend[1].Pin, RESET);
		}
	}// ket thuc bom hoa chat khong phai Coupling
	FillChemistryWellDone(&global_variable);
	// ==================== KET THUC CHU TRINH BOM HOA CHAT =======================================================================
#ifdef SYNO24_PNA
	global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Byte[0];
	global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Byte[1];
	uart_send_Feedback_Status_Run(1);
	PNA_wait_and_process_sync(global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data);
#else
	//HAL_TIM_Base_Start_IT(&htim14); // mo lai ngat de update thong tin len software
	// go to safe state
	// neu khong co thoi gian cho thi khong can toi safe state
	if(global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data > 0)
	{
		Stepper_move_Coordinates_XY( X_POSITION_PUSH_DOWN, Y_POSITION_PUSH_DOWN);
		Stepper_Z1_move((Z_POSITION_PUSH_DOWN -20));
		HAL_Delay(100);
	}
	// DOI SAU KHI BOM
	uart_send_Feedback_Status_Run(0, WAIT_AFTERFILL); // update function  = 0 count = 0 la state bom va doi
	DWT_Delay_ms(global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data);
	// open FAN IN BOX
	FeatureVacuumBox();
#endif
	// xu ly day hoa chat va thoi gian doi
	for(uint8_t counter_state = 0; counter_state < 10; counter_state++)
	{
		// kiem tra va dieu khien do am
		Syno24_get_and_auto_control_Humidity();
		switch(global_variable.synthetic_oligo.control_pressure.u8_option_pressure[counter_state])
		{
		case HIGH_VACUUM:
		{
			if((global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0) &&
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0)
			{
				/*
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // TAT DUONG XA KHI
				HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, SET); // MƠ KHI CB
				HAL_GPIO_WritePin(SOLENOID_3_GPIO_Port, SOLENOID_3_Pin, RESET);
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 99);
				HAL_Delay(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data); // waitting time
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, SET); // MO DUONG XA KHI

				// thoi gian doi xa khi
				//update_status_and_sensor_to_SW(); // update humidity and temp
#ifdef SYNO24_PNA
				PNA_wait_and_process_sync(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data);
#else
				HAL_Delay(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data); // waitting time
#endif
				HAL_GPIO_WritePin(CTRL_8_GPIO_Port, CTRL_8_Pin, RESET);
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // DONG DUONG XA KHI
				HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, RESET); // DONG KHI CB VACUUM
				 */
			}
			break;
		}
		case LOW_VACUUM:
		{
			if((global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0) &&
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0)
			{
				/*
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // TAT DUONG XA KHI
				HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, SET);
				HAL_GPIO_WritePin(SOLENOID_3_GPIO_Port, SOLENOID_3_Pin, RESET);
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 20);
				HAL_Delay(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data); // waitting time
				__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, SET); // MO DUONG XA KHI
				// thoi gian doi xa khi
				update_status_and_sensor_to_SW();// update humidity and temp
				//HAL_Delay(500);
#ifdef SYNO24_PNA
				//uart_send_Feedback_Status_Run(1);
				PNA_wait_and_process_sync(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data);
#else
				HAL_Delay(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data); // waitting time
#endif
				HAL_GPIO_WritePin(SOLENOID_2_GPIO_Port, SOLENOID_2_Pin, RESET); // DONG DUONG XA KHI
				HAL_GPIO_WritePin(SOLENOID_1_GPIO_Port, SOLENOID_1_Pin, RESET); // DONG KHI CB VACUUM
				 */
			}
			break;
		}
		case LOW_PUSH:
		{
			if((global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0) &&
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0)
			{
				// 28-03 chua test them vao de di chuyen Z truoc khi di chuyen XY
				//Stepper_Z1_Normal();
				//__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
				pushdown_LowPressure_Enable();
				//DNA_wait_time(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data);
				// 14-04-2025
				uart_send_Feedback_Status_Run(counter_state, PUSHDOWN_FNC); // update function progress for software ui
				DWT_Delay_ms(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data); // waitting time
				pushdown_LowPressure_Disable();
#ifdef SYNO24_PNA
				global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Byte[0];
				global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Byte[1];
				// SEND FOR START CLOCKDOWN COUNTER
				uart_send_Feedback_Status_Run(1);
				// DELAY for pressure stable
				PNA_wait_and_process_sync(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data);
#else
				DNA_wait_time(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data); // waitting time
				//FeatureVacuumBox();
#endif
			}
			break;
		}
		case HIGH_PUSH:
		{
			if((global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0) &&
					global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data != 0)
			{
				// 28-03 chua test them vao de di chuyen Z truoc khi di chuyen XY
				//Stepper_Z1_Normal();
				//__HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, 0);
				pushdown_HighPressure_Enable();
				//DNA_wait_time(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data);
				uart_send_Feedback_Status_Run(counter_state, PUSHDOWN_FNC); // update function progress for software ui
				DWT_Delay_ms(global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[counter_state].Data); // waitting time
				pushdown_HighPressure_Disable();
#ifdef SYNO24_PNA
				global_variable.UART_Command.u8_Data_Tx_Buffer[2] = global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Byte[0];
				global_variable.UART_Command.u8_Data_Tx_Buffer[3] = global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Byte[1];
				// SEND FOR START CLOCKDOWN COUNTER
				uart_send_Feedback_Status_Run(1);
				// DELAY for pressure stable
				PNA_wait_and_process_sync(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data);
#else
				DNA_wait_time(global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[counter_state].Data); // waitting time
				//FeatureVacuumBox();
#endif
			}
			break;
		}
		default:
		{
			break;
		}
		} // end switch process and wait
	} // end 10 function process and wait
	// xu ly Vacuum sau khi doi
	/*
	 */
	DisableVacuumBox(); // tat quat hut trong hop dam bao tat khi het step
}

void STOP_OLIGO()
{

}
/***
 * @LeHoaiGiang
 * @AutoPrimming amidite and activator
 * @input1 : none
 * @input2 : none
 */
void autoPrimming_beforeCoupling()
{
	if(global_variable.advanced_setting.flag_auto_primming_chemical == true)
	{
		// primming ammidite
		// can dua len software
		const uint16_t offsetX = OFFSET_X_PRIMMING;
		uint16_t X_Pos = 0;
		uint16_t u16_time_fill = 100;
		X_Pos = Fill_Position_X[0][0] + offsetX;
		Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		for(uint8_t i = A1; i<= C1; i++)// xa bo amidite
		{
			Valve_Set(i);
		}
		u16_time_fill = valve_calculator_timefill(&global_variable, A1 , global_variable.advanced_setting.u16tb_autoPrim_volume_amidite.Data);
		DWT_Delay_ms(u16_time_fill);
		Valve_DisAll();
		// primming activator
		X_Pos = Fill_Position_X[4][0]+ offsetX;
		Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		for(uint8_t i = ACTI1; i <= ACTI4; i++)
		{
			Valve_Set(i);
		}
		u16_time_fill = valve_calculator_timefill(&global_variable, ACTI1 , global_variable.advanced_setting.u16tb_autoPrim_volume_Activator.Data);
		DWT_Delay_ms(u16_time_fill);
		Valve_DisAll();
		//global_variable.advanced_setting.flag_auto_primming_chemical = false;
	}
}

/***
 * @LeHoaiGiang
 * @AutoPrimming amidite and activator
 * @input1 : chemical
 * @input2 : volume
 */
void autoPrimming(uint8_t chemical, uint16_t volume)
{
	uint16_t X_Pos = 0;
	const uint16_t offsetX = OFFSET_X_PRIMMING;// 25-03-2025
	uint16_t u16_time_fill;
	//Stepper_Z1_move(Z_POSITION_PRIMMING);
	switch(chemical)
	{
	case A1:
	case T1:
	case G1:
	case C1:
		X_Pos = Fill_Position_X[0][0] + offsetX;
		Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		break;
	case F1:
	case F2:
		X_Pos = Fill_Position_X[4][0]+ offsetX;
		Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		//Calib_volume(p_global_variable->primming_control.u8_valve_sellect, p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case ACTI1:
	case ACTI2:
	case ACTI3:
	case ACTI4:
		X_Pos = Fill_Position_X[6][0]+ offsetX;
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		for(uint8_t i = ACTI1; i <= ACTI4; i++)
		{
			Valve_Set(i);
		}
		u16_time_fill = valve_calculator_timefill(&global_variable, ACTI1 , volume);
		DWT_Delay_ms(u16_time_fill);
		Valve_DisAll();
		//Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		//Calib_volume(p_global_variable->primming_control.u8_valve_sellect, p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case TCA1:
	case TCA2:
	case TCA3:
	case TCA4:
		X_Pos = Fill_Position_X[6][0]+ offsetX;
		Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		//Calib_volume(p_global_variable->primming_control.u8_valve_sellect, p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case WASH1:
	case WASH2:
	case WASH3:
	case WASH4:
	case OX1:
	case OX2:
	case OX3:
	case OX4:
		X_Pos = Fill_Position_X[8][0]+ offsetX;
		Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		//Calib_volume(p_global_variable->primming_control.u8_valve_sellect, p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	case CAPB1:
	case CAPB2:
	case CAPB3:
	case CAPB4:
	case CAPA1:
	case CAPA2:
	case CAPA3:
	case CAPA4:
		X_Pos = Fill_Position_X[10][0]+ offsetX;
		Calib_volume(chemical, volume, X_Pos, Y_CALIB_POS);
		//Calib_volume(p_global_variable->primming_control.u8_valve_sellect, p_global_variable->primming_control.u32fb_time_primming_calib.Data, X_Pos, Y_CALIB_POS);
		break;
	default:
		break;
	}
}

/***
 * 16-11-2024
 * Function Go to position insert plate
 *
 *
 */
void Process_Trityl_Collection()
{
	Stepper_Z1_Run2Normal();
	Stepper_move_Coordinates_XY(X_INSERT_PLATE, Y_INSERT_PLATE);
	global_variable.signal_running.b_signal_last_state_trityl = true;
}

/***
 * Finish_Trityl_Collection
 * function xử lý kết thúc thu DMT, đợi độ ẩm đạt giá trị đã cài đặt mới cho tiếp tục chạy Coupling
 *
 */
void Finish_Trityl_Collection()
{
	Get_sensor();
	if(global_variable.signal_running.b_signal_last_state_trityl == true) // nếu có thu thì xử lý đợi và tự động primming
	{
		pushdown_LowPressure_Enable();
		for(int i = 0; i < 20; i++)
		{
			HAL_Delay(250);
			Get_sensor();
		}
		pushdown_LowPressure_Disable();
		Stepper_Z1_Run2Normal();
		HAL_Delay(250);
		global_variable.signal_running.b_signal_last_state_trityl = false;
		if(global_variable.status_and_sensor.flag_enable_auto_control_air_Nito == SET) // nếu có set limit độ ẩm
		{
			while(global_variable.status_and_sensor.f_humidity > global_variable.status_and_sensor.u8_high_limit_humidity)
			{
				Get_sensor();
				syno24_Control_Air_Humidity();
				HAL_Delay(50);
			}
		}
		Stepper_Z1_Run2Normal();
		// Tu dong prmming amidte va activator edit 25-03-25
		const uint16_t offsetX = OFFSET_X_PRIMMING;
		uint16_t X_Pos = 0;
		uint16_t u16_time_fill = 100;
		X_Pos = Fill_Position_X[0][0] + offsetX;
		Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		for(uint8_t i = A1; i<= C1; i++)// xa bo amidite
		{
			Valve_Set(i);
		}
		if(global_variable.advanced_setting.u16tb_autoPrim_volume_amidite.Data > 0)
		{
			u16_time_fill = valve_calculator_timefill(&global_variable, A1 , global_variable.advanced_setting.u16tb_autoPrim_volume_amidite.Data);
		}
		else
		{
			u16_time_fill = valve_calculator_timefill(&global_variable, A1 , 15); // mac dinh bom 15ul
		}
		DWT_Delay_ms(u16_time_fill);
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		Valve_DisAll();
		// primming activator
		X_Pos = Fill_Position_X[4][0]+ offsetX;
		Stepper_move_Coordinates_XY(X_Pos, Y_CALIB_POS);
		Stepper_Z1_move(Z_POSITION_PRIMMING);
		for(uint8_t i = ACTI1; i <= ACTI4; i++)
		{
			Valve_Set(i);
		}
		if(global_variable.advanced_setting.u16tb_autoPrim_volume_Activator.Data > 0)
		{
			u16_time_fill = valve_calculator_timefill(&global_variable, ACTI1 , global_variable.advanced_setting.u16tb_autoPrim_volume_Activator.Data);
		}
		else
		{
			u16_time_fill = valve_calculator_timefill(&global_variable, ACTI1 , 15);// mac dinh bom 15ul
		}
		DWT_Delay_ms(u16_time_fill);
		Valve_DisAll();
		Stepper_Z1_Run2Normal();
	}
}

//==================================================================================================================
void pushdown_LowPressure_Enable()
{
	Stepper_move_Coordinates_XY( X_POSITION_PUSH_DOWN, Y_POSITION_PUSH_DOWN);
	Stepper_Z1_move(Z_POSITION_PUSH_DOWN);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET);
	//{GPIOB, GPIO_PIN_14},//v2
	//{GPIOB, GPIO_PIN_11},//v4
	//{GPIOB, GPIO_PIN_10},//v6
}
//===================================================================================================================
void pushdown_LowPressure_Disable()
{
	//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET);
	//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, RESET);
}
//====================================================================================================================
void pushdown_HighPressure_Enable()
{
	Stepper_move_Coordinates_XY( X_POSITION_PUSH_DOWN, Y_POSITION_PUSH_DOWN);
	Stepper_Z1_move(Z_POSITION_PUSH_DOWN);
	HAL_Delay(100);
	//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, SET); //09-10-2025 tat valve lowpush  de test
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10,SET);
}
//====================================================================================================================
void pushdown_HighPressure_Disable()
{
	//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, RESET); //09-10-2025 tat valve lowpush  de test
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, RESET);
}

void EnableVacuumBox(uint16_t timeOpen)
{
	// 09-06-2025 quan ly dong mo bang thu vien moi viet
	//	HAL_GPIO_WritePin(SOLENOID[FAN_VACUUM_BOX].Port, SOLENOID[FAN_VACUUM_BOX].Pin, SET);
	//	HAL_Delay(timeOpen * 1000);
	//	HAL_GPIO_WritePin(SOLENOID[FAN_VACUUM_BOX].Port, SOLENOID[FAN_VACUUM_BOX].Pin, RESET);
	// gianglh22
	GPIO_Timer_turnOn(&FanInBox, timeOpen * 1000);

}
void DisableVacuumBox()
{
	// 09-06-2025 quan ly dong mo bang thu vien moi viet
	// gianglh22
	GPIO_Timer_turnOff(&FanInBox);

}

void UpdateVacuumBox()
{
	GPIO_Timer_update(&FanInBox);
}
//=====================================================================================================================
void FeatureVacuumBox()
{
	if(global_variable.advanced_setting.VacuumBox.Enablefeature)
	{
		switch(global_variable.synthetic_oligo.fill_chemical.u8_first_type_chemical)
		{
		case WASH_ACN_DCM:
		{
			if(global_variable.advanced_setting.VacuumBox.En_WASH)
			{
				EnableVacuumBox(global_variable.advanced_setting.VacuumBox.time.Data);
			}
			break;
		}
		case TCA_in_DCM:
		{
			if(global_variable.advanced_setting.VacuumBox.En_Deblock)
			{
				EnableVacuumBox(global_variable.advanced_setting.VacuumBox.time.Data);
			}
			break;
		}
		case COUPLING:
		{
			if(global_variable.advanced_setting.VacuumBox.En_Coupling)
			{
				EnableVacuumBox(global_variable.advanced_setting.VacuumBox.time.Data);
			}
			break;
		}
		case CAPPING:
		{
			if(global_variable.advanced_setting.VacuumBox.En_Cap)
			{
				EnableVacuumBox(global_variable.advanced_setting.VacuumBox.time.Data);
			}
			break;
		}
		case OXIDATION_IODINE:
		{
			if(global_variable.advanced_setting.VacuumBox.En_Ox)
			{
				EnableVacuumBox(global_variable.advanced_setting.VacuumBox.time.Data);
			}
			break;
		}
		default :
			break;
		}
	}
}
void Syno24_get_and_auto_control_Humidity()
{
	//ADS1115_Read_A0(&f_Volt_T);
	//ADS1115_Read_A1(&);

	//	if(ADS1115_readSingleEnded(ADS1115_MUX_AIN0, &f_Volt_T) == HAL_OK){
	//		//Voltage successfully read.
	//		global_variable.status_and_sensor.f_temperature = mapFloat(f_Volt_T, 0, 5000, -40, 80);
	//		global_variable.status_and_sensor.u16_temperature.Data = global_variable.status_and_sensor.f_temperature * 100;
	//	}
	//
	//	if(ADS1115_readSingleEnded(ADS1115_MUX_AIN1, &f_Volt_H) == HAL_OK){
	//		//Voltage successfully read.
	//		global_variable.status_and_sensor.f_humidity = mapFloat(f_Volt_H, 0, 5000, 0 , 100);
	//		global_variable.status_and_sensor.u16_humidity.Data = global_variable.status_and_sensor.f_humidity * 100;
	//	}
	//	if(ADS1115_Read_A0(&f_Volt_T)== HAL_OK){
	//		//Voltage successfully read.
	//		global_variable.status_and_sensor.f_temperature = mapFloat(f_Volt_T, 0, 5, -40, 80);
	//		global_variable.status_and_sensor.u16_temperature.Data = global_variable.status_and_sensor.f_temperature * 100;
	//	}
	//	HAL_Delay(5);
	//	if(ADS1115_Read_A1(&f_Volt_H) == HAL_OK){
	//		//Voltage successfully read.
	//		global_variable.status_and_sensor.f_humidity = mapFloat(f_Volt_H, 0, 5, 0 , 100);
	//		global_variable.status_and_sensor.u16_humidity.Data = global_variable.status_and_sensor.f_humidity * 100;
	//	}
	Get_sensor();
	syno24_Control_Air_Humidity();
}
void syno24_Control_Air_Humidity()
{
	if(global_variable.control_air.b_enable_clean_box == true && global_variable.control_air.u16_counter_30second > 0)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, SET);
	}
	else
	{
		if(global_variable.control_air.u16_counter_30second <= 0)
		{
			global_variable.control_air.b_enable_clean_box = false;
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, RESET);
		}
	}
	if(global_variable.control_air.b_enable_clean_box == false)
	{
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, RESET);
		if(     global_variable.signal_running.b_signal_runing_oligo == true
				&& global_variable.status_and_sensor.flag_enable_auto_control_air_Nito == SET
		)
		{
			if(global_variable.status_and_sensor.f_humidity >= global_variable.status_and_sensor.u8_high_limit_humidity)
			{
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, SET);
			}
			else
			{
				if(global_variable.status_and_sensor.f_humidity <= global_variable.status_and_sensor.u8_low_limit_humidity)
				{
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);
				}
			}
		}
		else
		{
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, RESET);
		}
	}
}
/*
 * *************************************************************************************************************
 * GIANGLH FUNCTION
 * send feedback status of machine to software
 * *************************************************************************************************************
 */
/*
 * *************************************************************************************************************
 * GIANGLH FUNCTION uart_send_Feedback_Status_Run GiangLH22 update 05-05-2025
 * *************************************************************************************************************
 */
void uart_send_Feedback_Status_Run(uint8_t u8_function_count, uint8_t u8_subfunction_run)
{
	global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_FEEDBACK_STATUS_RUN;
	global_variable.UART_Command.u8_Data_Tx_Buffer[1] =	u8_function_count; //
	global_variable.UART_Command.u8_Data_Tx_Buffer[2] =	u8_subfunction_run; //
	if(u8_subfunction_run == 0)
	{
		global_variable.UART_Command.u8_Data_Tx_Buffer[3] = 0;
		global_variable.UART_Command.u8_Data_Tx_Buffer[4] = 0;
		global_variable.UART_Command.u8_Data_Tx_Buffer[5] = (global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data) & 0xFF;
		global_variable.UART_Command.u8_Data_Tx_Buffer[6] = (global_variable.synthetic_oligo.fill_chemical.u16tb_wait_after_fill.Data >> 8)&0xFF;
	}
	else
	{
		global_variable.UART_Command.u8_Data_Tx_Buffer[3] = (global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[u8_function_count].Data) & 0xFF;
		global_variable.UART_Command.u8_Data_Tx_Buffer[4] = (global_variable.synthetic_oligo.control_pressure.u16tb_procs_time[u8_function_count].Data >> 8)&0xFF;
		global_variable.UART_Command.u8_Data_Tx_Buffer[5] = (global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[u8_function_count].Data) & 0xFF;
		global_variable.UART_Command.u8_Data_Tx_Buffer[6] = (global_variable.synthetic_oligo.control_pressure.u16tb_waitting_after_time[u8_function_count].Data >> 8)&0xFF;
	}
	HAL_UART_Transmit_DMA(&huart1, &global_variable.UART_Command.u8_Data_Tx_Buffer[0], UART_LENGTH_COMMAND_TX);
}
//=======================================================================================================================
void send_status_machine()
{

}
//=======================================================================================================================

void Get_sensor()
{
	if(ADS1115_Read_A0(&f_Volt_T)== HAL_OK){
		//Voltage successfully read.
		global_variable.status_and_sensor.f_temperature = mapFloat(f_Volt_T, 0, 5, -40, 80);
		global_variable.status_and_sensor.u16_temperature.Data = global_variable.status_and_sensor.f_temperature * 100;
		global_variable.status_and_sensor.Tcounter_error = 0;
	}
	else
	{
		global_variable.status_and_sensor.Tcounter_error++;
	}

	if(ADS1115_Read_A1(&f_Volt_H) == HAL_OK){
		//Voltage successfully read.
		global_variable.status_and_sensor.f_humidity = mapFloat(f_Volt_H, 0, 5, 0 , 100);
		global_variable.status_and_sensor.u16_humidity.Data = global_variable.status_and_sensor.f_humidity * 100;
		global_variable.status_and_sensor.Hcounter_error =0;
	}
	else
	{
		global_variable.status_and_sensor.Hcounter_error++;
	}
	if(global_variable.status_and_sensor.Hcounter_error > 10 || global_variable.status_and_sensor.Tcounter_error > 10)
	{
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		HAL_I2C_DeInit(&hi2c1);
		HAL_Delay(200);
		MX_I2C1_Init();
		HAL_Delay(200);
		//global_variable.status_and_sensor.Hcounter_error = 0;
		//global_variable.status_and_sensor.Tcounter_error = 0;
	}

	//	global_variable.status_and_sensor.f_temperature = mapFloat(f_Volt_T, 0, 5, -40, 80);
	//	global_variable.status_and_sensor.f_humidity = mapFloat(f_Volt_H, 0, 5, 0 , 100);
	//	global_variable.status_and_sensor.u16_temperature.Data = global_variable.status_and_sensor.f_temperature * 100;
	//	global_variable.status_and_sensor.u16_humidity.Data = global_variable.status_and_sensor.f_humidity * 100;
#ifdef UART_DEBUG
	printf("debug Temperature %f \r\n", Temperature);
	printf("debug Humidity %f \r\n", Humidity);
#endif
}
//==================================================================================
HAL_StatusTypeDef GetHumidityTemparature(float *temperature, float *humidity)
{
	HAL_StatusTypeDef status_T = ADS1115_Read_A0(&f_Volt_T);
	HAL_StatusTypeDef status_H = ADS1115_Read_A1(&f_Volt_H);

	if (status_T == HAL_OK)
	{
		*temperature = mapFloat(f_Volt_T, 0, 5, -40, 80);
		global_variable.status_and_sensor.u16_temperature.Data = (*temperature) * 100;
		global_variable.status_and_sensor.Tcounter_error = 0;
	}
	else
	{
		global_variable.status_and_sensor.Tcounter_error++;
	}
	if (status_H == HAL_OK)
	{
		*humidity = mapFloat(f_Volt_H, 0, 5, 0, 100);
		global_variable.status_and_sensor.u16_humidity.Data = (*humidity) * 100;
		global_variable.status_and_sensor.Hcounter_error = 0;
	}
	else
	{
		global_variable.status_and_sensor.Hcounter_error++;
	}

	// Xử lý lỗi nếu số lần thất bại vượt ngưỡng
	if (global_variable.status_and_sensor.Tcounter_error > 10 || global_variable.status_and_sensor.Hcounter_error > 10)
	{
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		buzzer_blink();
		HAL_Delay(10);
		HAL_I2C_DeInit(&hi2c1);
		HAL_Delay(200);
		MX_I2C1_Init();
		HAL_Delay(200);
	}
	// Trả v�? trạng thái tổng quan
	if (status_T == HAL_OK && status_H == HAL_OK)
	{
		return HAL_OK;
	}
	else
	{
		return HAL_ERROR;
	}
}

/*
 * 	GetHumidityTemparature_withTimeout
    float temperature = 0.0, humidity = 0.0; // Biến lưu giá trị nhiệt độ và độ ẩm
    uint32_t timeout = 5000;                // Timeout 5 giây

    // G�?i hàm Get_sensor_with_timeout
    if (Get_sensor_with_timeout(&temperature, &humidity, timeout) == HAL_OK)
    {
        // Thành công
        printf("Temperature: %.2f°C\n", temperature);
        printf("Humidity: %.2f%%\n", humidity);
    }
    else
    {
        // Lỗi hoặc timeout
        printf("Failed to read sensor data within %d ms.\n", timeout);
    }

 */

HAL_StatusTypeDef GetHumidityTemparature_withTimeout(float *temperature, float *humidity, uint32_t timeout_ms)
{
	uint32_t start_time = HAL_GetTick(); // Lấy th�?i gian bắt đầu
	HAL_StatusTypeDef status = HAL_ERROR;

	while ((HAL_GetTick() - start_time) < timeout_ms)
	{
		status = GetHumidityTemparature(temperature, humidity); // G�?i hàm Get_sensor để đ�?c dữ liệu
		if (status == HAL_OK)
		{
			return HAL_OK; // Thoát nếu đ�?c thành công
		}
		HAL_Delay(10); // �?ợi một khoảng th�?i gian nh�? trước khi thử lại
	}
	return HAL_ERROR; // Timeout, trả v�? lỗi
}


void DNA_wait_time(uint16_t u16_intervaltime_process)
{
	static uint16_t counter;
	counter = u16_intervaltime_process / 100;
	for(uint16_t time = 0; time < counter; time++)
	{

		HAL_Delay(99);// tru thoi gian doc cam bien va xu ly 100 -1ms = 99
		UpdateVacuumBox();// tu dong tat Fan in Box --  tinh nang advanced setting mo Fan Hut khi ben trong
		// check do am open valve
		Syno24_get_and_auto_control_Humidity();
	}
	HAL_Delay(u16_intervaltime_process % 100);

}
/**
 * @brief  This function is get data coordinates for Run
 * nếu nhận được dữ liệu và kiểm tra và bị sai thì get lại lần nữa
 * @retval None
 */
void GetCoordinatesFromSoftware()
{
	global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_REQUEST_COORDINATES_SW;
	UART_Send_Command_SW();
}
/*
 *
 */
/**
 * @brief  Check Data Coordinates form software
 * nếu nhận được dữ liệu và kiểm tra và bị sai thì get lại lần nữa
 * @retval None
 */
void CheckDataCoordinates()
{
	//global_variable.UART_Command.u8_Data_Tx_Buffer[0] = CMD_REQUEST_COORDINATES_SW;
	//UART_Send_Command_SW();
}


/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
