/*
 * demo_drawer.c
 *
 *  Created on: Sep 27, 2020
 *      Author: milos
 */

#include "App_Common.h"
#include "demo.h"
#include "platform.h"
#include "Gpu.h"


#define BRUSH_SIZE ( 3 )
#define BRUSH_COLOR ( 0x001F )

//	Do not allow pixel out of the bounds.
#define BRUSH_SX( x ) ( ( ( x ) >= BRUSH_SIZE ) ? ( x )-BRUSH_SIZE : 0 )
#define BRUSH_EX( x )                                                          \
    ( ( ( x ) <= ( d_w - BRUSH_SIZE ) ) ? ( x ) + BRUSH_SIZE : d_w )
#define BRUSH_SY( y ) ( ( ( y ) >= BRUSH_SIZE ) ? ( y )-BRUSH_SIZE : 0 )
#define BRUSH_EY( y )                                                          \
    ( ( ( y ) <= ( d_h - BRUSH_SIZE ) ) ? ( y ) + BRUSH_SIZE : d_h )

const uint16_t PaintColor[5] = {0, 0x001f, 0x07e0, 0xf800, 0xfe40 };

void
DEMO_CapacitiveTouchDraw( void )
{
    //if ( BSP_CapTouch_Detected( ) )
    {
        if ( TouchPanel.Touch )
        {

            /*
             * 	Proportion due to different resolution of the touch
             * 	and display.
             *
             * 	X : T_H = (?) : D_H
             * 	Y : T_W = (?) : D_W
             */
//            x = ( x * d_w ) / t_w;
//            y = ( y * d_h ) / t_h;

        	for(int i=0; i< TouchPanel.NOfTouch; i++)
        	{
        		BSP_Display_PaintBox( BRUSH_SX( TouchPanel.XY[i].x ), BRUSH_SY( TouchPanel.XY[i].y ), BRUSH_EX( TouchPanel.XY[i].x ),
        				BRUSH_EY( TouchPanel.XY[i].y ), PaintColor[i] );
        	}
        }
    }
}

void
DEMO_ResistiveTouchDraw( void )
{
    uint16_t x;
    uint16_t y;

    if ( BSP_AdcTouch_Point( &x, &y ) )
    {
        BSP_Display_PaintBox( BRUSH_SX( x ), BRUSH_SY( y ), BRUSH_EX( x ),
                              BRUSH_EY( y ), BRUSH_COLOR );
    }
}

extern Gpu_Hal_Context_t host;

void
DEMO_RibusTouchDraw( void )
{
    //App_Sketch( &host );
    sketch_black();

}
