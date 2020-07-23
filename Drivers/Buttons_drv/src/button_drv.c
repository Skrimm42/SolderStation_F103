//button_drv.c

#include "button_drv.h"



/*
@brief  Длинное и короткое нажатие кнопки

  Button - > ShortPush - короткое нажатие, при отпускании кнопки, в течении одного цикла программы
  Button - > LongPush - длинное нажатие, после LONGPRESSTIME циклов программы, в течении одного цикла программы
  Button - > LongPress - нажатое состояние кнопки.
  При детектировании длинного нажатия короткое нажатие не происходит (при отпускании кнопки).
  Фильтрация входного сигнала - в течении DEBOUNCE циклов программы.
*/
ButtonStatusTypeDef getButtonState(ButtonStateTypeDef* Button)
{  
  if((Button -> GPIOX == NULL) || (Button -> GPIO_PinX == NULL)) return BTNDRV_ERROR;
  
  Button -> current_button_state =  !HAL_GPIO_ReadPin(Button -> GPIOX, Button -> GPIO_PinX);
 
  if(Button -> jitter_cnt >= DEBOUNCE)
  {
    Button -> LongPress = 1;
    Button -> jitter_cnt = DEBOUNCE; 
  }
  else 
  {
    Button -> LongPress = 0;
  }
    
  Button -> ShortPush = (Button -> LongPress ) & ((Button -> LongPress ^ Button -> previous_button_state) & (!Button -> current_button_state));
  
  Button -> longpress_cnt = (Button -> longpress_cnt + 1) * Button -> LongPress;
  if(Button -> longpress_cnt >= LONGPRESSTIME)
  {
    Button -> LongPush = !Button -> prev_longpush_state;
    Button -> prev_longpush_state = 1;
    Button -> ShortPush = 0;
    Button -> longpress_cnt = LONGPRESSTIME; // to avoid overflow
  }
  else
  {
    Button -> LongPush = 0;
    Button -> prev_longpush_state = 0;
  }
  
  Button -> jitter_cnt = (Button -> jitter_cnt + 1) * Button -> previous_button_state;
  Button -> previous_button_state = Button -> current_button_state;

  return BTNDRV_OK;
}


/*
Привязка инстанса кнопки к GPIO пину
*/
ButtonStatusTypeDef Button_Init (ButtonStateTypeDef* Button , GPIO_TypeDef* GPIOx, uint16_t GPIO_Pinx)
{
   Button -> GPIOX = GPIOx;
   Button -> GPIO_PinX = GPIO_Pinx;
   return BTNDRV_OK;
}