//seeprom.c
//in CUBE GPIO name of the NSS pin should be "NSS_Pin"
#include "sEEPROM.h"
#include "main.h" //for spi nss pin definition


void sEE_WriteEnable(SPI_HandleTypeDef *hspi);
void sEE_WaitForWriteEnd(SPI_HandleTypeDef *hspi);


/**
  * @brief  Reads a block of data from the EEPROM.
  * @param  hspi: pointer to the spi instance
  * @param  pBuffer: pointer to the buffer that receives the data read from the EEPROM.
  * @param  ReadAddr: EEPROM's internal address to read from.
  * @param  NumByteToRead: number of bytes to read from the EEPROM.
  * @retval None
  */
void sEE_ReadBuffer(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead)
{  
  uint8_t tmparr[3];
  
  tmparr[0] = sEE_CMD_READ;
  tmparr[1] = (ReadAddr& 0xFF00) >> 8;
  tmparr[2] = ReadAddr & 0xFF;
  
  sEE_CS_LOW();
  
  /*!< Send "Read from Memory " instruction */
  if(HAL_SPI_Transmit(hspi, tmparr, 3, 1000) != HAL_OK)Error_Handler();

  if(HAL_SPI_Receive(hspi, pBuffer, NumByteToRead, 1000) != HAL_OK)Error_Handler();
  
  /*!< Deselect the EE: Chip Select high */
  sEE_CS_HIGH();
}

/**
  * @brief  Writes block of data to the EE. In this function, the number of
  *         WRITE cycles are reduced, using Page WRITE sequence.
  * @param  pBuffer: pointer to the buffer  containing the data to be written
  *         to the EE.
  * @param  WriteAddr: EE's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the EE.
  * @retval None
  */
void sEE_WriteBuffer(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
  uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
 
  Addr = WriteAddr % sEE_SPI_PAGESIZE;
  count = sEE_SPI_PAGESIZE - Addr;
  NumOfPage =  NumByteToWrite / sEE_SPI_PAGESIZE;
  NumOfSingle = NumByteToWrite % sEE_SPI_PAGESIZE;

  if (Addr == 0) /*!< WriteAddr is sEE_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /*!< NumByteToWrite < sEE_PAGESIZE */
    {
      sEE_WritePage(hspi, pBuffer, WriteAddr, NumByteToWrite);
    }
    else /*!< NumByteToWrite > sEE_PAGESIZE */
    {
      
      while (NumOfPage--)
      {
        sEE_WritePage(hspi, pBuffer, WriteAddr, sEE_SPI_PAGESIZE);
        WriteAddr +=  sEE_SPI_PAGESIZE;
        pBuffer += sEE_SPI_PAGESIZE;
      }

      sEE_WritePage(hspi, pBuffer, WriteAddr, NumOfSingle);
    }
  }
  else /*!< WriteAddr is not sEE_PAGESIZE aligned  */
  {
    if (NumOfPage == 0) /*!< NumByteToWrite < sEE_PAGESIZE */
    {
      if (NumOfSingle > count) /*!< (NumByteToWrite + WriteAddr) > sEE_PAGESIZE */
      {
        temp = NumOfSingle - count;

        sEE_WritePage(hspi, pBuffer, WriteAddr, count);
        WriteAddr +=  count;
        pBuffer += count;

        sEE_WritePage(hspi, pBuffer, WriteAddr, temp);
      }
      else
      {
        sEE_WritePage(hspi, pBuffer, WriteAddr, NumByteToWrite);
      }
    }
    else /*!< NumByteToWrite > sEE_PAGESIZE */
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / sEE_SPI_PAGESIZE;
      NumOfSingle = NumByteToWrite % sEE_SPI_PAGESIZE;

      sEE_WritePage(hspi, pBuffer, WriteAddr, count);
      WriteAddr +=  count;
      pBuffer += count;

      while (NumOfPage--)
      {
        sEE_WritePage(hspi, pBuffer, WriteAddr, sEE_SPI_PAGESIZE);
        WriteAddr +=  sEE_SPI_PAGESIZE;
        pBuffer += sEE_SPI_PAGESIZE;
      }

      if (NumOfSingle != 0)
      {
        sEE_WritePage(hspi, pBuffer, WriteAddr, NumOfSingle);
      }
    }
  }
}


/**
  * @brief  Writes more than one byte to the EE with a single WRITE cycle 
  *         (Page WRITE sequence).
  * @note   The number of byte can't exceed the EE page size.
  * @param  pBuffer: pointer to the buffer  containing the data to be written
  *         to the EE.
  * @param  WriteAddr: EE's internal address to write to.
  * @param  NumByteToWrite: number of bytes to write to the EE, must be equal
  *         or less than "sEE_PAGESIZE" value.
  * @retval None
  */
void sEE_WritePage(SPI_HandleTypeDef *hspi, uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
  uint8_t tmparr[3];  

  /*!< Enable the write access to the EE */
  sEE_WriteEnable(hspi);

  /*!< Select the EE: Chip Select low */
  sEE_CS_LOW();
 
  /*!< Send "Write to Memory " instruction */
  /*!< Send WriteAddr  address byte to write to */
  tmparr[0] = sEE_CMD_WRITE;
  tmparr[1] = (WriteAddr& 0xFF00) >> 8;
  tmparr[2] = WriteAddr & 0xFF;
  
  if(HAL_SPI_Transmit(hspi, tmparr, 3, 1000) != HAL_OK) Error_Handler();
  
  if(HAL_SPI_Transmit(hspi, pBuffer, NumByteToWrite, 1000) != HAL_OK)Error_Handler();
  
  /*!< Deselect the EE: Chip Select high */
  sEE_CS_HIGH();

  /*!< Wait the end of EE writing */
  sEE_WaitForWriteEnd(hspi);
}


/**
  * @brief  Enables the write access to the EE.
  * @param  None
  * @retval None
  */
void sEE_WriteEnable(SPI_HandleTypeDef *hspi)
{
  uint8_t tmp;

  tmp = sEE_CMD_WREN;
  /*!< Select the EE: Chip Select low */
  sEE_CS_LOW();

  /*!< Send "Write Enable" instruction */
  if(HAL_SPI_Transmit(hspi, &tmp, 1, 1000) != HAL_OK)Error_Handler();

  /*!< Deselect the EE: Chip Select high */
  sEE_CS_HIGH();
}

/**
  * @brief  Polls the status of the Write In Progress (WIP) flag in the EE's
  *         status register and loop until write operation has completed.
  * @param  None
  * @retval None
  */
void sEE_WaitForWriteEnd(SPI_HandleTypeDef *hspi)
{
  uint8_t EEstatus = 0, tmp = sEE_CMD_RDSR;

  /*!< Select the EE: Chip Select low */
  sEE_CS_LOW();

  /*!< Send "Read Status Register" instruction */
  if(HAL_SPI_Transmit(hspi, &tmp, 1, 1000) != HAL_OK)Error_Handler();

  /*!< Loop as long as the memory is busy with a write cycle */
  do
  {
    /*!< Send a dummy byte to generate the clock needed by the EE
    and put the value of the status register in EE_Status variable */
    if(HAL_SPI_Receive(hspi, &EEstatus, 1, 1000) != HAL_OK)Error_Handler();
  }
  while ((EEstatus & sEE_WIP_FLAG) == SET); /* Write in progress */

  /*!< Deselect the EE: Chip Select high */
  sEE_CS_HIGH();
}

