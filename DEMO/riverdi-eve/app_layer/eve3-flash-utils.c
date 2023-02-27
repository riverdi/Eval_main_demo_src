#include "eve3-flash-utils.h"
#include "Gpu.h"
#include "CoPro_Cmds.h"
#include "ff.h"
/*
 *  FlashErase()
 */
bool_t
FlashErase (void)
{

  /* erase the flash */
  uint8_t status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

  if (status == FLASH_STATUS_DETACHED)
    {
	  BSP_Print(" - Flash is not attached\n");
      Gpu_CoCmd_FlashAttach(phost);
      App_Flush_Co_Buffer(phost);
      Gpu_Hal_WaitCmdfifo_empty(phost);

      status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
      if (FLASH_STATUS_BASIC != status)
        {
          BSP_Print(" - Flash is not able to attach\n");
          return FALSE;
        }
    }

  uint32_t result=0;
  Gpu_CoCmd_FlashFast(phost, result);
  status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

  Gpu_CoCmd_FlashErase(phost);
  BSP_Print("Gpu_CoCmd_FlashErase\r\n");
  App_Flush_Co_Buffer(phost);
  BSP_Print("App_Flush_Co_Buffer\n\r");
  Gpu_Hal_WaitCmdfifo_empty(phost);
  BSP_Print("Gpu_Hal_WaitCmdfifo_empty done \n\r");
  /* check first 4KB */
  uint8_t *p_flashbuf = (uint8_t *)calloc(4 * 1024, sizeof(uint8_t));

//  Gpu_CoCmd_FlashHelper_Read(phost, 0, 0, 4 * 1024, p_flashbuf);
//  for (int i = 0; i < 4 * 1024; i++)
//    {
//      if (*(p_flashbuf++) != 0xFF)
//        {
//          return FALSE;
//        }
//    }

  return TRUE;
}


/*
 * Utils_Write_File_To_Flash()
 */
bool_t
Utils_Write_File_To_Flash (uchar8_t  *fileName,
                           uint32_t   addr,
                           uint32_t   tmp_ram_addr)
{
	FIL file;
	UINT read;

	uint32_t fileLen, ret = 0;
	uint8_t pBuff[(1024 * 32)];

	FRESULT fres = f_open(&file, fileName, FA_READ);
  if (fres == FR_OK)
    {


      ret = fileLen= f_size(&file);


      while (fileLen > 0)
        {
          int32_t ramlen = 0;
          tmp_ram_addr = 0;

          while ((ramlen < (1024 * 1024)) && (fileLen > 0))
            {
              uint32_t blocklen = fileLen > (1024 * 32) ? (1024 * 32) : fileLen;



              f_read(&file, pBuff, blocklen, &read);
              fileLen -= blocklen;

              Gpu_Hal_WrMem(phost, tmp_ram_addr, pBuff, blocklen);

              tmp_ram_addr += blocklen;
              ramlen += blocklen;
            }

          ramlen = (ramlen + 4095) & (~4095);//to ensure 4KB alignment

          Gpu_CoCmd_FlashHelper_Update(phost, addr, 0, ramlen);
          addr += ramlen;
          BSP_Print("addr %d %d\r\n", addr, ramlen);
        }

      f_close( &file );
    }
  else
    {
      BSP_Print(" - Unable to open file: %s\n\r", fileName);
      return FALSE;
    }

  return TRUE;
}


/*
 * FlashWriteFirst()
 */
bool_t
FlashWriteFirst (const char  *pblob,
                 uint32_t     gramaddr)
{
  uint8_t status;

  {
    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

    if (status == FLASH_STATUS_DETACHED)
      {
        Gpu_CoCmd_FlashAttach(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);

        status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
        if (FLASH_STATUS_BASIC != status)
          {
            BSP_Print(" - Flash is not able to attach\n\r");
            return FALSE;
          }
       }

    /* write the blob image, location 0 is utilized here */
    if (Utils_Write_File_To_Flash((uchar8_t*)pblob, 0, gramaddr) != TRUE)
      return FALSE;

    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (status == FLASH_STATUS_DETACHED)
      {
        Gpu_CoCmd_FlashAttach(phost);
        App_Flush_Co_Buffer(phost);
        Gpu_Hal_WaitCmdfifo_empty(phost);

        status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
        if (FLASH_STATUS_BASIC != status)
          {
            BSP_Print(" - Flash is not able to attach\n\r");
            return FALSE;
          }
      }

    /* try detaching and attaching the flash followed by fast mode */
    Gpu_CoCmd_FlashDetach(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (FLASH_STATUS_DETACHED != status)
      {
        BSP_Print(" - Flash is not able to detatch\n\r");
        return FALSE;
      }

    Gpu_CoCmd_FlashAttach(phost);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);

    if (FLASH_STATUS_BASIC != status)
      {
        BSP_Print(" - Flash is not able to attach\n\r");
        return FALSE;
      }

    Gpu_CoCmd_FlashFast(phost, 0);
    App_Flush_Co_Buffer(phost);
    Gpu_Hal_WaitCmdfifo_empty(phost);

    status = Gpu_Hal_Rd8(phost, REG_FLASH_STATUS);
    if (FLASH_STATUS_FULL != status)
      {
        BSP_Print(" - Flash is not able to get into full mode\n\r");
        return FALSE;
      }

  }

  return TRUE;
}


/*
 * FlashVerify()
 */
bool_t
FlashVerify (const char  *pfile,
             uint8_t      gramaddr,
             uint32_t     addr,
             uint32_t     size)
{
  bool_t ret = TRUE;

  if ((gramaddr % 4) != 0)
    {
      BSP_Print(" - Destination address in main memory (gramaddr) must be 4-byte aligned");
      return FALSE;
    }

  if ((addr % 64) != 0)
    {
      BSP_Print(" - Source address in flash memory (flashaddr) must be 64-byte aligned");
      return FALSE;
    }

  if ((size % 4) != 0)
    {
      BSP_Print(" - Number of bytes to read must be multiple of 4 must be 4-byte aligned");
      return FALSE;
    }

  FIL file;
  UINT read;
  FRESULT fres = f_open(&file, pfile, FA_READ);

  uint32_t file_len = f_size(&file);

  if (size > file_len)
    {
      BSP_Print(" - Size > File Size (%d)", file_len);
      f_close( &file );
      return FALSE;
    }

  file_len = 16 * 1024 * 1024;
  if (size > file_len)
    {
      BSP_Print(" - Size > Flash Size (%d)", file_len);
      f_close( &file );
      return FALSE;
    }

  #define BLOCK (32 * 1024)

//  uint8_t *p_filebuf = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));
//  uint8_t *p_flashbuf = (uint8_t *)calloc(BLOCK, sizeof(uint8_t));


  uint8_t p_filebuf[BLOCK];
  uint8_t p_flashbuf[BLOCK];

  while (size > 0)
    {
      uint32_t blocklen = size > (BLOCK) ? (BLOCK) : size;

      Gpu_CoCmd_FlashHelper_Read(phost, gramaddr, addr, blocklen, p_flashbuf);


      f_read(&file, p_filebuf, blocklen, &read);

      if (memcmp(p_filebuf, p_flashbuf, blocklen) != 0)
        {
          ret = FALSE;
          break;
        }

      size -= blocklen;
      addr += blocklen;
    }
  f_close( &file );

//  free(p_filebuf);
//  free(p_flashbuf);

  return ret;
}

void writeBlob(const char * fileName)
{
	int ret = 0;
	BSP_Print("Writing data from blob.bin file to Flash... (please wait)\r\n");
	  if (!FlashWriteFirst(fileName, RAM_G))
	    {
		  BSP_Print(" !!! ERROR !!!\n\r");
	      ret = -1;

	    }

	  /* verify */

	  BSP_Print("Verifying... (please wait)\r\n");
	  if (!FlashVerify(fileName, RAM_G, 0, 4096))
	    {
		  BSP_Print(" !!! ERROR !!!\n\r");
	      ret = -1;

	    }
}

void eraseFlash(void)
{
	int ret = 0;

	BSP_Print("Erasing Flash... (please wait)\r\n");

	  if (!FlashErase())
	    {
		  BSP_Print(" !!! ERROR !!!\r\n");
	      ret = -1;

	    }

	  BSP_Print("Erasing Flash DONE\r\n");
}



