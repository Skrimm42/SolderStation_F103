//button_drv.h


#ifndef __BUTTON_DRV_H__
#define __BUTTON_DRV_H__

#include "stm32f1xx_hal.h"

#define LONGPRESSTIME 200
#define DEBOUNCE 5


typedef enum 
{
  BTNDRV_OK       = 0x00U,
  BTNDRV_ERROR    = 0x01U,
} ButtonStatusTypeDef;

typedef struct
{
  uint8_t ShortPush:1;
  uint8_t LongPush:1;
  uint8_t LongPress:1;
  //internal variables
  uint8_t current_button_state:1;
  uint8_t previous_button_state, prev_longpush_state;
  uint8_t jitter_cnt;
  uint16_t longpress_cnt;
  GPIO_TypeDef* GPIOX;
  uint16_t GPIO_PinX;
} ButtonStateTypeDef;

ButtonStatusTypeDef Button_Init (ButtonStateTypeDef* Button , GPIO_TypeDef* GPIOx, uint16_t GPIO_Pinx);
ButtonStatusTypeDef getButtonState(ButtonStateTypeDef* Button);


#endif //__BUTTON_DRV_H__