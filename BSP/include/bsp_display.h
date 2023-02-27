/*
 * bsp_display.h
 *
 *  Created on: Sep 6, 2020
 *      Author: milos
 */

#ifndef INC_BSP_DISPLAY_H_
#define INC_BSP_DISPLAY_H_

#include <stdint.h>
#include <fonts.h>

#define MAKE_RGB565(r, g, b) ((uint16_t)((((uint16_t)r & 0xF8)<<8) | ((uint16_t)g & 0xFC)<<3) | (((uint16_t)b & 0xF8)>>3))

extern uint32_t FBStartAdress;

#define FBStartAdressF1 (0xc0000000)
#define FBStartAdressF2 (0xc01C2000)

typedef enum
{
    DISPLAY_UNKNOWN,
    DISPLAY_RGB_2_8,
    DISPLAY_RGB_3_5,
    DISPLAY_RGB_4_3,
    DISPLAY_RGB_5_0,
    DISPLAY_RGB_7_0,
    DISPLAY_IPS_3_5,
    DISPLAY_IPS_4_3,
    DISPLAY_IPS_5_0,
    DISPLAY_IPS_7_0,
	DISPLAY_IPS_10_1,
    _DISPLAY_COUNT

} display_t;


/**
* @brief  LCD Drawing main properties
*/
typedef struct
{
  uint32_t TextColor; /*!< Specifies the color of text */
  uint32_t BackColor; /*!< Specifies the background color below the text */
  sFONT    *pFont;    /*!< Specifies the font used for the text */

} LCD_DrawPropTypeDef;

typedef enum
{
  CENTER_MODE             = 0x01,    /*!< Center mode */
  RIGHT_MODE              = 0x02,    /*!< Right mode  */
  LEFT_MODE               = 0x03     /*!< Left mode   */

} Text_AlignModeTypdef;

void
BSP_Display_Initialize( display_t type );

uint32_t
BSP_Display_GetWidth( display_t type );

uint32_t
BSP_Display_GetHeight( display_t type );

void
BSP_Display_Resolution( uint32_t * w, uint32_t * h );

void
BSP_Display_Enable( void );

void
BSP_Display_SetBackgroung( uint8_t r, uint8_t g, uint8_t b );

void
BSP_Display_PaintBox( uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey,
                          uint16_t color );

void
BSP_Display_Pixel( uint32_t x, uint32_t y, uint16_t color );

void BSP_LCD_DrawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t Radius);

void
BSP_Display_LoopTest( void );

void BSP_Display_SwapFrame(void);

void     BSP_LCD_SetTextColor(uint32_t Color);
uint32_t BSP_LCD_GetTextColor(void);
void     BSP_LCD_SetBackColor(uint32_t Color);
uint32_t BSP_LCD_GetBackColor(void);
void     BSP_LCD_SetFont(sFONT *fonts);

void     BSP_LCD_DisplayStringAt(uint16_t Xpos, uint16_t Ypos, uint8_t *Text, Text_AlignModeTypdef Mode);
void     BSP_LCD_DisplayChar(uint16_t Xpos, uint16_t Ypos, uint8_t Ascii);

#endif /* INC_BSP_DISPLAY_H_ */
