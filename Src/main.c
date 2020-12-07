/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
// 
//----ADC1-----ADC2------
//after update ADC1 ch.1 ch.3 ch.1 ch.3
//             ADC2 ch.2 ch.4 ch.2 ch.4
// channels in adc_buffer: 1,2,3,4,1,2,3,4,1,2,3,4,1,2,3,4
//FAN Thermocouple    - PA.1 Ch.1 
//FAN fan             - PA.2 Ch.2
//Solder Thermocouple - PA.3 Ch.3
//Spare channel       - PA.4. Ch.4

//Push buttons unpressed state = High level

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
#include "sEEPROM.h"
#include "button_drv.h"
#include "MicroMenu.h"
#include "sEEPROM.h"
#include "ssd1306.h"
#include "menu.h"
#include "const_var.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI2_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */
volatile void delay(volatile uint32_t cnt);
void fan_working_layout(void);
void solder_working_layout(void);
void getADC_values(void);
void regulator_solder(void);
void regulator_fan(void);
void getDiscreteInputs(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void fan_working_layout(void)
{
  progstate = FAN_E;
  
  __IO uint16_t tim1cr1 = TIM2 -> CR1;
  tim1cr1 |= 0x0060; //CMS = 11, encoder value max = arr, no rollover 
  TIM2 -> CR1 = tim1cr1;
  
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(0, 0);
  SSD1306_Puts("Fan", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(0, 16);
  SSD1306_Puts("Set: ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(50, 0);
  SSD1306_Puts("Blower: ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
}

void solder_working_layout(void)
{
 
  __IO uint16_t tim1cr1 = TIM2 -> CR1;
  tim1cr1 |= 0x0060; //CMS = 11, encoder value max = arr, no rollover 
  TIM2 -> CR1 = tim1cr1;
  __HAL_TIM_SET_COUNTER(&htim2, Solder_temp_z * 2 + 1); 
  progstate = SOLDER_E;
    
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(0, 0);
  SSD1306_printf(&segoeUI_8ptFontInfo, "Solder %d",  N_solder_tip);
  SSD1306_GotoXY(0, 16);
  SSD1306_Puts("Set: ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(100, 0);
  if(soldertype == T_12)
  {
    SSD1306_Puts("T-12", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  }
  else 
  {
    SSD1306_Puts("H-907", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  }
}


void getADC_values(void)
{
  Solder_Thermocouple_adc = 0;
  Fan_Thermocouple_adc = 0;
  Fan_fan_adc  = 0;
  Solder_H907_adc = 0;
  
  for(uint8_t i = 0; i < 16; i += 4)
  {
    Fan_Thermocouple_adc += adc_buffer[i];
    Fan_fan_adc += adc_buffer[i + 1];
    Solder_Thermocouple_adc += adc_buffer[i + 2];
    Solder_H907_adc += adc_buffer[i + 3];
  } 
  Solder_Thermocouple_adc /= 4;
  Fan_Thermocouple_adc /= 4;
  Fan_fan_adc  /= 4;
  Solder_H907_adc /= 4;
  
  //---------Normalization------------------------------------------------------
  Fan_Thermocouple_temp = (int16_t)((float)Fan_Thermocouple_adc * k_fan + b_fan) - 16 * !F_fan_gerkon; //Compensate gercon pullup resistor 1kOhm
  U_fan_temp = (float)Fan_Thermocouple_temp / 450.0f;
  U_fan_temp_z = (float)Fan_temp_z / 450.0f;
  
  if(soldertype == T_12)
  {
    Solder_Thermocouple_temp = (int16_t)((float)Solder_Thermocouple_adc * k_solder + b_solder);
  }
  else 
  {
    Solder_Thermocouple_temp = (int16_t)((float)Solder_H907_adc * k_solder + b_solder);
  }
  U_solder_temp = (float)Solder_Thermocouple_temp / 400;
  U_solder_temp_z = (float)Solder_temp_z / 400;
  
  Fan_fan_percent = (uint16_t)(100 - (2550 - Fan_fan_adc) / 24.9);
  Fan_fan_percent = MIN(Fan_fan_percent, 100);
      
}


void regulator_solder(void)
{
  Uy_solder_p = U_solder_temp - U_solder_temp_z;
  Uy_solder_p = fmaxf(Uy_solder_p, -1.25);
  Uy_solder_p = fminf(Uy_solder_p, 1.25);
  
  Uy_solder_i = (Uy_solder_i + (tp / T1) * Uy_solder_p) * F_solder;
  Uy_solder_i = fminf(Uy_solder_i, 1);
  Uy_solder_i = fmaxf(Uy_solder_i, 0);
  
  Uy_solder = Uy_solder_p * (K1_h907 * (bool)soldertype + K1_t12 * !(bool)soldertype) + Uy_solder_i;
  
  //---Preheat H907 handle. When cold there is low resistance of the solder heater.
  if((soldertype == HAKKO_907) && (U_solder_temp <= 0.25))
  {
    Uy_solder = fmaxf(Uy_solder, Solder_H907_PWM_limit);//100 degree 
  }
  
  Uy_solder = fminf(Uy_solder, 1.25);
  Uy_solder = fmaxf(Uy_solder, 0.12);
}


void regulator_fan(void)
{
  Uy_fan_p = U_fan_temp - U_fan_temp_z;
  Uy_fan_p = fmaxf(Uy_fan_p, -1.25);
  Uy_fan_p = fminf(Uy_fan_p, 1.25);
  
  Uy_fan_i = (Uy_fan_i + (tp / T2) * Uy_fan_p) * F_fan;
  Uy_fan_i = fminf(Uy_fan_i, 1);
  Uy_fan_i = fmaxf(Uy_fan_i, 0);
  
  Uy_fan = Uy_fan_p * K2 + Uy_fan_i;
  Uy_fan = fminf(Uy_fan, 1.25);
  Uy_fan = fmaxf(Uy_fan, 0.3);
}

//Get state of the fan handle gerkon and solder handle vibration switch
void getDiscreteInputs(void)
{
  bool F_fan_gerkon_tmp, F_solder_switch_tmp, F_fan_gerkon_gpio;
  static uint8_t F_fan_gerkon_cnt;
  static bool F_solder_switch_tmp_previous;
  
  //Gerkon with bounce protection 
  F_fan_gerkon_tmp = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10);
  F_fan_gerkon_cnt = (F_fan_gerkon_cnt + 1) * F_fan_gerkon_tmp;
  if(F_fan_gerkon_cnt >= DEBOUNCE)
  {
    F_fan_gerkon_cnt = DEBOUNCE;
    F_fan_gerkon_gpio = 1;
  }
  else F_fan_gerkon_gpio = 0;
  
  //----Fan switch off button-----------------------------------------------
  if(Fan_off_btn.ShortPush) 
  {
    F_fan_btn_off = !F_fan_btn_off;
  }
  
  if(fan_switch_off_source == BUTT_GERCON)
  {
    // Костыль, тк везде по коду берется переменная F_fan_gerkon, а F_fan_btn_off добавлена позднее, 
    // то результирующий сигнал выключения фена с кнопки или с геркона сводится к переменной F_fan_gerkon
    F_fan_gerkon = F_fan_gerkon_gpio & F_fan_btn_off; // Если управление феном установлено с кнопки, нажатие кнопки разрешает работу геркона, повторное нажатие делает переменную F_fan_gerkon = 0, т.е. работа фена запрещена.
  }
  else if(fan_switch_off_source == GERCON)
  {
     F_fan_gerkon = F_fan_gerkon_gpio;
  }
  else if(fan_switch_off_source == BUTTON)
  {
    F_fan_gerkon = F_fan_btn_off;
  }
  
  //  Vibration switch. Only changing of this signal is take into account.
  // Need for solder timeout
  if(soldertype == HAKKO_907) F_solder_switch = 0; //Turn off vibroswitch feature for Hakko 907 handle
  else
  {
    F_solder_switch_tmp = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);
    F_solder_switch = !(F_solder_switch_tmp ^ F_solder_switch_tmp_previous);
    F_solder_switch_tmp_previous = F_solder_switch_tmp;
  }
}

void blower_fan_manage(void)
{
  static bool F_blower_block;
  
  //Turn Off blower
  if((F_fan_gerkon == 0) && ((Fan_Thermocouple_temp <= FAN_FLUSH_TEMPERATURE) || (F_blower_block == 1)))
  {
    K_blower_flush = 1;
    K_blower_off = 0;
    F_blower_block = 1;
  }
  //Flush
  if((F_fan_gerkon == 0) && (Fan_Thermocouple_temp > FAN_FLUSH_TEMPERATURE) && (F_blower_block == 0))
  {
    K_blower_flush = 0;
    K_blower_off = 1;
  }
  //Working
  if(F_fan_gerkon == 1)
  {
    K_blower_flush = 1;
    K_blower_off = 1;
    F_blower_block = 0;
  }
  // Set outputs
  if(K_blower_flush) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
  
  if(K_blower_off) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
  else HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
}

// Main Callback from Solder PWM timer at the end of PWM period.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint16_t cnt_slowdown;
  static uint16_t Solder_timeout_cnt;

  uint16_t Fan_CCR_Load, Solder_CCR_Load;

  if (htim->Instance==TIM3) //check if the interrupt comes from TIM3(Solder PWM)
  {
    delay(4000);
    HAL_ADC_Start(&hadc1);
    getButtonState(&EncBtn);
    getButtonState(&Solder_off_btn);
    getButtonState(&Fan_off_btn);
    
    if(EncBtn.LongPush) BtnCntr_LongPush = 1;
    if(EncBtn.ShortPush) BtnCntr_ShortPush = 1;
    if(progstate == MENU_E) //Duplicate for menu
    {
      if(EncBtn.ShortPush) BtnCntr_Menu = 1;
    }
    
    getADC_values();    
    getDiscreteInputs(); //Fan gerkon and solder handle vibration switch
        
    //---------Encoder----------------------------------------------------------
    //--if encoder value is less than SOLDER_MIN_TEMP_Z or FAN_MIN_TEMP_Z, 
    // the correspinding devise is switch off
    uint16_t counter = __HAL_TIM_GET_COUNTER(&htim2);
    if(progstate == SOLDER_E)
    {    
      //--------Solder Switch_off button------------------------------------------
      if(Solder_off_btn.ShortPush) 
      {
        F_solder_btn_off = !F_solder_btn_off;
        // sEE_WriteBuffer(&hspi2, (uint8_t*)&F_solder_btn_off, EE_BTNOFF_SOLDER_ADDR, 1);
      }
      
      if(F_solder_btn_off)//If button not press
      {
        Solder_temp_z = (uint16_t)(counter / 2);
      }
      else 
      {
        __HAL_TIM_SET_COUNTER(&htim2, Solder_temp_z * 2 + 1); //Freeze encoder input
      }
      
      if(Solder_temp_z <= SOLDER_MIN_TEMP_Z)//Switch off the iron with encoder
      {
        __HAL_TIM_SET_COUNTER(&htim2, SOLDER_MIN_TEMP_Z * 2 + 1);
        F_solder_enable = 0;
        Solder_temp_z = SOLDER_MIN_TEMP_Z;
      }
      else F_solder_enable = 1;
    }
    if(progstate == FAN_E)
    {
      //---Fan switch off button management in getDiscreteInputs() 
      Fan_temp_z = (uint16_t)(counter / 2);
      if(Fan_temp_z <= FAN_MIN_TEMP_Z)//Switch off the fan with encoder
      {
        __HAL_TIM_SET_COUNTER(&htim2, FAN_MIN_TEMP_Z * 2 + 1);
        F_fan_enable = 0;
        Fan_temp_z = FAN_MIN_TEMP_Z;
      }
      else F_fan_enable = 1;
    }
    

    //-----------Solder timeout-------------------------------------------------
    // In case of Hakko907 handle F_solder_switch variable is allways become 0.
    if((Solder_temp_z - Solder_temp_z_former) != 0) F_encoder_change_value = 0;
    else F_encoder_change_value = 1; // if encoder changed its value, timeout reset
    Solder_timeout_cnt = (Solder_timeout_cnt + 1) * (F_solder_switch & F_encoder_change_value);
    if(Solder_timeout_cnt >= TimeoutTime * 100)
    {
      F_solder_timeout = 0;
      Solder_timeout_cnt = TimeoutTime * 100;
    }
    else F_solder_timeout = 1;

    
    //---------Protection from Fan thermocouple disconnection-------------------
    if(U_fan_temp > 1.05) F_fan_temp_protect = 1;
    else F_fan_temp_protect = 0;
    //---------Protection from blower stop or less than 30%---------------------
    if(Fan_fan_percent < 30)F_fan_blower_protect = 1;
    else F_fan_blower_protect = 0;
    //---------Protection from solder disconnection-----------------------------
    if(U_solder_temp > 1.075) F_solder_temp_protect = 1; //Or overheat in case of H907 handle
    else F_solder_temp_protect = 0;
    
    //---F_fan-----F_solder-----------------------------------------------------
    F_fan = F_fan_enable & F_fan_gerkon & !F_fan_temp_protect;
    F_solder = F_solder_timeout & F_solder_enable & !F_solder_temp_protect & F_solder_btn_off;

    blower_fan_manage(); //Blower flush and on/off 
    
    if((progstate == SOLDER_E) || (progstate == FAN_E)) 
    {
      regulator_solder();
      regulator_fan();
      
      Solder_CCR_Load = (uint16_t)(Uy_solder * MAX_CCR_LOAD_SOLDER + !F_solder * MAX_CCR_LOAD_SOLDER);
      Fan_CCR_Load = (uint16_t)(Uy_fan * MAX_CCR_LOAD_FAN + !F_fan * MAX_CCR_LOAD_FAN);
      
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, Solder_CCR_Load); 
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, Fan_CCR_Load); 
    }
    
    //-------LED blinking-------------------------------------------------------
    if(cnt_slowdown++>50)
    {
      cnt_slowdown = 0;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
    }
  }
}

volatile void delay(volatile uint32_t cnt)
{
  while (--cnt);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  static uint16_t cntr;
  static bool Fcntr;
  static uint8_t cnt_filter; //filtering temperature for display
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
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  
  SSD1306_Init(&hi2c1, 0x78);
  
  Button_Init(&EncBtn, GPIOB, GPIO_PIN_6);
  Button_Init(&Solder_off_btn, GPIOC, GPIO_PIN_14);  
  Button_Init(&Fan_off_btn, GPIOC, GPIO_PIN_15);
  
  HAL_Delay(200);
  
  //Initiate EEPROM values
  __IO uint32_t ID_read;
  sEE_ReadBuffer(&hspi2, (uint8_t*)&ID_read, EE_ID_ADDR, 8);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&ID_read, EE_ID_ADDR, 8);//Was error here, have to read 2 times
  
  //Erase EE_ID so eeprom will fill with default values. Kind of hard reset.
  //To hard reset, press the Switch-Off Solder Button while turning the station on
  if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET)
  {
    SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(1, 17);
    SSD1306_Puts("Set default values.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    
    uint8_t delaytimer = 60;
    do
    {
      delaytimer--;
      HAL_Delay(50);
    }while((HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET) && (delaytimer != 1)); //delay approx. 3s while holding the Solder button
    
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_RESET)
    {
      ID_read = 0;
      sEE_WriteBuffer(&hspi2, (uint8_t*)&ID_read, EE_ID_ADDR, 4);
      for(uint8_t i = 0; i <10; i++)//LED blinking fast 
      {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_5);
        HAL_Delay(50);
      }
    }
  }
  
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_UpdateScreen();
  
  if(ID_read != ID_EE)
  {
    //k and b scaling coeffs for solder tips T12 and Hakko907
    for(uint8_t i = 0; i < 5; i++)  
    {
      uint16_t address1 =  i * 8;
      uint16_t address2 = address1 + 4;
      sEE_WriteBuffer(&hspi2, (uint8_t*)&k_solder_T12_default, EE_CAL_COEFF_SOLDER_T12_ADDR + address1, 4);
      sEE_WriteBuffer(&hspi2, (uint8_t*)&b_solder_T12_default, EE_CAL_COEFF_SOLDER_T12_ADDR + address2, 4);
      
      sEE_WriteBuffer(&hspi2, (uint8_t*)&k_solder_H907_default, EE_CAL_COEFF_SOLDER_907_ADDR + address1, 4);
      sEE_WriteBuffer(&hspi2, (uint8_t*)&b_solder_H907_default, EE_CAL_COEFF_SOLDER_907_ADDR + address2, 4);
    }
    //k and b scaling coeffs for fan
    sEE_WriteBuffer(&hspi2, (uint8_t*)&k_fan_default, EE_CAL_COEFF_FAN_ADDR, 4);
    sEE_WriteBuffer(&hspi2, (uint8_t*)&b_fan_default, EE_CAL_COEFF_FAN_ADDR + 4, 4);
    //Timeout for solder
    sEE_WriteBuffer(&hspi2, (uint8_t*)&Timeout_time_default, EE_TIMEOUT_ADDR, 2);
    //preset temperature for fan and solder
    sEE_WriteBuffer(&hspi2, (uint8_t*)&Temp_z_default, EE_TEMP_Z_SOLDER_ADDR, 2);
    sEE_WriteBuffer(&hspi2, (uint8_t*)&Temp_z_default, EE_TEMP_Z_FAN_ADDR, 2);
    
    //Type of solder, T-12 or 907 handle
    sEE_WriteBuffer(&hspi2, (uint8_t*)&soldertype, EE_SOLDER_TYPE_ADDR, 1);
    //Number of current solder tip
    sEE_WriteBuffer(&hspi2, (uint8_t*)&N_tip_default, EE_TIP_N_ADDR, 1);
    //Fan switch-off source (front pannel button or handle gercon)
    sEE_WriteBuffer(&hspi2, (uint8_t*)&fan_switch_off_source, EE_FAN_SWOFF_ADDR, 1);
    //Solder H907 handle preheat PWM limit
    sEE_WriteBuffer(&hspi2, (uint8_t*)&Solder_H907_PWM_limit_default, EE_SOLDER_PWM_LIMIT_ADDR, 4);
    //ID
    ID_read = ID_EE;
    sEE_WriteBuffer(&hspi2, (uint8_t*)&ID_read, EE_ID_ADDR, 4);
    
    SSD1306_GotoXY(1, 17);
    SSD1306_Puts("Initial settings done.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen(); 
    HAL_Delay(2000);
  } 
  
  
  sEE_ReadBuffer(&hspi2, (uint8_t*)&TimeoutTime, EE_TIMEOUT_ADDR, 2);
  
  sEE_ReadBuffer(&hspi2, (uint8_t*)&Solder_temp_z, EE_TEMP_Z_SOLDER_ADDR, 2);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&Fan_temp_z, EE_TEMP_Z_FAN_ADDR, 2);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&N_solder_tip, EE_TIP_N_ADDR, 1);
  
  sEE_ReadBuffer(&hspi2, (uint8_t*)&soldertype, EE_SOLDER_TYPE_ADDR, 1);
  uint16_t address1 =  (N_solder_tip -1) * 8;
  uint16_t address2 = address1 + 4;
  if(soldertype == T_12)
  {
    sEE_ReadBuffer(&hspi2, (uint8_t*)&k_solder, EE_CAL_COEFF_SOLDER_T12_ADDR + address1, 4);
    sEE_ReadBuffer(&hspi2, (uint8_t*)&b_solder, EE_CAL_COEFF_SOLDER_T12_ADDR + address2, 4);
  }
  else if(soldertype == HAKKO_907)
  {
    sEE_ReadBuffer(&hspi2, (uint8_t*)&k_solder, EE_CAL_COEFF_SOLDER_907_ADDR + address1, 4);
    sEE_ReadBuffer(&hspi2, (uint8_t*)&b_solder, EE_CAL_COEFF_SOLDER_907_ADDR + address2, 4);
  }
  
  sEE_ReadBuffer(&hspi2, (uint8_t*)&k_fan, EE_CAL_COEFF_FAN_ADDR, 4);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&b_fan, EE_CAL_COEFF_FAN_ADDR + 4, 4);
  
  sEE_ReadBuffer(&hspi2, (uint8_t*)&Solder_H907_PWM_limit, EE_SOLDER_PWM_LIMIT_ADDR, 4);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&fan_switch_off_source, EE_FAN_SWOFF_ADDR, 1);
  
  Solder_temp_z_former = Solder_temp_z;
  Fan_temp_z_former = Fan_temp_z;
  
  
  __HAL_TIM_SET_COUNTER(&htim2, Solder_temp_z * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
   
  solder_working_layout();
  
    //ADC DMA in circular mode
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc2);
  HAL_ADC_Start(&hadc2);
  HAL_ADCEx_MultiModeStart_DMA(&hadc1, (uint32_t*)&adc_buffer, 8);
  
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  
  SSD1306_UpdateScreen(); 
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    
    //------Write preset temperature variable into eeprom after some time-------
    //------Both at the same time-----------------------------------------------
    if((progstate == SOLDER_E) || (progstate == FAN_E))
    {
      if(((Solder_temp_z - Solder_temp_z_former) != 0) || (Fan_temp_z - Fan_temp_z_former) != 0)
      {
        cntr = 0;
        Fcntr = 1;
      }
      cntr += Fcntr;
      if(cntr >= 40)
      {
        cntr = 0;
        Fcntr = 0;
        sEE_WriteBuffer(&hspi2, (uint8_t*)&Solder_temp_z, EE_TEMP_Z_SOLDER_ADDR, 2);
        sEE_WriteBuffer(&hspi2, (uint8_t*)&Fan_temp_z, EE_TEMP_Z_FAN_ADDR, 2);
      }
      Solder_temp_z_former = Solder_temp_z;
      Fan_temp_z_former = Fan_temp_z;
    }
    
    //-----Switch working layout, fan or solder---------------------------------
    if(BtnCntr_ShortPush)
    {
      if(progstate == SOLDER_E) 
      {
        progstate = FAN_E;
        __HAL_TIM_SET_COUNTER(&htim2, Fan_temp_z * 2 + 1);
        __HAL_TIM_SET_AUTORELOAD(&htim2, 901);
        fan_working_layout();
      }
      else if(progstate == FAN_E) 
      {
        progstate = SOLDER_E;
        __HAL_TIM_SET_COUNTER(&htim2, Solder_temp_z * 2 + 1);
        __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
        solder_working_layout();
      }
      BtnCntr_ShortPush = 0;
    }
    
    //------------Set menu layout----------------------------------------------
    if(BtnCntr_LongPush)
    {
      BtnCntr_LongPush = 0;
      SSD1306_Fill(SSD1306_COLOR_BLACK);
      if((progstate == SOLDER_E) || (progstate == FAN_E))
      {
        prog_state_previous = progstate;
        progstate = MENU_E;
        //Switch off solder and fan
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
        
        SSD1306_GotoXY(20, 0);
        SSD1306_Puts("Settings menu", &mSGothic_12ptFontInfo, SSD1306_COLOR_WHITE);
        
        __IO uint16_t tim2cr1 = TIM2 -> CR1;
        tim2cr1 &= 0xFF9F; //CMS = 00, encoder value rollover
        TIM2 -> CR1 = tim2cr1;
        
        Menu_Navigate(&Menu_1);
        enc_value_previous = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) / 2); //for proper menu show
      }
      else if(progstate == MENU_E)
      {
        solder_working_layout();
      }
      else fan_working_layout();
    }
    
    //----------Display information---------------------------------------------
    int32_t T_filtered;
    T_filtered = 0;
    
    //Filtering temperature for display
    Solder_filter_array[cnt_filter] = Solder_Thermocouple_temp;
    Fan_filter_array[cnt_filter++] = Fan_Thermocouple_temp;
    if(cnt_filter >= T_FILTER_N)
    {
      cnt_filter = 0;
    }
    
    
    if(progstate == SOLDER_E)//-----------------Solder-------------------------
    {
      for(uint8_t i = 0; i < T_FILTER_N; i++)
      {
        T_filtered += Solder_filter_array[i];
      }
      T_filtered /= T_FILTER_N;
      
      SSD1306_DrawFilledRectangle(96, 16, 31, 16, SSD1306_COLOR_BLACK);
      SSD1306_GotoXY(96, 19);
      if(F_solder_enable && F_solder_btn_off)
      {  
        SSD1306_printf(&palatinoLinotype_12ptFontInfo, "%d",  Solder_temp_z);
      }
      else
      {
        SSD1306_printf(&palatinoLinotype_12ptFontInfo, "Off");
      }
      SSD1306_DrawFilledRectangle(0, 32, 127, 31, SSD1306_COLOR_BLACK);
      SSD1306_GotoXY(10, 35);
      SSD1306_printf(&amperzand_24ptFontInfo, "%d",  T_filtered);
      SSD1306_GotoXY(90, 48);
      if(!F_solder_timeout)
      {  
        SSD1306_Puts("Time", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      }
      SSD1306_GotoXY(90, 38);
      if(F_solder_temp_protect)
      {
        SSD1306_Puts("Err!", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      }
      SSD1306_UpdateScreen();
      HAL_Delay(50);
    }
    else if(progstate == FAN_E)//----------------Fan----------------------------
    {
      for(uint8_t i = 0; i < T_FILTER_N; i++)
      {
        T_filtered += Fan_filter_array[i];
      }
      T_filtered /= T_FILTER_N;
      
      SSD1306_DrawFilledRectangle(96, 0, 31, 48, SSD1306_COLOR_BLACK);
      SSD1306_GotoXY(96, 19);
      if(F_fan_enable)
      {
        SSD1306_printf(&palatinoLinotype_12ptFontInfo, "%d",  Fan_temp_z);
      }
      else 
      {
        SSD1306_printf(&palatinoLinotype_12ptFontInfo, "Off");
      }
      SSD1306_DrawFilledRectangle(0, 32, 127, 31, SSD1306_COLOR_BLACK);
      SSD1306_GotoXY(10, 35);
//      if(T_filtered >= 75)
//      {
        SSD1306_printf(&amperzand_24ptFontInfo, "%d",  T_filtered);
//      }
//      else
//      {
//        SSD1306_printf(&amperzand_24ptFontInfo, "<75" );
//      }
      SSD1306_GotoXY(95, 0);
      SSD1306_printf(&palatinoLinotype_12ptFontInfo, "%d%%",  Fan_fan_percent);
      if(F_fan_temp_protect || F_fan_blower_protect)
      {
        SSD1306_GotoXY(90, 38);
         SSD1306_Puts("Err!", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      }
      SSD1306_GotoXY(90, 50);
      if(F_fan_gerkon)
      {
         SSD1306_Puts("On", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      }
      else
      {
        SSD1306_Puts("Off", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      }
      SSD1306_UpdateScreen();
      HAL_Delay(50);
    }
    else if (progstate == MENU_E)
    {
      show_menu();
    }
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_DUALMODE_REGSIMULT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */
  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

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
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 38;
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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 59;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 10199;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OnePulse_Init(&htim1, TIM_OPMODE_SINGLE) != HAL_OK)
  {
    Error_Handler();
  }
  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_TRIGGER;
  sSlaveConfig.InputTrigger = TIM_TS_TI2FP2;
  sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
  sSlaveConfig.TriggerFilter = 15;
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
  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = 10200;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_TIM_DISABLE_OCxPRELOAD(&htim1, TIM_CHANNEL_1);
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim2.Init.Period = 901;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 15;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 15;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
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
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = 10000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_NSS_GPIO_Port, SPI2_NSS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_NSS_Pin */
  GPIO_InitStruct.Pin = SPI2_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SPI2_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6 Solder_switch_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_6|Solder_switch_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
