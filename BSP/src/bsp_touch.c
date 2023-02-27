/*
 * bsp_touch.c
 *
 *  Created on: Aug 26, 2020
 *      Author: milos
 */

#include "bsp_touch.h"
#include "bsp_print.h"
#include "i2c.h"

/// Macros -------------------------------------------------------------------

#define RGB_TP_ADDRESS ( 0x38 << 1 )
#define RGB_TP_REG_OFFSET ( 0x03 )
#define RGB_TP_REG_REP ( 6 )
#define RGB_TP_REG_SIZE ( 4 )


TouchPanel_t TouchPanel;

/// Constants -----------------------------------------------------------------

static const uint16_t CAP_TOUCH_RESOLUTION[_CAP_TOUCH_COUNT][2] = {
    //  W    H
    { 0, 0 },                       //  NO TOUCH PLACETAKER
    { 0, 0 },                       //	CAP_TOUCH_RGB_2_8
    { 896, 640 },                   //	CAP_TOUCH_RGB_3_5
    { 1280, 768 },                  //	CAP_TOUCH_RGB_4_3
    { 800, 480 },                   //	CAP_TOUCH_RGB_5_0
    { 1792, 1024 },                 //	CAP_TOUCH_RGB_7_0

    /*
     *  TODO:
     *  Add proper TP resolutions for IPS displays here.
     */

    { 0, 0 },                       //  CAP_TOUCH_IPS_3_5
    { 0, 0 },                       //  CAP_TOUCH_IPS_4_3
    { 0, 0 },                       //  CAP_TOUCH_IPS_5_0
    { 0, 0 }                        //  CAP_TOUCH_IPS_7_0
};

/// Variables -----------------------------------------------------------------

static cap_touch_t ct;

/// Public Functions ----------------------------------------------------------

int
BSP_CapTouch_Initialize( cap_touch_t type )
{
    ct = type;

    switch ( ct )
    {
    case CAP_TOUCH_RGB_2_8:
    case CAP_TOUCH_RGB_3_5:
    case CAP_TOUCH_RGB_4_3:
    case CAP_TOUCH_RGB_5_0:
    case CAP_TOUCH_RGB_7_0:
    {
        uint8_t tx_test = 0;
        uint8_t tx_buf[2] = { 0x00, 0x00 };

        HAL_GPIO_WritePin( LCD_TPRST_GPIO_Port, LCD_TPRST_Pin, GPIO_PIN_RESET );
        HAL_Delay( 100 );
        HAL_GPIO_WritePin( LCD_TPRST_GPIO_Port, LCD_TPRST_Pin, GPIO_PIN_SET );
        HAL_Delay( 2000 );

        while ( HAL_OK !=
                HAL_I2C_Master_Transmit( &hi2c1, RGB_TP_ADDRESS, tx_buf, 2, 100 ) )
        {
            if ( ++tx_test > 5 )
            {
                return 1;
            }

            HAL_Delay( 50 );
        }

        break;
    }
    case CAP_TOUCH_IPS_3_5:
    case CAP_TOUCH_IPS_4_3:
    case CAP_TOUCH_IPS_5_0:
    case CAP_TOUCH_IPS_7_0:
    {
        uint8_t touch_data[16];
		uint8_t status;
		HAL_StatusTypeDef ret;

		HAL_GPIO_WritePin( LCD_TPRST_GPIO_Port, LCD_TPRST_Pin, GPIO_PIN_RESET );
		HAL_Delay( 100 );
		HAL_GPIO_WritePin( LCD_TPRST_GPIO_Port, LCD_TPRST_Pin, GPIO_PIN_SET );
		HAL_Delay( 2000 );

      ret = HAL_I2C_Mem_Read(&hi2c1, 0x82, 0x31, 1, touch_data, 1, 100);
      BSP_Print("WakeUp %02x\n\r",  touch_data[0]);


		  uint32_t  x,y;

  	  while(hi2c1.State == HAL_I2C_STATE_BUSY);
   	  ret = HAL_I2C_Mem_Read(&hi2c1, 0x82, 0x20, 1, touch_data, 14, 100);

   	  x = (0xff & touch_data[1]) << 8;
   	  x |=touch_data[0];

   	  y = (0xff & touch_data[3]) << 8;
   	  y |=touch_data[2];


   	  BSP_Print("resolution x = %04d y = %04d \n\r", x, y);
    }
        break;
    default:

        break;
    }

    return 0;
}

void
BSP_CapTouch_Resolution( uint32_t * w, uint32_t * h )
{
    *w = CAP_TOUCH_RESOLUTION[ct][0];
    *h = CAP_TOUCH_RESOLUTION[ct][1];
}

bool
BSP_CapTouch_Detected( void )
{
    switch ( ct )
    {
    case CAP_TOUCH_RGB_2_8:
    case CAP_TOUCH_RGB_3_5:
    case CAP_TOUCH_RGB_4_3:
    case CAP_TOUCH_RGB_5_0:
    case CAP_TOUCH_RGB_7_0:

        if ( !HAL_GPIO_ReadPin( LCD_TPINT_GPIO_Port, LCD_TPINT_Pin ) )
        {
            return true;
        }

        break;
    case CAP_TOUCH_IPS_3_5:
    case CAP_TOUCH_IPS_4_3:
    case CAP_TOUCH_IPS_5_0:
    case CAP_TOUCH_IPS_7_0:

    	if ( !HAL_GPIO_ReadPin( LCD_TPINT_GPIO_Port, LCD_TPINT_Pin ) )
        {
            return true;
        }
        break;
    default:

        break;
    }

    return false;
}

int
BSP_CapTouch_Read( TouchPanel_t *TouchPanel)
{
    switch ( ct )
    {
    case CAP_TOUCH_IPS_3_5:
    case CAP_TOUCH_IPS_4_3:
    case CAP_TOUCH_IPS_5_0:
    case CAP_TOUCH_IPS_7_0:

    {
    	uint8_t touch_data[64];
    	uint8_t status;
    	HAL_StatusTypeDef ret;

    	uint32_t tx=0, ty=0;

    	TouchPanel->NOfTouch=0;

       while(hi2c1.State == HAL_I2C_STATE_BUSY);
	   ret = HAL_I2C_Mem_Read(&hi2c1, 0x82, 0x10, 1, touch_data, 50, 100);



	   for(int i=0; i<10; i++)
	   {
		  status = touch_data[1+i*5];

		  tx = touch_data[3+i*5];
		  tx = tx << 8;
		  tx |= touch_data[2+i*5];

		  ty = touch_data[5+i*5];
		  ty = ty << 8;
		  ty |= touch_data[4+i*5];

		  if((status & 0xF0) == 0x40)
		  {
			  TouchPanel->NOfTouch++;
			  TouchPanel->XY[i].x = tx;
			  TouchPanel->XY[i].y = ty;
		  }
		}

    }


        break;
    default:

        break;
    }

    return 0;
}

//void
//BSP_CapTouch_LoopTest( uint32_t delay )
//{
//    unsigned long start = HAL_GetTick( );
//
//    while ( ( start + delay ) > HAL_GetTick( ) )
//    // for ( ;; )	///	Block when testing.
//    {
//        if ( BSP_CapTouch_Detected( ) )
//        {
//            uint32_t X;
//            uint32_t Y;
//
//            if ( !BSP_CapTouch_Read( &X, &Y ) )
//            {
//                BSP_Print( "\r\n\tTouch ( %lu, %lu )", X, Y );
//            }
//        }
//    }
//}
