//ssd1306.c

#include "stm32f1xx_hal.h"

#include "ssd1306.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


char SSD1306_Putspace(FONT_INFO* Font, SSD1306_COLOR_t color);


// Databuffer voor het scherm
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

// Een scherm-object om lokaal in te werken
static SSD1306_t SSD1306;

int SSD1306_Init(I2C_HandleTypeDef* i2c_port, uint8_t address)
{	
  SSD1306.I2C_port = i2c_port;
  SSD1306.Address = address;
  // Waiting for SEG/COM ON after reset
  HAL_Delay(100);
  // Initialize LCD
  uint8_t init_data[] = {0xAE, 0xA6, 0x20, 0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07, 0xA1, 0xC8, 0xA8, 0x3F, 0x81, 0x7F, 0x8D, 0x14, 0xAF};
//  uint8_t init_data[] = {0xAE, 0x20, 0x10, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 
//                         0xF0, 0xD9, 0x22, 0xDA, 0x12, 0xDB, 0x20, 0x8D, 0x14, 0xAF};
  
  // 0xAE - display off
  // 0xA6 - normal (not inversed)
  // 0x20, 0x00 - page horizontal adressing mode
  // 0x21, 0x00, 0x7F - column address from 0 to 127
  // 0x22, 0x00, 0x07 - page address from 0 to 7
  // 0xA1 - segment re-map (vertical mirroring)
  // 0xC8 - COM scan direction (horizontal mirroring)
  // 0xA8, 0x3F - multiplex ratio
  // 0x81, 0x7F - contrast ratio 127 
  // 0x8D, 0x14 - enable charge pump
  // 0xAF - display on (only just after enabling charge pump)
  ssd1306_SendToDisplay(Commands, init_data, sizeof(init_data));
  // clearing screen
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_UpdateScreen();
  // setting default position
  SSD1306.CurrentX = 0;
  SSD1306.CurrentY = 0;
  return 0;
}


void SSD1306_UpdateScreen(void) 
{
  uint8_t update_region_data[6] = {0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
  //0x21, 0x00, 0x7F - column address from 0 to 127
  //0x22, 0x00, 0x07 - page address from 0 to 7
  ssd1306_SendToDisplay(Commands, update_region_data, 6);

  //update pages from 0 to 7
//  for(uint32_t page = 0; page < 8; page++) 
//  {
//    ssd1306_SendToDisplay(Datas, &SSD1306_Buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);
//  }

  ssd1306_SendToDisplay(Datas, SSD1306_Buffer, 1024);
}


void SSD1306_SetContrast(uint8_t contrast)
{
  //contrast command with empty data
  uint8_t contrast_data[2] = {0x81};
  //adding contrast data
  contrast_data[1] = contrast;
  //and send to display
  ssd1306_SendToDisplay(Commands, contrast_data, 2);
}



static void ssd1306_SendToDisplay(SSD1306_DATA_TYPE type, uint8_t *data, uint16_t length)
{
#ifdef USE_DMA
  while(HAL_I2C_GetState(SSD1306.I2C_port)!= HAL_I2C_STATE_READY);
  if(type == Commands) 
  {
    HAL_I2C_Mem_Write(SSD1306.I2C_port, SSD1306.Address, type, 1, data, length, 100); 
    return;
  }
  HAL_I2C_Mem_Write_DMA(SSD1306.I2C_port, SSD1306.Address, type, 1, data, length);     
#else        
  HAL_I2C_Mem_Write(SSD1306.I2C_port, SSD1306.Address, type, 1, data, length, 100);   
#endif
}


//---------------------------


void SSD1306_ToggleInvert(void) {
	uint16_t i;

	/* Toggle invert */
	SSD1306.Inverted = !SSD1306.Inverted;

	/* Do memory toggle */
	for (i = 0; i < sizeof(SSD1306_Buffer); i++) {
		SSD1306_Buffer[i] = ~SSD1306_Buffer[i];
	}
}

void SSD1306_Fill(SSD1306_COLOR_t color) {
	/* Set memory */
	memset(SSD1306_Buffer, (color == SSD1306_COLOR_BLACK) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color) {
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Error */
		return;
	}

	/* Check if pixels are inverted */
	if (SSD1306.Inverted) {
		color = (SSD1306_COLOR_t)!color;
	}

	/* Set color */
	if (color == SSD1306_COLOR_WHITE) {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} else {
		SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}

void SSD1306_GotoXY(uint16_t x, uint16_t y) {
	/* Set write pointers */
	SSD1306.CurrentX = x;
	SSD1306.CurrentY = y;
}

uint16_t SSD1306_GetCurrentX(void)
{
  return SSD1306.CurrentX;
}

uint16_t SSD1306_GetCurrentY(void)
{
  return SSD1306.CurrentY;
}

char SSD1306_Putspace(FONT_INFO* Font, SSD1306_COLOR_t color) {
  
  uint8_t width_x = (Font -> spacePixels)*3;
  uint8_t height_y = (Font -> heightPixels); 
  SSD1306_DrawFilledRectangle(SSD1306.CurrentX, SSD1306.CurrentY, width_x, height_y, SSD1306_COLOR_BLACK);
  /* Increase pointer */
  SSD1306.CurrentX += width_x;
  /* Return character written */
  return 0x20; //space hex ascii code
}

char SSD1306_Putc(char ch, FONT_INFO* Font, SSD1306_COLOR_t color) {
  
  uint32_t i, b, j, character;
  uint8_t x;
  
  if(ch == 0x20) return SSD1306_Putspace(Font, color);    // space char
  
  if(Font == &dSEG7Classic_20ptFontInfo) ch = ch - 12;
  
  uint16_t f_height = Font -> heightPixels;
  uint8_t f_width = Font -> charInfo[ch-33].widthBits;
  
  /* Check available space in LCD */
  if (
      SSD1306_WIDTH <= (SSD1306.CurrentX + f_width) ||
        SSD1306_HEIGHT <= (SSD1306.CurrentY + f_height)
          ) {
            /* Error */
            return 0;
          }
  
  /* Go through font */
  //How much bytes in character row
  if(f_width <= 8)x = 1;
  else if((f_width > 8) && (f_width <= 16)) x = 2;
  else if((f_width > 16) && (f_width <= 24)) x = 3;
  else if((f_width > 24) && (f_width <= 32)) x = 4;
  else return 0; // Error
  
  for (i = 0; i < f_height; i++) 
  {
    b = 0;
    for(j = 0; j < x; j++)//get char row
    {
      character = (Font -> data[Font -> charInfo[ch-33].offset + i*x + j]);          
      if(j > 0)b = character | (b << 8);
      else b = character;
    }
    b = b << (32-8*x);//MSB bit first
    
    for (j = 0; j < f_width; j++) {
      if ((b << j) & 0x80000000) {
        SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR_t) color);
      } else {
        SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR_t)!color);
      }
    }
    for (j = 0; j < Font -> spacePixels; j++) {
      // space pixels between chars
      SSD1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY), (SSD1306_COLOR_t)!color);
    }
  }
  
  /* Increase pointer */
  SSD1306.CurrentX += (f_width + Font -> spacePixels);
  
  /* Return character written */
  return ch;
}

char SSD1306_Puts(char* str, FONT_INFO* Font, SSD1306_COLOR_t color) {
	/* Write characters */
	while (*str) {
		/* Write character by character */
		if (SSD1306_Putc(*str, Font, color) != *str) {
			/* Return error */
			return *str;
		}

		/* Increase string pointer */
		str++;
	}

	/* Everything OK, zero should be returned */
	return *str;
}


void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp;

	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) {
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH) {
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT) {
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT) {
		y1 = SSD1306_HEIGHT - 1;
	}

	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1);
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1);
	sx = (x0 < x1) ? 1 : -1;
	sy = (y0 < y1) ? 1 : -1;
	err = ((dx > dy) ? dx : -dy) / 2;

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			SSD1306_DrawPixel(x0, i, c);
		}

		/* Return from function */
		return;
	}

	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}

		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}

		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			SSD1306_DrawPixel(i, y0, c);
		}

		/* Return from function */
		return;
	}

	while (1) {
		SSD1306_DrawPixel(x0, y0, c);
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err;
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		}
	}
}

void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	/* Check input parameters */
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw 4 lines */
	SSD1306_DrawLine(x, y, x + w, y, c);         /* Top line */
	SSD1306_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	SSD1306_DrawLine(x, y, x, y + h, c);         /* Left line */
	SSD1306_DrawLine(x + w, y, x + w, y + h, c); /* Right line */
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	uint8_t i;

	/* Check input parameters */
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}

	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}

	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		SSD1306_DrawLine(x, y + i, x + w, y + i, c);
	}
}

void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	/* Draw lines */
	SSD1306_DrawLine(x1, y1, x2, y2, color);
	SSD1306_DrawLine(x2, y2, x3, y3, color);
	SSD1306_DrawLine(x3, y3, x1, y1, color);
}


void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
	yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
	curpixel = 0;

	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		SSD1306_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, c);
        SSD1306_DrawPixel(x0 - x, y0 + y, c);
        SSD1306_DrawPixel(x0 + x, y0 - y, c);
        SSD1306_DrawPixel(x0 - x, y0 - y, c);

        SSD1306_DrawPixel(x0 + y, y0 + x, c);
        SSD1306_DrawPixel(x0 - y, y0 + x, c);
        SSD1306_DrawPixel(x0 + y, y0 - x, c);
        SSD1306_DrawPixel(x0 - y, y0 - x, c);
    }
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);
    SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }
}


/**
 * Send a formatted string to the LCD
 *
 * @param fmt	String format
 * @param ...	Variable arguments (see printf())
 */
void SSD1306_printf(FONT_INFO* Font, const char *fmt, ...) {

	uint16_t i;
	uint16_t size;
	uint8_t character;
	char buffer[32];
	va_list args;

	va_start(args, fmt);
	size = vsprintf(buffer, fmt, args);
        
	for (i = 0; i < size; i++) {
		character = buffer[i];

		if (character == 10)
			break;
		else
			SSD1306_Putc(character, Font, SSD1306_COLOR_WHITE);
	}
}
