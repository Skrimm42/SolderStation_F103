//#include "ssd1306_fonts.h"

#ifndef SSD1306_FONTS_H
#define SSD1306_FONTS_H 120

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif
  
#include "stm32f1xx_hal.h"
#include "string.h"


/**
 * @brief  Font structure used on my LCD libraries
 */

  
// This structure describes a single character's display information
typedef struct
{
	const uint8_t widthBits;					// width, in bits (or pixels), of the character
	const uint16_t offset;					// offset of the character's bitmap, in bytes, into the the FONT_INFO's data array
	
} FONT_CHAR_INFO;	

// Describes a single font
typedef struct
{
	const uint8_t 			heightPixels;	// height, in pixels, of the font's characters
	const uint8_t 			startChar;		// the first character in the font (e.g. in charInfo and data)
	const uint8_t 			endChar;		// the last character in the font
	const uint8_t			spacePixels;	// number of pixels that a space character takes up
	const FONT_CHAR_INFO*	        charInfo;		// pointer to array of char information
	const uint8_t*			data;			// pointer to generated array of character visual representation
		
} FONT_INFO;	
  

extern FONT_INFO palatinoLinotype_12ptFontInfo;
extern FONT_INFO dSEG7Classic_20ptFontInfo;
extern FONT_INFO lessPerfectDOSVGA_13ptFontInfo;
extern FONT_INFO mSGothic_12ptFontInfo;
extern FONT_INFO segoeUI_8ptFontInfo;
/* C++ detection */
#ifdef __cplusplus
}
#endif


#endif//SSD1306_FONTS_H

