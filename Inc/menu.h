//menu.h

#ifndef __MENU_H__
#define __MENU_H__

#include "stm32f1xx_hal.h"
#include "MicroMenu.h"

static void setSolderKp(void);
uint8_t inputUintCoeff(uint32_t *tmp, uint8_t Kp);
static void SetTimeout(void);
static void Reset_solder(void);
static void Reset_fan(void);
static void getN_Solder_tip(void);
static void ExitMenu(void);
void fan_working_layout(void);

void show_menu(void);

extern Menu_Item_t const Menu_1, Menu_2, Menu_3, Menu_4, Menu_5, Menu_6;
extern Menu_Item_t const Menu_1_1, Menu_1_2, Menu_1_3;
extern Menu_Item_t const Menu_2_1, Menu_2_2, Menu_2_3, Menu_2_4, Menu_2_5, Menu_2_6;
extern Menu_Item_t const Menu_4_1, Menu_4_2, Menu_4_3;
extern Menu_Item_t const Menu_5_1, Menu_5_2, Menu_5_3;
extern Menu_Item_t const Menu_5_1_1, Menu_5_1_2, Menu_5_1_3;
extern Menu_Item_t const Menu_5_2_1, Menu_5_2_2, Menu_5_2_3;


#endif//__MENU_H__