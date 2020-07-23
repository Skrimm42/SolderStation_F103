//sEEPROM.h

#ifndef __SEEPROM_H__
#define __SEEPROM_H__

#include "stm32f1xx_hal.h"

/* M25P SPI EE supported commands */  
#define sEE_CMD_WRITE          0x02  /* Write to Memory instruction */
#define sEE_CMD_WRSR           0x01  /* Write Status Register instruction */
#define sEE_CMD_WREN           0x06  /* Write enable instruction */
#define sEE_CMD_READ           0x03  /* Read from Memory instruction */
#define sEE_CMD_RDSR           0x05  /* Read Status Register instruction  */
#define sEE_CMD_RDID           0x83  /* Read identification */
#define sEE_CMD_LID            0x82  /* Locks the Identification page in read-only mode*/

#define sEE_WIP_FLAG           0x01  /* Write In Progress (WIP) flag */

#define sEE_DUMMY_BYTE         0xA5
#define sEE_SPI_PAGESIZE       0x20


/* Select sEE: Chip Select pin low */
#define sEE_CS_LOW()        HAL_GPIO_WritePin(SPI2_NSS_GPIO_Port, SPI2_NSS_Pin, GPIO_PIN_RESET);
/* Deselect sEE: Chip Select pin high */
#define sEE_CS_HIGH()       HAL_GPIO_WritePin(SPI2_NSS_GPIO_Port, SPI2_NSS_Pin, GPIO_PIN_SET);

void sEE_ReadBuffer(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead);
void sEE_WritePage(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void sEE_WriteBuffer(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

#endif//__SEEPROM_H__
