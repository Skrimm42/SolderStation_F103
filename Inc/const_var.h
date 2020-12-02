//const_var.h

#ifndef __CONST_VAR_H__
#define __CONST_VAR_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include "button_drv.h"

#define T_FILTER_N 10


#define MAX_CCR_LOAD_SOLDER 10000
#define MAX_CCR_LOAD_FAN    10200

#define FAN_FLUSH_TEMPERATURE 75

#define ID_EE                           0xDEADBEEF
#define EE_ID_ADDR                      0x0020
#define EE_TEMP_Z_SOLDER_ADDR           0x0040
#define EE_TEMP_Z_FAN_ADDR              0x0042
#define EE_TIP_N_ADDR                   0x0060
#define EE_CAL_COEFF_SOLDER_T12_ADDR    0x0080
#define EE_CAL_COEFF_FAN_ADDR           0x00A8
#define EE_TIMEOUT_ADDR                 0x00B0
#define EE_KP_SOLDER_ADDR               0x00C0
#define EE_KP_FAN_ADDR                  0x00C4
#define EE_KI_SOLDER_ADDR               0x00C8
#define EE_KI_FAN_ADDR                  0x00CC
#define EE_BTNOFF_SOLDER_ADDR           0x00D0
#define EE_SOLDER_TYPE_ADDR             0x00D1
#define EE_CAL_COEFF_SOLDER_907_ADDR    0x00E0
#define EE_FAN_SWOFF_ADDR               0x00F0
#define EE_SOLDER_PWM_LIMIT_ADDR        0x00F1

#define SOLDER_MIN_TEMP_Z 99
#define FAN_MIN_TEMP_Z    99

#define DEBOUNCE          5

typedef enum  
{
  SOLDER_E,
  FAN_E,
  MENU_E
}ProgState;

typedef enum  
{
  T_12 = 0,
  HAKKO_907 = 1,
}SolderType;

typedef enum  
{
  GERCON = 0,
  BUTTON = 1,
}FanSwitchOffType;

extern ButtonStateTypeDef EncBtn, Solder_off_btn, Fan_off_btn;

extern ProgState progstate, prog_state_previous;
extern SolderType soldertype;
extern FanSwitchOffType fan_switch_off_source;

extern const float k_solder_T12_default, b_solder_T12_default, k_fan_default,
                   k_solder_H907_default, b_solder_H907_default, b_fan_default;
extern const uint8_t N_tip_default;
extern const uint16_t Timeout_time_default, Temp_z_default;
extern const uint32_t ID;
extern const float Solder_H907_PWM_limit_default;

extern uint16_t adc_buffer[24];

extern uint16_t TimeoutTime;
extern uint32_t Timeout_cntr;
extern uint16_t enc_value_previous;
extern uint16_t Solder_temp_z, Solder_temp_z_former, Fan_temp_z, Fan_temp_z_former;
extern uint8_t N_solder_tip;
extern float k_solder, b_solder, k_fan, b_fan;

extern uint8_t Fan_fan_percent;
extern uint16_t Solder_Thermocouple_adc, Fan_Thermocouple_adc, Fan_fan_adc, Solder_H907_adc;
extern int16_t Solder_Thermocouple_temp, Fan_Thermocouple_temp;
extern float U_solder_temp, U_solder_temp_z, Uy_solder_p, Uy_solder_i, Uy_solder;
extern float U_fan_temp, U_fan_temp_z, Uy_fan_p, Uy_fan_i, Uy_fan;
extern const float K1_h907, K1_t12, K2, T1, T2, tp; 


extern bool F_solder, F_solder_switch, F_solder_timeout, F_solder_enable, F_fan, 
     F_fan_enable, F_fan_gerkon, F_fan_temp_protect, F_solder_temp_protect, 
     F_fan_blower_protect, F_encoder_change_value, F_solder_btn_off;
extern bool BtnCntr_ShortPush, BtnCntr_LongPush, BtnCntr_Menu, F_fan_btn_off;

extern int16_t Solder_filter_array[T_FILTER_N];
extern int16_t Fan_filter_array[T_FILTER_N];

extern const char strNULL[];
extern uint8_t Kp_Solder;
extern bool K_blower_flush, K_blower_off;

extern float Solder_H907_PWM_limit;

#endif//__CONST_VAR_H__
