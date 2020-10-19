//const_var.c

#include "const_var.h"

ButtonStateTypeDef EncBtn, Solder_off_btn;

ProgState progstate = SOLDER_E, prog_state_previous = SOLDER_E;
SolderType soldertype = T_12;

const float k_solder_T12_default = 0.131541, b_solder_T12_default = -42.93635,
            k_solder_H907_default = 0.11931, b_solder_H907_default = -9.9862,
            k_fan_default = 0.100217864923747, b_fan_default = 48.8910675381264;
const uint8_t N_tip_default = 1;
const uint16_t Timeout_time_default = 180, Temp_z_default = 220;
const uint32_t ID = ID_EE;

uint16_t adc_buffer[24];

uint16_t TimeoutTime;
uint32_t Timeout_cntr;

uint16_t Solder_temp_z, Solder_temp_z_former, Fan_temp_z, Fan_temp_z_former;
uint8_t N_solder_tip;
float k_solder, b_solder, k_fan, b_fan;
uint16_t enc_value_previous;
uint8_t Fan_fan_percent;
uint16_t Solder_Thermocouple_adc, Fan_Thermocouple_adc, Fan_fan_adc, Solder_H907_adc;
int16_t Solder_Thermocouple_temp, Fan_Thermocouple_temp;
float U_solder_temp, U_solder_temp_z, Uy_solder_p, Uy_solder_i, Uy_solder;
float U_fan_temp, U_fan_temp_z, Uy_fan_p, Uy_fan_i, Uy_fan;
const float K1 = 5, K2 = 15, T1 = 0.3, T2 = 0.3, tp = 0.01; 


bool F_solder, F_solder_switch, F_solder_timeout = 1, F_solder_enable, F_fan, 
     F_fan_enable, F_fan_gerkon, F_fan_temp_protect, F_solder_temp_protect, 
     F_fan_blower_protect, F_encoder_change_value, F_solder_btn_off;
bool BtnCntr_ShortPush, BtnCntr_LongPush, BtnCntr_Menu;

int16_t Solder_filter_array[T_FILTER_N + 1];
int16_t Fan_filter_array[T_FILTER_N + 1];


const char strNULL[] = "";

uint8_t Kp_Solder = 20;

bool K_blower_flush = 1, K_blower_off = 1;
    
    