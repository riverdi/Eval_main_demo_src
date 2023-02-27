/*
 * bsp_flash.c
 *
 *  Created on: Aug 26, 2020
 *      Author: milos
 */

#include "bsp_print.h"
#include "quadspi.h"
#include <stdlib.h>
#include <string.h>

static void
_Write_enable( void );
static unsigned char
_Flash_Busy( void );
static void
_Quad_Enable( void );

void
BSP_Flash_Get_Manufacturer_ID( void )
{
    QSPI_CommandTypeDef cmd;
    unsigned char data[2];

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x90;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = 0x00000000UL;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.DummyCycles = 0;
    cmd.NbData = 2;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Receive( &hqspi, data, 1000 );

    BSP_Print( "\r\n\tManufacturer ID: %xh %xh", data[0], data[1] );
}

void
BSP_Flash_Sector_Erase( uint32_t sector )
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x20;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = sector;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.NbData = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    _Quad_Enable( );
    _Write_enable( );
    HAL_QSPI_Command( &hqspi, &cmd, 1000 );

    BSP_Print( "\r\n\tSector erasing - " );
    while ( _Flash_Busy( ) )
        ;
    BSP_Print( "Done" );
}

void
BSP_Flash_Chip_Erase( void )
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x60;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = 0x00000000UL;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.NbData = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );

    BSP_Print( "\r\n\tChip erasing - " );
    while ( _Flash_Busy( ) )
        ;
    BSP_Print( "Done" );
}

void
BSP_Flash_Read_Data_Q( unsigned long address, unsigned char * data,
                       unsigned int length )
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x6B;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = ( address & 0x007FFFFFUL );
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.DummyCycles = 8;
    cmd.NbData = length;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    _Quad_Enable( );
    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Receive( &hqspi, data, 1000 );

    BSP_Print( "\r\n\tData Read (%x) -", address );
    for ( unsigned int i = 0; i < length; ++i )
    {
        BSP_Print( " %xh", data[i] );
    }
}

void
BSP_Flash_Write_Data_Q( unsigned long address, unsigned char * data,
                        unsigned int length )
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x32;
    cmd.AddressMode = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = ( address & 0x007FFFFFUL );
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_4_LINES;
    cmd.DummyCycles = 0;
    cmd.NbData = length;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    _Quad_Enable( );
    _Write_enable( );
    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Transmit( &hqspi, data, 1000 );

    BSP_Print( "\r\n\tData Write (%x) -", address );
    for ( unsigned int i = 0; i < length; ++i )
    {
        BSP_Print( " %xh", data[i] );
    }
    BSP_Print( "\r\n\t\tFlash Writing - " );
    while ( _Flash_Busy( ) )
        ;
    BSP_Print( "Done" );
}

void
BSP_Flash_SingleTest( void )
{
    char custom_string_wr[10] = "Hello";
    char custom_string_rd[10] = "";
    unsigned long address = rand( ) & 0x007FFFFF;

    BSP_Print( "\t\tFlash Test\r\n" );
    BSP_Flash_Get_Manufacturer_ID( );
    BSP_Flash_Chip_Erase( );

    BSP_Flash_Write_Data_Q( address, ( unsigned char * )custom_string_wr,
                            strlen( custom_string_wr ) );
    BSP_Flash_Read_Data_Q( address, ( unsigned char * )custom_string_rd,
                           strlen( custom_string_wr ) );

    if ( strcmp( custom_string_wr, custom_string_rd ) )
    {
        BSP_Print( "\r\n\tFlash Test - FAILED" );
    }
    else
    {
        BSP_Print( "\r\n\tFlash Test - OK" );
    }
}

static unsigned char
_Flash_Busy( void )
{
    unsigned char status_reg;
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x05;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = 0x00000000UL;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.DummyCycles = 0;
    cmd.NbData = 1;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Receive( &hqspi, &status_reg, 1000 );

    BSP_Print( "\r\n\t\tStatus Register - %xh", status_reg );
    return ( status_reg & 0x01 );
}

static void
_Quad_Enable( void )
{
    unsigned char status_reg;
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x35;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = 0x00000000UL;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_1_LINE;
    cmd.DummyCycles = 0;
    cmd.NbData = 1;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Receive( &hqspi, &status_reg, 1000 );

    cmd.Instruction = 0x31;
    status_reg |= 0x02;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    HAL_QSPI_Transmit( &hqspi, &status_reg, 1000 );
    BSP_Print( "\r\n\t\tQuad Enabled" );
}

static void
_Write_enable( void )
{
    QSPI_CommandTypeDef cmd;

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction = 0x06;
    cmd.AddressMode = QSPI_ADDRESS_NONE;
    cmd.AddressSize = QSPI_ADDRESS_24_BITS;
    cmd.Address = 0x00000000UL;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.AlternateBytes = 0;
    cmd.AlternateBytesSize = 0;
    cmd.DataMode = QSPI_DATA_NONE;
    cmd.DummyCycles = 0;
    cmd.NbData = 0;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    HAL_QSPI_Command( &hqspi, &cmd, 1000 );
    BSP_Print( "\r\n\t\tWrite Enabled" );
}
