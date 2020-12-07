//menu.c

#include "menu.h"
#include "sEEPROM.h"
#include "ssd1306.h"
#include "const_var.h"
#include "main.h"


#define ENCODER_ROLLOVER       uint16_t tim2cr1_1 = TIM2 -> CR1; \
                               tim2cr1_1 &= 0xFF9F; \
                               TIM2 -> CR1 = tim2cr1_1;
                               
#define ENCODER_NO_ROLLOVER    uint16_t tim2cr1_2 = TIM2 -> CR1; \
                               tim2cr1_2 |= 0x0060; \
                               TIM2 -> CR1 = tim2cr1_2;


extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern SPI_HandleTypeDef hspi2;

//private function prototypes
void dispMenu(void);
static void Calibrate_solder(void);
static void Calibrate_fan(void);
static void GoBack(void);
uint8_t inputUintCoeff(uint32_t *tmp, uint8_t Kp);
char* menuText(int8_t menuShift);

static void Fan_switch_off_source(void);
static void Solder_H907_PWM_lim(void);
static void MemBackupEEPROM(void);//Backup memory
static void MemRestoreEEPROM(void);//Restore memory

// Menus  Name | Next | Prev | Parent | Child | SelectFunction | EnterFunction | Text
MENU_ITEM(Menu_1, Menu_2, Menu_6, NULL_MENU, Menu_1_1,  NULL, NULL,          "1. Calibrate");
MENU_ITEM(Menu_2, Menu_3, Menu_1, NULL_MENU, Menu_2_1,  NULL, NULL,          "2. Set Solder tip");
MENU_ITEM(Menu_3, Menu_4, Menu_2, NULL_MENU, NULL_MENU, NULL, SetTimeout,    "3. Set solder timeout");
MENU_ITEM(Menu_4, Menu_5, Menu_3, NULL_MENU, Menu_4_1,  NULL, NULL,          "4. Reset");
MENU_ITEM(Menu_5, Menu_6, Menu_4, NULL_MENU, NULL_MENU, NULL, SetSolderType, "5. Set solder type");
MENU_ITEM(Menu_6, Menu_7, Menu_5, NULL_MENU, NULL_MENU, NULL, ExitMenu,      "6. Exit");
MENU_ITEM(Menu_7, Menu_8, Menu_6, NULL_MENU, Menu_7_1,  NULL, NULL,          "7. Options");
MENU_ITEM(Menu_8, Menu_1, Menu_7, NULL_MENU, Menu_8_1,  NULL, NULL,          "8. Mem backup restore");

MENU_ITEM(Menu_1_1, Menu_1_2, Menu_1_3, Menu_1, NULL_MENU, NULL, Calibrate_solder, "Solder");
MENU_ITEM(Menu_1_2, Menu_1_3, Menu_1_1, Menu_1, NULL_MENU, NULL, Calibrate_fan,    "Fan");
MENU_ITEM(Menu_1_3, Menu_1_1, Menu_1_2, Menu_1, NULL_MENU, NULL, GoBack,           "Back");


MENU_ITEM(Menu_2_1, Menu_2_2, Menu_2_6, Menu_2, NULL_MENU, NULL, getN_Solder_tip, "Tip #1");
MENU_ITEM(Menu_2_2, Menu_2_3, Menu_2_1, Menu_2, NULL_MENU, NULL, getN_Solder_tip, "Tip #2");
MENU_ITEM(Menu_2_3, Menu_2_4, Menu_2_2, Menu_2, NULL_MENU, NULL, getN_Solder_tip, "Tip #3");
MENU_ITEM(Menu_2_4, Menu_2_5, Menu_2_3, Menu_2, NULL_MENU, NULL, getN_Solder_tip, "Tip #4");
MENU_ITEM(Menu_2_5, Menu_2_6, Menu_2_4, Menu_2, NULL_MENU, NULL, getN_Solder_tip, "Tip #5");
MENU_ITEM(Menu_2_6, Menu_2_1, Menu_2_5, Menu_2, NULL_MENU, NULL, GoBack, "Back");

MENU_ITEM(Menu_4_1, Menu_4_2, Menu_4_3, NULL_MENU, NULL_MENU, NULL, Reset_solder, "Reset Solder");
MENU_ITEM(Menu_4_2, Menu_4_3, Menu_4_1, NULL_MENU, NULL_MENU, NULL, Reset_fan,    "Reset Fan");
MENU_ITEM(Menu_4_3, Menu_4_1, Menu_4_2, Menu_4,    NULL_MENU, NULL, GoBack,       "Back");

MENU_ITEM(Menu_7_1, Menu_7_2, Menu_7_3, NULL_MENU, NULL_MENU, NULL, Fan_switch_off_source, "Fan switch off source");
MENU_ITEM(Menu_7_2, Menu_7_3, Menu_7_1, NULL_MENU, NULL_MENU, NULL, Solder_H907_PWM_lim,   "Solder PWM limit");
MENU_ITEM(Menu_7_3, Menu_7_1, Menu_7_2, Menu_7,    NULL_MENU, NULL, GoBack,                 "Back");

MENU_ITEM(Menu_8_1, Menu_8_2, Menu_8_3, NULL_MENU, NULL_MENU, NULL, MemBackupEEPROM, "EEPROM backup");
MENU_ITEM(Menu_8_2, Menu_8_3, Menu_8_1, NULL_MENU, NULL_MENU, NULL, MemRestoreEEPROM, "EEPROM restore");
MENU_ITEM(Menu_8_3, Menu_8_1, Menu_8_2, Menu_8,    NULL_MENU, NULL, GoBack, "Back");

static void MemBackupEEPROM(void)//Backup memory
{
  uint8_t backup_arr[250];
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  BtnCntr_Menu = 0;
  sEE_ReadBuffer(&hspi2, backup_arr,EE_ID_ADDR, 250);
  sEE_WriteBuffer(&hspi2, backup_arr,EE_ID_ADDR + 0x0100, 250);
  SSD1306_Puts("EEPROM backup sucsess.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}

static void MemRestoreEEPROM(void)//Restore memory
{
  uint8_t restore_arr[250];
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  BtnCntr_Menu = 0;
  sEE_ReadBuffer(&hspi2, restore_arr,EE_ID_ADDR + 0x0100, 250);
  sEE_WriteBuffer(&hspi2, restore_arr,EE_ID_ADDR, 250);
  SSD1306_Puts("EEPROM restore sucsess.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}



static void Fan_switch_off_source(void)
{
  uint16_t tmparr;
  uint8_t counter_temp;
  FanSwitchOffType fan_switchoff_tmp;
  
  
  BtnCntr_Menu = 0;
  tmparr = __HAL_TIM_GET_AUTORELOAD(&htim2);
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts("Select fan switch off source:", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  
  ENCODER_ROLLOVER
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 3);
  while(!BtnCntr_Menu)
  {
    BtnCntr_Menu = 0;
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2)) / 2;
    SSD1306_DrawFilledRectangle(5, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(15, 35);
    if(counter_temp == 0)
    {
      SSD1306_Puts("Gercon", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      fan_switchoff_tmp = GERCON;
    }
    else
    {
      SSD1306_Puts("Button ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      fan_switchoff_tmp = BUTTON;
    }
    SSD1306_UpdateScreen();
    
    if(BtnCntr_LongPush)//exit setting solder type without eeprom write
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
      ENCODER_ROLLOVER
      return;
    }
    HAL_Delay(50);
  }
  BtnCntr_Menu = 0;
  fan_switch_off_source = fan_switchoff_tmp;
  sEE_WriteBuffer(&hspi2, (uint8_t*)&fan_switch_off_source, EE_FAN_SWOFF_ADDR, 1);
  
  //--end of a selection-------------------------------------------------------
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts("Fan switch off source is ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(1, 27);
  SSD1306_Puts("set to ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(25, 27);
  if(fan_switch_off_source == GERCON) SSD1306_Puts("Handle Gercon", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  else SSD1306_Puts("Front Pannel Button", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
  HAL_Delay(3000);
}


static void Solder_H907_PWM_lim(void)
{
  uint16_t tmparr, counter_temp;
  uint8_t H907_PWM_limit_percent;
  
  if(soldertype != HAKKO_907) 
  {
    SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(1, 27);
    SSD1306_Puts("Nothing to enter here.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    HAL_Delay(3000);
    return;
  }
  
  H907_PWM_limit_percent = (uint8_t)(Solder_H907_PWM_limit * 100); 
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts("Enter PWM limit in percent:", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  
  tmparr = __HAL_TIM_GET_AUTORELOAD(&htim2);
  ENCODER_NO_ROLLOVER
    
  __HAL_TIM_SET_COUNTER(&htim2, H907_PWM_limit_percent * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 201);
    BtnCntr_Menu = 0;
  while(!BtnCntr_Menu)
  {
    BtnCntr_Menu = 0;
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2)) / 2;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d", counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//exit setting timeout without eeprom write
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
      ENCODER_ROLLOVER
      return;
    }
    HAL_Delay(50);
  }
  Solder_H907_PWM_limit = (float)counter_temp / 100;
  sEE_WriteBuffer(&hspi2, (uint8_t*)&Solder_H907_PWM_limit, EE_SOLDER_PWM_LIMIT_ADDR, 4);
  
  BtnCntr_Menu = 0;
  ENCODER_ROLLOVER
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "PWM limit is set to", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(15, 35);
  SSD1306_printf(&segoeUI_8ptFontInfo, "%d s", Solder_H907_PWM_limit * 100);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}


static void SetSolderType(void)
{
  uint16_t tmparr;
  uint8_t counter_temp;
  SolderType soldertype_tmp;
  uint16_t base_address;
  
  BtnCntr_Menu = 0;
  tmparr = __HAL_TIM_GET_AUTORELOAD(&htim2);
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts("Enter handle type:", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  
  ENCODER_ROLLOVER
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 3);
  while(!BtnCntr_Menu)
  {
    BtnCntr_Menu = 0;
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2)) / 2;
    SSD1306_DrawFilledRectangle(5, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(15, 35);
    if(counter_temp == 0)
    {
      SSD1306_Puts("T-12 ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      soldertype_tmp = T_12;
    }
    else
    {
      SSD1306_Puts("Hakko 907 ", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
      soldertype_tmp = HAKKO_907;
    }
    SSD1306_UpdateScreen();
    
    if(BtnCntr_LongPush)//exit setting solder type without eeprom write
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
      ENCODER_ROLLOVER
      return;
    }
    HAL_Delay(50);
  }
  BtnCntr_Menu = 0;
  soldertype = soldertype_tmp;
  sEE_WriteBuffer(&hspi2, (uint8_t*)&soldertype, EE_SOLDER_TYPE_ADDR, 1);
  
  //reload solder coefficients
  uint16_t address1 =  (N_solder_tip -1) * 8;
  uint16_t address2 = address1 + 4;
  if(soldertype == T_12)
  {
     base_address = EE_CAL_COEFF_SOLDER_T12_ADDR;
  }
  else 
  {
     base_address = EE_CAL_COEFF_SOLDER_907_ADDR;
  }
  sEE_ReadBuffer(&hspi2, (uint8_t*)&k_solder, base_address + address1, 4);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&b_solder, base_address + address2, 4);
  
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Solder type is set to", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(15, 35);
  if(soldertype == T_12)
  {
  SSD1306_Puts( "T-12", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  }
  else 
  {
    SSD1306_Puts( "Hakko 907", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  }
  SSD1306_UpdateScreen();
  __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
  HAL_Delay(3000);
}


static void SetTimeout(void)
{
  uint16_t tmparr, counter_temp;
  if(soldertype == HAKKO_907) 
  {
    SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(1, 27);
    SSD1306_Puts("Nothing to enter here.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
    HAL_Delay(3000);
    return;
  }
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts("Enter timeout in sec.", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  
  __IO uint16_t tim2cr1 = TIM2 -> CR1;
  tim2cr1 |= 0x0060; //CMS = 11, encoder value max = arr, no rollover 
  TIM2 -> CR1 = tim2cr1;
  tmparr = __HAL_TIM_GET_AUTORELOAD(&htim2);
  
  __HAL_TIM_SET_COUNTER(&htim2, TimeoutTime * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 65535);
    BtnCntr_Menu = 0;
  while(!BtnCntr_Menu)
  {
    BtnCntr_Menu = 0;
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2)) / 2;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d", counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//exit setting timeout without eeprom write
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_AUTORELOAD(&htim2, tmparr);
      tim2cr1 = TIM2 -> CR1;
      tim2cr1 &= 0xFF9F; //CMS = 00, encoder value rollover
      TIM2 -> CR1 = tim2cr1;
      return;
    }
    HAL_Delay(50);
  }
  TimeoutTime = counter_temp;
  sEE_WriteBuffer(&hspi2, (uint8_t*)&TimeoutTime, EE_TIMEOUT_ADDR, 2);
  
  BtnCntr_Menu = 0;
  tim2cr1 = TIM2 -> CR1;
  tim2cr1 &= 0xFF9F; //CMS = 00, encoder value rollover
  TIM2 -> CR1 = tim2cr1;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Timeout is set to", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(15, 35);
  SSD1306_printf(&segoeUI_8ptFontInfo, "%d s", TimeoutTime);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}


static void Reset_solder(void)
{
  uint16_t base_address;
  float k_default, b_default;
  
  uint16_t address1 =  (N_solder_tip -1) * 8;
  uint16_t address2 = address1 + 4;
  
  if(soldertype == T_12) 
  {
    base_address = EE_CAL_COEFF_SOLDER_T12_ADDR;
    k_default = k_solder_T12_default;
    b_default = b_solder_T12_default;
  }
  else  
  {
    base_address = EE_CAL_COEFF_SOLDER_907_ADDR;
    k_default = k_solder_H907_default;
    b_default = b_solder_H907_default;
  }
  
  sEE_WriteBuffer(&hspi2, (uint8_t*)&k_default, base_address + address1, 4);
  sEE_WriteBuffer(&hspi2, (uint8_t*)&b_default, base_address + address2, 4);
  k_solder = k_default;
  b_solder = b_default;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(5, 25);
  if(soldertype == T_12) 
  {
    SSD1306_Puts( "T-12 handle", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  }
  else SSD1306_Puts( "Hakko907 handle", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY(5, 45);
  SSD1306_printf(&segoeUI_8ptFontInfo, "Solder tip # %d reset.",  N_solder_tip);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
}
 
static void Reset_fan(void)
{  
  sEE_WriteBuffer(&hspi2, (uint8_t*)&k_fan_default, EE_CAL_COEFF_FAN_ADDR, 4);
  sEE_WriteBuffer(&hspi2, (uint8_t*)&b_fan_default, EE_CAL_COEFF_FAN_ADDR + 4, 4);
  k_fan = k_fan_default;
  b_fan = b_fan_default;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(5, 25);
  SSD1306_Puts("Fan coeffs are reset", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
}

static void getN_Solder_tip(void)
{
  Menu_Item_t* tmpMenu = Menu_GetCurrentMenu();

  if(tmpMenu == &Menu_2_1) N_solder_tip = 1;
  else if(tmpMenu == &Menu_2_2) N_solder_tip = 2;
  else if(tmpMenu == &Menu_2_3) N_solder_tip = 3;
  else if(tmpMenu == &Menu_2_4) N_solder_tip = 4;
  else if(tmpMenu == &Menu_2_5) N_solder_tip = 5;

  uint16_t address1 =  (N_solder_tip -1) * 8;
  uint16_t address2 = address1 + 4;
  sEE_ReadBuffer(&hspi2, (uint8_t*)&k_solder, EE_CAL_COEFF_SOLDER_T12_ADDR + address1, 4);
  sEE_ReadBuffer(&hspi2, (uint8_t*)&b_solder, EE_CAL_COEFF_SOLDER_T12_ADDR + address2, 4);
  
  sEE_WriteBuffer(&hspi2, (uint8_t*)&N_solder_tip, EE_TIP_N_ADDR, 2);
  
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(5, 25);
  SSD1306_printf(&segoeUI_8ptFontInfo, "Solder tip is set to # %d",  N_solder_tip);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}


static void ExitMenu(void)
{
   ENCODER_NO_ROLLOVER
  if (prog_state_previous == SOLDER_E) 
  {
    __HAL_TIM_SET_COUNTER(&htim2, Solder_temp_z * 2 + 1);
    __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
    solder_working_layout();
  }
  else 
  {
    __HAL_TIM_SET_COUNTER(&htim2, Fan_temp_z * 2 + 1);
    __HAL_TIM_SET_AUTORELOAD(&htim2, 901);
    fan_working_layout();
  }
}


static void Calibrate_solder(void)
{
  static float temp_calibration_solder1, adc_calibration_solder1, temp_calibration_solder2, adc_calibration_solder2;
  uint16_t counter_pwm, counter_temp, base_address;
  
  ENCODER_NO_ROLLOVER
  
  //Start Point PWM
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter Start PWM", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, 201);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 201);

  while(!BtnCntr_Menu)
  {
    counter_pwm = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, counter_pwm * 100);
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_pwm);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
      return;
    }
    HAL_Delay(50);
  }
  
  //Start temperature
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter temperature1", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, 201);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
  while(!BtnCntr_Menu)
  {
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    temp_calibration_solder1 = counter_temp;
    adc_calibration_solder1 = Solder_Thermocouple_adc * !(bool)soldertype + Solder_H907_adc * (bool)soldertype;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
      return;
    }
    HAL_Delay(50);
  }
  
  //End Point PWM
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter End PWM", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, counter_pwm * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 201);
  while(!BtnCntr_Menu)
  {
    uint16_t counter_pwm = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, counter_pwm * 100);
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_pwm);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
      return;
    }
    HAL_Delay(50);
  }
  
  //End temperature
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter temperature2", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, counter_temp * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
  while(!BtnCntr_Menu)
  {
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    temp_calibration_solder2 = counter_temp;
    adc_calibration_solder2 = Solder_Thermocouple_adc * !(bool)soldertype + Solder_H907_adc * (bool)soldertype;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
      return;
    }
    HAL_Delay(50);
  }
  
  k_solder = (temp_calibration_solder2 - temp_calibration_solder1) / (adc_calibration_solder2 - adc_calibration_solder1);
  b_solder = temp_calibration_solder1 - k_solder * adc_calibration_solder1;
  
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MAX_CCR_LOAD_SOLDER);
  
  
  if(soldertype == T_12) 
  {
    base_address = EE_CAL_COEFF_SOLDER_T12_ADDR;
  }
  else base_address = EE_CAL_COEFF_SOLDER_907_ADDR;
 
  uint16_t address1 =  (N_solder_tip -1) * 8;
  uint16_t address2 = address1 + 4;
  sEE_WriteBuffer(&hspi2, (uint8_t*)&k_solder, base_address + address1, 4);
  sEE_WriteBuffer(&hspi2, (uint8_t*)&b_solder, base_address + address2, 4);
  
  BtnCntr_Menu = 0;
  ENCODER_ROLLOVER
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 27);
  SSD1306_Puts( "Calibration finished", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
  
}


static void Calibrate_fan(void)
{
  static float temp_calibration_fan1, adc_calibration_fan1, temp_calibration_fan2, adc_calibration_fan2;
  uint16_t counter_pwm, counter_temp;
  
  ENCODER_NO_ROLLOVER
    
    //Start Point PWM
    BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter Start PWM", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, 201);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 201);
  
  while(!BtnCntr_Menu)
  {
    counter_pwm = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, counter_pwm * (MAX_CCR_LOAD_FAN / 100));
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_pwm);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
      return;
    }
    HAL_Delay(50);
  }
  
  //Start temperature
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter temperature1", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, 201);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
  while(!BtnCntr_Menu)
  {
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    temp_calibration_fan1 = counter_temp;
    adc_calibration_fan1 = Fan_Thermocouple_adc;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
      return;
    }
    HAL_Delay(50);
  }
  
  //End Point PWM
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter End PWM", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, counter_pwm * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 201);
  while(!BtnCntr_Menu)
  {
    uint16_t counter_pwm = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, counter_pwm * (MAX_CCR_LOAD_FAN / 100));
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_pwm);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
      return;
    }
    HAL_Delay(50);
  }
  
  //End temperature
  BtnCntr_Menu = 0;
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 17);
  SSD1306_Puts( "Enter temperature2", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  __HAL_TIM_SET_COUNTER(&htim2, counter_temp * 2 + 1);
  __HAL_TIM_SET_AUTORELOAD(&htim2, 801);
  while(!BtnCntr_Menu)
  {
    counter_temp = (__HAL_TIM_GET_COUNTER(&htim2) / 2);
    temp_calibration_fan2 = counter_temp;
    adc_calibration_fan2 = Fan_Thermocouple_adc;
    SSD1306_DrawFilledRectangle(30, 27, 127, 27, SSD1306_COLOR_BLACK);
    SSD1306_GotoXY(30, 27);     
    SSD1306_printf(&amperzand_24ptFontInfo, "%d",  counter_temp);
    SSD1306_UpdateScreen();
    if(BtnCntr_LongPush)//reset calibration
    {
      BtnCntr_LongPush = 0;
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
      return;
    }
    HAL_Delay(50);
  }
  
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MAX_CCR_LOAD_FAN);
  
  k_fan = (temp_calibration_fan2 - temp_calibration_fan1) / (adc_calibration_fan2 - adc_calibration_fan1);
  b_fan = temp_calibration_fan1 - k_fan * adc_calibration_fan1;
  
  sEE_WriteBuffer(&hspi2, (uint8_t*)&k_fan, EE_CAL_COEFF_FAN_ADDR, 4);
  sEE_WriteBuffer(&hspi2, (uint8_t*)&b_fan, EE_CAL_COEFF_FAN_ADDR + 4, 4);
  
  BtnCntr_Menu = 0;
  ENCODER_ROLLOVER
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  SSD1306_GotoXY(1, 27);
  SSD1306_Puts( "Calibration finished", &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay(3000);
}


static void GoBack(void)
{
  Menu_Navigate(MENU_PARENT);
}


//Main Menu function
void show_menu(void)
{
  Menu_Item_t* tmpMenu;
  uint16_t enc_value;
  int16_t delta;
  
  enc_value = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) / 2);
  delta = enc_value - enc_value_previous;
 
   
  if(delta > 0) Menu_Navigate(MENU_PREVIOUS);
  if(delta < 0) Menu_Navigate(MENU_NEXT);
  if(BtnCntr_Menu)
  {
     tmpMenu = Menu_GetCurrentMenu();
     void (*EnterCallback)(void) = tmpMenu -> EnterCallback;
     if (!((tmpMenu -> Child == &NULL_MENU) || (tmpMenu -> Child == NULL)))
     {
       Menu_Navigate(MENU_CHILD);
     }
     else if (EnterCallback)
     {
       Menu_EnterCurrentItem();
     }
  }
  
  if(tmpMenu != &Menu_6)
  dispMenu(); 
  BtnCntr_Menu = 0;
  SSD1306_UpdateScreen();
  enc_value_previous = enc_value;
  HAL_Delay(50);
  
}


char* menuText(int8_t menuShift)
{
  int8_t i;	
  Menu_Item_t* tmpMenu;
  
  tmpMenu = Menu_GetCurrentMenu();
  if ((tmpMenu == &NULL_MENU) || (tmpMenu == NULL))
    return (char *)strNULL;
  
  
  i = menuShift;
  if (i > 0) 
  {
    while( i != 0 ) 
    {
      if (!((tmpMenu == &NULL_MENU) || (tmpMenu == NULL)))
      {
        tmpMenu = tmpMenu -> Next;
      }
      i--;
    }
  }
  else 
  {
    while( i != 0 ) 
    {
      if (!((tmpMenu == &NULL_MENU) || (tmpMenu == NULL)))
      {
        tmpMenu = tmpMenu -> Previous;
      }
      i++;
    }
  }
  
  if ((tmpMenu == &NULL_MENU) || (tmpMenu == NULL))
    return (char *)strNULL;
  else 
    return ((char *)tmpMenu -> Text);
  
}
 
void dispMenu(void) 
{
  SSD1306_DrawFilledRectangle(0, 17, 127, 46, SSD1306_COLOR_BLACK);
  
  SSD1306_GotoXY(7, 17);
  SSD1306_Puts( menuText(-1), &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  
  SSD1306_GotoXY(7, 34);
  SSD1306_Puts( menuText(0), &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  
  SSD1306_GotoXY(7, 51);
  SSD1306_Puts( menuText(1), &segoeUI_8ptFontInfo, SSD1306_COLOR_WHITE);
  
  SSD1306_DrawRectangle(1, 32, 127, 14, SSD1306_COLOR_WHITE);
  
}