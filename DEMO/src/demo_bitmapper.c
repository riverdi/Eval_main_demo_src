/*
 * demo_bitmapper.c
 *
 *  Created on: Sep 27, 2020
 *      Author: milos
 */

#include "App_Common.h"
#include "CoPro_Cmds.h"
#include "Gpu.h"
#include "demo.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "Hal_Utils.h"

//#define MAKE_RGB565(r, g, b) ((uint16_t)((((uint16_t)r & 0xF8)<<8) + ((uint16_t)g & 0xFC)<<3) + (((uint16_t)b & 0xF8)>>3))
/// Types ---------------------------------------------------------------------

extern enum dev_size device_size;

#pragma pack( push, 1 )
struct file_meta
{
    uint16_t signature;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixelArrayOffset;
};

struct bitmap_meta
{
    uint32_t dibHeaderSize;
    int32_t imageWidth;
    int32_t imageHeight;
    uint16_t numColorPlanes;
    uint16_t bitsPerPixel;
    uint32_t compressionMethod;
    uint32_t totalImageSize;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t numColorsInPalette;
    uint32_t numImportantColors;
};
#pragma pack( pop )

/// Declarations---------------------------------------------------------------

static int
_compose_bitmap_filename( uint8_t file_index, char * fname );

static int
_compose_bitmap_filename_ribus( uint8_t file_index, char * fname );

static int
_display_bitmap( char * fname );

static int
_display_bitmap_ribus( char * fname );

static int
_verify_file_meta( struct file_meta * desc );

static int
_verify_bmp_meta( struct bitmap_meta * desc );


void JpegFromFile(Gpu_Hal_Context_t *phost, const char * fileName);
void LoadJpegImageFromFlash(Gpu_Hal_Context_t *phost, uint32_t adr);
void LoadJpegImageMediaFifo(Gpu_Hal_Context_t *phost, const char * fileName);
void Load_ImageASTC(Gpu_Hal_Context_t *phost, uint32_t adr, uint16_t fmt);
void playMovie(uint32_t adr);
/// Exported ------------------------------------------------------------------

int
DEMO_DisplayBitmapFromSd( uint8_t idx )
{
    char filename[256];

    if ( _compose_bitmap_filename( idx, filename ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Filename compose failed" );
        return 1;
    }

    if ( _display_bitmap( filename ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Display image failed" );
        return 1;
    }

    return 0;
}

int
DEMO_DisplayBitmapFromSd_Ribus( uint8_t idx )
{
    char filename[256];

    if ( _compose_bitmap_filename_ribus( idx, filename ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Filename compose failed" );
        return 1;
    }

//    if ( _display_bitmap_ribus( filename ) )
//    {
//        BSP_Print( "\r\n[DEMO RUN] Display image failed" );
//        return 1;
//    }
    if(device_size < DEV_7_0)
    	JpegFromFile(&host, filename);//LoadJpegImageMediaFifo(phost, filename);//
    else
    	Load_ImageASTC(phost, 4096 + idx * 1024 * 1024, COMPRESSED_RGBA_ASTC_6x6_KHR);


    return 0;
}

/// Privates ------------------------------------------------------------------

static int
_compose_bitmap_filename( uint8_t file_index, char * fname )
{
    char tmp[16];

    //	Default file system.
    strcpy( fname, "0:/" );

    //	Default folder location depending on screen resolution.
    snprintf( tmp, sizeof( tmp ), "%lu", d_w );
    strcat( fname, tmp );
    strcat( fname, "x" );
    snprintf( tmp, sizeof( tmp ), "%lu", d_h );
    strcat( fname, tmp );
    strcat( fname, "/" );

    //	Filename composed of index and default extension.
    snprintf( tmp, sizeof( tmp ), "%d", file_index );
    strcat( fname, tmp );
    strcat( fname, ".bmp" );
    BSP_Print( "\r\n[DEMO RUN] Filename %s: ", fname );

    return 0;
}

static int
_compose_bitmap_filename_ribus( uint8_t file_index, char * fname )
{
    char tmp[16];

#if 1
	//	Default file system.
	strcpy( fname, "0:/" );

	//	Default folder location depending on screen resolution.
	snprintf( tmp, sizeof(tmp), "%lu", d_w );
	strcat( fname, tmp );
	strcat( fname, "x" );
	snprintf( tmp, sizeof(tmp), "%lu", d_h );
	strcat( fname, tmp );
	strcat( fname, "/" );
#endif

    //strcpy( fname, "0:/480x272/" );

    //	Filename composed of index and default extension.
    snprintf( tmp, sizeof( tmp ), "%d", file_index );
    strcat( fname, tmp );
    strcat( fname, ".jpg" );
    BSP_Print( "\r\n[DEMO RUN] Filename %s: ", fname );

    return 0;
}

static int
_display_bitmap( char * fname )
{
    FIL file;
    UINT read;
    uint8_t tmp_buf[1024];
    struct file_meta file_desc;
    struct bitmap_meta bmp_desc;

    uint16_t pixel;

    if ( FR_OK != f_open( &file, fname, FA_READ ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Can't open file" );
        return 1;
    }

    //	Read file meta.
    if ( FR_OK !=
         f_read( &file, &file_desc, sizeof( struct file_meta ), &read ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Can't read file metadata" );
        f_close( &file );
        return 1;
    }
    
	//	Read bitmap meta.
    if ( FR_OK !=
         f_read( &file, &tmp_buf, file_desc.pixelArrayOffset - sizeof( struct file_meta ), &read ) )
    {
    	BSP_Print( "\r\n[DEMO RUN] Can't read bitmap metadata" );
        f_close( &file );
        return 1;
    }

    memcpy( &bmp_desc, tmp_buf, sizeof( struct bitmap_meta ) );

    if ( _verify_file_meta( &file_desc ) )
    {
        BSP_Print( "\r\n[DEMO RUN] File metadata verification failed" );
        f_close( &file );
        return 1;
    }

    if ( _verify_bmp_meta( &bmp_desc ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Bitmap metadata verification failed" );
        f_close( &file );
        return 1;
    }

    //	Read bitmap data - row by row, starting from the bottom.
    for ( int32_t rc = bmp_desc.imageHeight; rc > 0; --rc )
    {
        uint8_t row_buffer[1024*3];    //	Bytes per pixel * max row width => 3 *
                                     //800 = 2400
        FRESULT ffres;
        //	Load row from file to buffer.

        ffres = f_read( &file, row_buffer, ( bmp_desc.imageWidth * 3 ), &read );

        if ( FR_OK != ffres)
        {
            BSP_Print( "\r\n[DEMO RUN] Bitmap data read failed" );
            f_close( &file );
            return 1;
        }

        //	Display row from buffer - left to right.
        for ( int32_t cc = 0; cc < bmp_desc.imageWidth-1; cc++ )
        {


            /*
             * Convert bmp format to frame buffer format.
             *  3 bytes per pixel to RGB565.
             */
//            pixel |= row_buffer[cc * 3 + 2];
//            pixel <<= 8;
//            pixel |= row_buffer[cc * 3 + 1];
//            pixel <<= 8;
//            pixel |= row_buffer[cc * 3];

//            pixel = ((uint16_t)row_buffer[cc * 3 + 2])& 0xF8;
//            pixel = (pixel << 8);
//            pixel |=((uint16_t)row_buffer[cc * 3 + 1] & 0xFC)<<3;
//            pixel |=((uint16_t)row_buffer[cc * 3] & 0xF8)>>3;

            pixel = MAKE_RGB565(row_buffer[cc * 3 + 2],row_buffer[cc * 3 + 1],row_buffer[cc * 3 ]);

           	BSP_Display_Pixel( (uint32_t)cc, (uint32_t)rc-1, pixel );
        }
    }

    f_close( &file );

    return 0;
}

static int
_display_bitmap_ribus( char * fname )
{
    //	Download data to the RAM

    FIL file;
    UINT read;
    struct file_meta file_desc;
    struct bitmap_meta bmp_desc;

    if ( FR_OK != f_open( &file, fname, FA_READ ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Can't open file" );
        return 1;
    }

    //	Read and verify file meta.
    if ( FR_OK !=
         f_read( &file, &file_desc, sizeof( struct file_meta ), &read ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Can't read file metadata" );
        f_close( &file );
        return 1;
    }

#if 0
	if (( read != sizeof( struct file_meta ) ) || ( _verify_file_meta( &file_desc ) ) )
	{
		BSP_Print( "\r\n[DEMO RUN] File metadata verification failed" );
		f_close( &file );
		return 1;
	}
#endif

    //	Read bitmap meta.
    if ( FR_OK !=
         f_read( &file, &bmp_desc, sizeof( struct bitmap_meta ), &read ) )
    {
        BSP_Print( "\r\n[DEMO RUN] Can't read bitmap metadata" );
        f_close( &file );
        return 1;
    }

#if 0
	if (( read != sizeof( struct bitmap_meta ) ) || ( _verify_bmp_meta( &bmp_desc ) ) )
	{
		BSP_Print( "\r\n[DEMO RUN] Bitmap metadata verification failed" );
		f_close( &file );
		return 1;
	}
#endif

    //	Read bitmap data - row by row, starting from the bottom.
    for ( int32_t rc = bmp_desc.imageHeight; rc > 0; --rc )
    {
        uint8_t row_buffer[2400];    //	Bytes per pixel * max row width => 3 *
                                     //800 = 2400
        uint8_t rgb_buffer[1600];    //	Bytes per pixel * max row width => 2 *
                                     //800 = 1600

        //	Load row from file to buffer.
        if ( FR_OK !=
             f_read( &file, row_buffer, ( bmp_desc.imageWidth * 3 ), &read ) )
        {
            BSP_Print( "\r\n[DEMO RUN] Bitmap data read failed" );
            f_close( &file );
            return 1;
        }

        //	Display row from buffer - left to right.
        for ( int32_t cc = 0; cc < bmp_desc.imageWidth; ++cc )
        {
            /*
             * Convert bmp format to frame buffer format.
             *  3 bytes per pixel to 2 bytes per pixel RGB565.
             */
            rgb_buffer[cc * 2 + 1] = ( row_buffer[cc * 3 + 2] & 0xF8 );
            rgb_buffer[cc * 2 + 1] |= ( row_buffer[cc * 3 + 1] >> 5 );
            rgb_buffer[cc * 2] = ( row_buffer[cc * 3 + 1] & 0xE0 );
            rgb_buffer[cc * 2] |= ( row_buffer[cc * 3] >> 3 );
        }

        Gpu_Hal_WrMem( &host, RAM_G + ( rc * bmp_desc.imageWidth * 2 ),
                       rgb_buffer, bmp_desc.imageWidth * 2 );
    }

    App_DisplayImage( &host, bmp_desc.imageWidth, bmp_desc.imageHeight, d_w,
                      d_h );
    f_close( &file );

    return 0;
}

static int
_verify_file_meta( struct file_meta * desc )
{
    //	Signature of the .bmp file according to documentation.
    if ( desc->signature != 0x4D42 )
    {
        return 1;
    }

    return 0;
}

static int
_verify_bmp_meta( struct bitmap_meta * desc )
{
    //	Width and height must match the display resolution.
    if ( desc->imageHeight != d_h )
    {
        BSP_Print( "\r\n[DEMO RUN] Wrong image height: %d", desc->imageHeight );
        return 1;
    }
    if ( desc->imageWidth != d_w )
    {
        BSP_Print( "\r\n[DEMO RUN] Wrong image width: %d", desc->imageWidth );
        return 1;
    }

    return 0;
}

void JpegFromFile(Gpu_Hal_Context_t *phost, const char * fileName)
{
	uint16_t iw = phost->display_config.DispWidth;
	uint16_t ih = phost->display_config.DispHeight;
	uint16_t format = RGB565;


	Gpu_CoCmd_Dlstart(phost);
	App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));


	uint32_t length=0;
	uint8_t  mem[10*1024];

	BSP_Print("fopen %s\r\n", fileName);


	FIL file;
	 UINT read;

	 FRESULT fres = f_open(&file, fileName, FA_READ);

	 if(fres != FR_OK)

	 {
		 BSP_Print("dupa \r\n");
	 }

	Gpu_CoCmd_LoadImage(phost, RAM_G, 0);
	uint32_t size=0;
	 do
	{
		f_read( &file, mem, 10*1024, &read );
		if(read == 0)
			break;
		else if(read < 10240)
		{

			read += 4 - (read % 4);
			Gpu_Hal_WrCmdBuf(phost, (uint8_t*)mem, read);
			break;
		}
		Gpu_Hal_WrCmdBuf(phost, (uint8_t*)mem, read);

		BSP_Print("size :%d \r\n", read);
	}while(read);

	 f_close( &file );




//		Gpu_CoCmd_SetBitmap(phost, RAM_G,RGB565 , iw, ih);
//		//Start drawing bitmap
//		App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
//		App_WrCoCmd_Buffer(phost, VERTEX2II(0, 0, 0, 0));
//		App_WrCoCmd_Buffer(phost, END());
//		App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());
//		App_WrCoCmd_Buffer(phost, DISPLAY());
//		Gpu_CoCmd_Swap(phost);
//		App_Flush_Co_Buffer(phost);
//		Gpu_Hal_WaitCmdfifo_empty(phost);


	Gpu_Hal_WaitCmdfifo_empty(phost);
	App_WrCoCmd_Buffer(phost, BITMAP_SOURCE(0L)); //specify the source address of
	//bitmap in RAM_Gc
	//specify bit map format, linestride and height for RGB565
	App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(RGB565, iw * 2, ih));
	App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT_H(((iw * 2)>>10), 0));
	//specify bit map format, linestride and height for L8
	//  Ft_App_WrCoCmd_Buffer(phost, BITMAP_LAYOUT(L8, 800 * 1, 480));
	//controls drawing of bitmap
	App_WrCoCmd_Buffer(phost, BITMAP_SIZE_H(iw>>9, ih>>9));
	App_WrCoCmd_Buffer(phost, BITMAP_SIZE(NEAREST, BORDER, BORDER, iw, ih));

	App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost, VERTEX2II(0,0, 0, 0));
	App_WrCoCmd_Buffer(phost,DISPLAY());
	Gpu_CoCmd_Swap(phost);

	/* Download the commands into fifo */
	App_Flush_Co_Buffer(phost);
	/* Wait till coprocessor completes the operation */
	Gpu_Hal_WaitCmdfifo_empty(phost);

}

void LoadJpegImageFromFlash(Gpu_Hal_Context_t *phost, uint32_t adr)
{
	Gpu_CoCmd_Dlstart(phost);
	App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

	//Gpu_CoCmd_FlashHelper_SwitchFullMode(&host);
	Gpu_CoCmd_FlashSource(phost, adr);
	Gpu_CoCmd_LoadImage(phost, 0, OPT_FLASH | OPT_FULLSCREEN );


	//Start drawing bitmap
	App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost, VERTEX2II(0, 0, 0, 0));
	App_WrCoCmd_Buffer(phost, END());

	App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());
	App_WrCoCmd_Buffer(phost, DISPLAY());
	Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);

	//platform_sleep_ms(3000);

}

#define MEDIAFIFO_SIZE (32*1024)
void LoadJpegImageMediaFifo(Gpu_Hal_Context_t *phost, const char * fileName)
{
	uint8_t mem[MEDIAFIFO_SIZE];

	uint32_t fifo_rd;
	uint32_t fifo_wr;

	Fifo_t stFifo;

	Fifo_Init(&stFifo, 1024*1024-MEDIAFIFO_SIZE, MEDIAFIFO_SIZE, REG_MEDIAFIFO_READ, REG_MEDIAFIFO_WRITE);

	Gpu_CoCmd_Dlstart(phost);
	App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

	Gpu_CoCmd_MediaFifo(phost, 1024*1024-MEDIAFIFO_SIZE , MEDIAFIFO_SIZE);
	Gpu_CoCmd_LoadImage(phost, 0, OPT_MEDIAFIFO |  OPT_FULLSCREEN);

	 FIL file;
	 UINT read;

	 FRESULT fres = f_open(&file, fileName, FA_READ);

	 if(fres != FR_OK)
	 {
		 BSP_Print("dupa \r\n");
	 }

	 fifo_rd = Gpu_Hal_Rd32(phost, REG_MEDIAFIFO_READ);
	 fifo_wr = Gpu_Hal_Rd32(phost, REG_MEDIAFIFO_WRITE);


	 do
	{
		f_read( &file, mem, MEDIAFIFO_SIZE, &read );


		fifo_wr = Gpu_Hal_Rd32(phost, REG_MEDIAFIFO_WRITE);
		do{
				fifo_rd=Gpu_Hal_Rd32(phost, REG_MEDIAFIFO_READ);

		 }while(fifo_rd<fifo_wr);

		Gpu_Hal_Wr32(phost, REG_MEDIAFIFO_WRITE, 0);

		if(read==0)
			break;

		Gpu_Hal_WrMem(phost, 1024*1024-MEDIAFIFO_SIZE, mem, MEDIAFIFO_SIZE);
		Gpu_Hal_Wr32(phost, REG_MEDIAFIFO_WRITE, read);
		fifo_wr = read;

	}while(read);



	 f_close( &file );

	//Start drawing bitmap
	App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost, VERTEX2II(0, 0, 0, 0));
	App_WrCoCmd_Buffer(phost, END());

	App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());
	App_WrCoCmd_Buffer(phost, DISPLAY());
	Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);

	//platform_sleep_ms(3000);

}

/**
 * @brief Video playback with audio
 *
 */


#define SCRATCH_BUFF_SZ (1024*8)

void VideoPlayback() {
	uint16_t aviw = 480;
	uint16_t avih = 272;
	uint8_t g_scratch[SCRATCH_BUFF_SZ];
	Fifo_t stFifo;
	uint32_t mediafifo;
	uint32_t mediafifolen;
	uint32_t filesz;
	uint32_t currchunk;
	uint32_t bytesread;
	uint32_t cmdrd;
	uint32_t cmdwr;

	FIL file;
	UINT read;

	 //FRESULT fres = f_open(&file, "0:/costa.avi", FA_READ);
	FRESULT fres = f_open(&file, "0:/4.jpg", FA_READ);


	if (fres != FR_OK) {
		Gpu_CoCmd_Dlstart(phost);
		App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(255, 255, 255));
		App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		App_WrCoCmd_Buffer(phost, COLOR_RGB(250,0,0));
		Gpu_CoCmd_Text(phost, (int16_t)(aviw / 2), (int16_t)(avih / 2), 29, OPT_CENTER, "Unable to open file.");
		App_WrCoCmd_Buffer(phost, END());
		App_WrCoCmd_Buffer(phost, DISPLAY());
		Gpu_CoCmd_Swap(phost);
		App_Flush_Co_Buffer(phost);
		Gpu_Hal_WaitCmdfifo_empty(phost);

		return;
	}
	else {
		/* start video playback, load the data into media fifo */


		mediafifo = aviw * avih * 2L + 32 * 1024L; //starting address of the media fifo
		mediafifolen = RAM_G_SIZE - mediafifo; //how much memory will be allocated for the video playback fifo


		Gpu_CoCmd_MediaFifo(phost, mediafifo, mediafifolen); //address of the media fifo buffer
		BSP_Print("Mediafifo: Start address and length %d %d\n", mediafifo, mediafifolen);

		Fifo_Init(&stFifo, mediafifo, mediafifolen, REG_MEDIAFIFO_READ, REG_MEDIAFIFO_WRITE); //initialize application media fifo structure


		filesz = file.obj.objsize;

		Gpu_Hal_Wr8(phost, REG_VOL_PB, 255);
		Gpu_Hal_Wr32(phost, REG_PLAY_CONTROL, 1);

		Gpu_CoCmd_Dlstart(phost);
		App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 0, 0));

		App_Flush_Co_Buffer(phost);
		Gpu_Hal_WaitCmdfifo_empty(phost);



//		App_WrCoCmd_Buffer(phost, CMD_PLAYVIDEO);
//		App_WrCoCmd_Buffer(phost, OPT_MEDIAFIFO|OPT_NOTEAR|OPT_SOUND|OPT_FULLSCREEN);
		Gpu_CoCmd_LoadImage(phost, 0, OPT_MEDIAFIFO |  OPT_FULLSCREEN);


		/* fill the complete fifo buffer before entering into steady state */
		currchunk = mediafifolen - SCRATCH_BUFF_SZ;
		currchunk = ALIGN_TWO_POWER_N(currchunk, SCRATCH_BUFF_SZ);

		while (currchunk > 0) {
			uint32_t tempchunk = currchunk;
			if (tempchunk > SCRATCH_BUFF_SZ) {
				tempchunk = SCRATCH_BUFF_SZ;
			}
			else {
				tempchunk = currchunk;
			}

			f_read( &file, g_scratch, tempchunk, &read );

			Fifo_WriteWait(phost, &stFifo, g_scratch, tempchunk);
			currchunk -= tempchunk;
		}
		cmdrd = Gpu_Hal_Rd16(phost, REG_CMD_READ);
		cmdwr = Gpu_Hal_Rd16(phost, REG_CMD_WRITE);

		filesz -= stFifo.fifo_wp;
		while ((cmdrd != cmdwr) || (filesz > 0))  //loop till end of the file
		{
			if (filesz <= 0) {
				cmdrd = Gpu_Hal_Rd16(phost, REG_CMD_READ);
				cmdwr = Gpu_Hal_Rd16(phost, REG_CMD_WRITE);
				continue;
			}

			if (filesz > SCRATCH_BUFF_SZ) {
				currchunk = SCRATCH_BUFF_SZ;
			}
			else {
				currchunk = filesz;
			}


			f_read( &file, g_scratch, currchunk, &bytesread );


			Fifo_WriteWait(phost, &stFifo, g_scratch, bytesread); //download the whole chunk into ring buffer - blocking call

			filesz -= currchunk;

			cmdrd = Gpu_Hal_Rd16(phost, REG_CMD_READ);
			cmdwr = Gpu_Hal_Rd16(phost, REG_CMD_WRITE);

		}

		f_close(&file);


	}
}




void Load_ImageASTC(Gpu_Hal_Context_t *phost, uint32_t adr, uint16_t fmt)
{
	uint16_t iw = phost->display_config.DispWidth;
	uint16_t ih = phost->display_config.DispHeight;

	Gpu_CoCmd_Dlstart(phost);
	//App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
	//App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

	//Gpu_CoCmd_FlashSource(phost, adr);
	Gpu_CoCmd_SetBitmap(phost, (0x800000 | adr/ 32 ), fmt, iw, ih);

	//Gpu_CoCmd_SetBitmap(phost, RAM_G,RGB565 , iw, ih);
	//Start drawing bitmap
	App_WrCoCmd_Buffer(phost, BEGIN(BITMAPS));
	App_WrCoCmd_Buffer(phost, VERTEX2F(0, 0));
	App_WrCoCmd_Buffer(phost, END());

	App_WrCoCmd_Buffer(phost, RESTORE_CONTEXT());
	App_WrCoCmd_Buffer(phost, DISPLAY());
	Gpu_CoCmd_Swap(phost);
	App_Flush_Co_Buffer(phost);
	Gpu_Hal_WaitCmdfifo_empty(phost);
}


void playMovie(uint32_t adr)
{
		Gpu_CoCmd_FlashHelper_SwitchFullMode(&host);
		//Gpu_Hal_Wr8(phost, REG_PLAY_CONTROL, 1);
		Gpu_CoCmd_Dlstart(phost);
		App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));
		App_WrCoCmd_Buffer(phost, COLOR_RGB(255, 255, 255));

		Gpu_CoCmd_FlashSource(phost, adr);
		App_WrCoCmd_Buffer(phost, CMD_PLAYVIDEO);
		App_WrCoCmd_Buffer(phost, OPT_FLASH|OPT_FULLSCREEN);
		App_Flush_Co_Buffer(phost);
		Gpu_CoCmd_Nop(phost);
		App_Flush_Co_Buffer(phost);
		Gpu_Hal_Sleep(10*1000);
		Gpu_Print_Error(phost);
		Gpu_Hal_Wr8(phost, REG_PLAY_CONTROL, -1);


}

#define ANIM_ADDR     (4096+4*1024*1024) // address of "{output_file}.anim.object" from *.map after generating Flash
#define FRAME_COUNT   (8)

static int system_clock = -1;

/* require to sleep one second to get system clock */
double Get_Sys_Clock(Gpu_Hal_Context_t *phost)
{
	Gpu_Hal_WaitCmdfifo_empty(phost);

	uint32_t c0 = Gpu_Hal_Rd32(phost, REG_CLOCK);
	uint32_t c1 = Gpu_Hal_Rd32(phost, REG_CLOCK);
	uint32_t overhead = c1 - c0;

	c0 = Gpu_Hal_Rd32(phost, REG_CLOCK);
	Gpu_Hal_Sleep(1000);
	c1 = Gpu_Hal_Rd32(phost, REG_CLOCK);

	return ((c1 - c0) - overhead) / 1000.0; //in KHz
}

/* return the time spending to draw a frame in miliseconds */
double Get_Frame_Time(Gpu_Hal_Context_t *phost)
{
    if (system_clock == -1)
        system_clock = Get_Sys_Clock(phost);

	Gpu_CoCmd_Sync(phost);
	Gpu_CoCmd_Memcpy(phost, 0, REG_CLOCK, 4);
	Gpu_CoCmd_Sync(phost);
	Gpu_CoCmd_Memcpy(phost, 4, REG_CLOCK, 4);

	uint32_t ft = Gpu_Hal_Rd32(phost, 4);
	ft -= Gpu_Hal_Rd32(phost, 0);

	return (ft / system_clock);
}

void Load_Animation(Gpu_Hal_Context_t *phost, uint8_t frame_rate)
{
    double duration = 1000.0 / frame_rate;
	double remain = 0;
    double frame_time = 0;

    // Switch Flash to FULL Mode
    Gpu_CoCmd_FlashHelper_SwitchFullMode(phost);
    //Gpu_CoCmd_FlashFast(phost, 0);

	for (int frame = 0; frame < FRAME_COUNT; frame++)
	{
		remain = duration;
		do
		{
			Gpu_CoCmd_Dlstart(phost);
			App_WrCoCmd_Buffer(phost, CLEAR_COLOR_RGB(0, 0, 0));
			App_WrCoCmd_Buffer(phost, CLEAR(1, 1, 1));

			Gpu_CoCmd_AnimFrame(phost, 0, 0, ANIM_ADDR, frame);
			App_WrCoCmd_Buffer(phost, DISPLAY());
			Gpu_CoCmd_Swap(phost);
			App_Flush_Co_Buffer(phost);
			Gpu_Hal_WaitCmdfifo_empty(phost);

			frame_time = Get_Frame_Time(phost);
			remain -= frame_time;

			if (0 < remain && remain < frame_time)
			{
				Gpu_Hal_Sleep(remain);
				remain = 0;
			}
		} while (remain > 0);
	}
}

/// -------------------------------------------------------------- End of file
