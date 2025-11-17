/**
 * @file    is25lp040e.c
 * @brief   Source file for IS25LP040E SPI Flash memory driver.
 *          Implements functions for SPI Flash operations.
 * @author  MootSeeker
 * @date    2025-11-17
 * 
 * @copyright Copyright (c) 2025 MootSeeker
 */

/**
 * @include necessary headers
 */
#include "is25lp040e.h"
#include "main.h"
#include "spi.h"

#include <string.h>

/**
 * @brief   IS25LP040E Commands (Standard SPI Flash Commands)
 */
#define CMD_WRITE_ENABLE        0x06    // Write Enable
#define CMD_WRITE_DISABLE       0x04    // Write Disable
#define CMD_READ_STATUS_REG     0x05    // Read Status Register
#define CMD_WRITE_STATUS_REG    0x01    // Write Status Register
#define CMD_READ_DATA           0x03    // Read Data
#define CMD_FAST_READ           0x0B    // Fast Read
#define CMD_PAGE_PROGRAM        0x02    // Page Program
#define CMD_SECTOR_ERASE        0x20    // Sector Erase (4KB)
#define CMD_BLOCK_ERASE_32K     0x52    // Block Erase 32KB
#define CMD_BLOCK_ERASE_64K     0xD8    // Block Erase 64KB
#define CMD_CHIP_ERASE          0xC7    // Chip Erase
#define CMD_READ_JEDEC_ID       0x9F    // Read JEDEC ID
#define CMD_READ_DEVICE_ID      0x90    // Read Manufacturer/Device ID
#define CMD_READ_UNIQUE_ID      0x4B    // Read Unique ID
#define CMD_DEEP_POWER_DOWN     0xB9    // Deep Power-Down
#define CMD_RELEASE_POWER_DOWN  0xAB    // Release from Deep Power-Down

/**
 * @brief   Status Register Bits
 */
#define STATUS_BUSY             0x01    // Write In Progress (WIP)
#define STATUS_WEL              0x02    // Write Enable Latch

/**
 * @brief   Timeouts (in milliseconds)
 */
#define TIMEOUT_SPI             5       // SPI Communication
#define TIMEOUT_PAGE_PROGRAM    10      // Page Program (~3ms typ)
#define TIMEOUT_SECTOR_ERASE    200     // Sector Erase (~100ms typ)
#define TIMEOUT_BLOCK_ERASE_32K 500     // Block Erase 32KB
#define TIMEOUT_BLOCK_ERASE_64K 1000    // Block Erase 64KB
#define TIMEOUT_CHIP_ERASE      10000   // Chip Erase (~3s typ)

#define DUMMY_BYTE              0xFF

/**
 * @brief  Set the CS-Signal to Low
 */
static void SPI_CS_Low( sIS25LP_Handle_t *handle )
{
	HAL_GPIO_WritePin( handle->cs_gpio.port, handle->cs_gpio.pin, GPIO_PIN_RESET );
}

/**
 * @brief  Set the CS-Signal to High
 */
static void SPI_CS_High( sIS25LP_Handle_t *handle )
{
	HAL_GPIO_WritePin( handle->cs_gpio.port, handle->cs_gpio.pin, GPIO_PIN_SET );
}

/**
 * @brief  Write Enable command
 */
static eIS25LP_Status_t IS25LP_WriteEnable( sIS25LP_Handle_t *handle )
{
    uint8_t cmd = CMD_WRITE_ENABLE;

    SPI_CS_Low( handle );
    uint8_t status = HAL_SPI_Transmit( handle->spi_handle, &cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    return (HAL_OK == status ) ? IS25LP_OK : IS25LP_ERROR;
}

/**
 * @brief  Read Status Register
 */
static uint8_t IS25LP_ReadStatusRegister( sIS25LP_Handle_t *handle )
{
    uint8_t cmd[2] = { CMD_READ_STATUS_REG, DUMMY_BYTE };
    uint8_t response[ 2 ];

    SPI_CS_Low( handle );
    HAL_SPI_TransmitReceive( handle->spi_handle, cmd, response, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    return response[ 1 ];
}

/**
 * @brief  Wait until Flash is ready (WIP Bit = 0)
 */
static eIS25LP_Status_t IS25LP_WaitForReady( sIS25LP_Handle_t *handle, uint32_t timeout_ms )
{
    uint32_t tickstart = HAL_GetTick( );

    while(( IS25LP_ReadStatusRegister( handle ) & STATUS_BUSY ) != 0 )
    {
        if(( HAL_GetTick( ) - tickstart) > timeout_ms )
        {
            return IS25LP_ERROR;
        }
        HAL_Delay(1);
    }

    return IS25LP_OK;
}

/**
 * @brief  Initialize Flash
 */
eIS25LP_Status_t IS25LP_Init( sIS25LP_Handle_t *handle )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Set CS High (Idle State)
    SPI_CS_High( handle );
    HAL_Delay(10);

    // Read JEDEC ID for verification
    uint8_t manufacturer, memory_type, capacity;

    if( IS25LP_OK != IS25LP_ReadJedecID( handle, &manufacturer, &memory_type, &capacity ))
    {
        return IS25LP_ERROR;
    }

    // Check if it is the correct chip
    if( IS25LP_MANUFACTURER_ID != manufacturer )
    {
        return IS25LP_ERROR;  // Wrong manufacturer
    }

    if( IS25LP_DEVICE_ID != capacity )
    {
        return IS25LP_ERROR;  // Wrong capacity
    }

    // Mark handle as initialized
    handle->initialized = true;

    return IS25LP_OK;
}

/**
 * @brief  Read JEDEC ID (0x9F command)
 */
eIS25LP_Status_t IS25LP_ReadJedecID( sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *memory_type, uint8_t *capacity )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    uint8_t cmd[4] = { CMD_READ_JEDEC_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE };
    uint8_t response[ 4 ];

    SPI_CS_Low( handle );
    if( HAL_OK != HAL_SPI_TransmitReceive( handle->spi_handle, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }
    SPI_CS_High( handle );

    *manufacturer = response[ 1 ];  // 0x9D (ISSI)
    *memory_type = response[ 2 ];   // 0x60
    *capacity = response[ 3 ];      // 0x13 (4Mbit)

    return IS25LP_OK;
}

/**
 * @brief  Read Device ID (0x90 command)
 */
eIS25LP_Status_t IS25LP_ReadDeviceID( sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *device_id )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    uint8_t cmd[6] = { CMD_READ_DEVICE_ID, 0x00, 0x00, 0x00, DUMMY_BYTE, DUMMY_BYTE };
    uint8_t response[ 6 ];

    SPI_CS_Low( handle );
    if( HAL_OK != HAL_SPI_TransmitReceive( handle->spi_handle, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }
    SPI_CS_High( handle );

    *manufacturer = response[ 4 ];  // Manufacturer ID
    *device_id = response[ 5 ];     // Device ID

    return IS25LP_OK;
}

/**
 * @brief  Read 64-bit Unique ID (0x4B command)
 */
eIS25LP_Status_t IS25LP_ReadUniqueID( sIS25LP_Handle_t *handle, uint8_t *unique_id )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    uint8_t cmd[ 13 ] = { CMD_READ_UNIQUE_ID };
    uint8_t response[ 13 ];

    // 4 Dummy Bytes + 8 Unique ID Bytes
    for (int i = 1; i < 13; i++) {
        cmd[i] = DUMMY_BYTE;
    }

    SPI_CS_Low( handle );
    if( HAL_OK != HAL_SPI_TransmitReceive( handle->spi_handle, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }
    SPI_CS_High( handle );

    // Unique ID starts at byte 5 (after command + 4 dummy bytes)
    memcpy( unique_id, &response[ 5 ], 8 );

    return IS25LP_OK;
}

/**
 * @brief  Get Device Info
 */
eIS25LP_Status_t IS25LP_GetDeviceInfo( sIS25LP_Handle_t *handle, sIS25LP_DeviceInfo_t *info)
{
    // Validate handle parameter
    if( NULL == handle || NULL == info )
    {
        return IS25LP_ERROR;
    }

    if( !handle->initialized )
    {
        return IS25LP_ERROR;
    }

    // Read device information
    if( IS25LP_OK != IS25LP_ReadJedecID( handle, &info->manufacturer_id, &info->memory_type, &info->capacity ))
    {
        return IS25LP_ERROR;
    }

    if( IS25LP_OK != IS25LP_ReadUniqueID( handle, info->unique_id ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}

/**
 * @brief  Read data from Flash memory
 */
eIS25LP_Status_t IS25LP_Read( sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Validate parameters
    if( NULL == buffer || 0 == length )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if(( address + length ) > IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_SPI ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low]
    uint8_t cmd[ 4 ] = {
        CMD_READ_DATA,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF )
    };

    SPI_CS_Low( handle );

    // Send command and address
    if( HAL_OK != HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    // Read data
    if( HAL_OK != HAL_SPI_Receive( handle->spi_handle, buffer, length, TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    SPI_CS_High( handle );

    return IS25LP_OK;
}

/**
 * @brief  Fast read data from Flash memory
 */
eIS25LP_Status_t IS25LP_FastRead( sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Validate parameters
    if( NULL == buffer || 0 == length )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if(( address + length ) > IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_SPI ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low][Dummy]
    uint8_t cmd[ 5 ] = {
        CMD_FAST_READ,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF ),
        DUMMY_BYTE
    };

    SPI_CS_Low( handle );

    // Send command, address, and dummy byte
    if( HAL_OK != HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    // Read data
    if( HAL_OK != HAL_SPI_Receive( handle->spi_handle, buffer, length, TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    SPI_CS_High( handle );

    return IS25LP_OK;
}

/**
 * @brief  Write one page to Flash memory
 */
eIS25LP_Status_t IS25LP_WritePage( sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint16_t length )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Validate parameters
    if( NULL == buffer || 0 == length || length > IS25LP_PAGE_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if( address >= IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Ensure write does not cross page boundary
    uint32_t page_offset = address % IS25LP_PAGE_SIZE;
    if(( page_offset + length ) > IS25LP_PAGE_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_PAGE_PROGRAM ))
    {
        return IS25LP_ERROR;
    }

    // Enable write operations
    if( IS25LP_OK != IS25LP_WriteEnable( handle ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low]
    uint8_t cmd[ 4 ] = {
        CMD_PAGE_PROGRAM,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF )
    };

    SPI_CS_Low( handle );

    // Send command and address
    if( HAL_OK != HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    // Write data
    if( HAL_OK != HAL_SPI_Transmit( handle->spi_handle, ( uint8_t* )buffer, length, TIMEOUT_SPI ))
    {
        SPI_CS_High( handle );
        return IS25LP_ERROR;
    }

    SPI_CS_High( handle );

    // Wait for write operation to complete
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_PAGE_PROGRAM ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}

/**
 * @brief  Write data to Flash memory (multi-page)
 */
eIS25LP_Status_t IS25LP_Write( sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint32_t length )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Validate parameters
    if( NULL == buffer || 0 == length )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if(( address + length ) > IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    uint32_t bytes_written = 0;
    uint32_t current_address = address;
    const uint8_t *current_buffer = buffer;

    while( bytes_written < length )
    {
        // Calculate bytes remaining in current page
        uint32_t page_offset = current_address % IS25LP_PAGE_SIZE;
        uint32_t bytes_to_page_end = IS25LP_PAGE_SIZE - page_offset;
        uint32_t bytes_to_write = ( length - bytes_written );

        // Write only up to page boundary
        if( bytes_to_write > bytes_to_page_end )
        {
            bytes_to_write = bytes_to_page_end;
        }

        // Write current page
        if( IS25LP_OK != IS25LP_WritePage( handle, current_address, current_buffer, bytes_to_write ))
        {
            return IS25LP_ERROR;
        }

        // Update counters and pointers
        bytes_written += bytes_to_write;
        current_address += bytes_to_write;
        current_buffer += bytes_to_write;
    }

    return IS25LP_OK;
}

/**
 * @brief  Erase a 4KB sector
 */
eIS25LP_Status_t IS25LP_EraseSector( sIS25LP_Handle_t *handle, uint32_t address )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if( address >= IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Align address to sector boundary
    address = ( address / IS25LP_SECTOR_SIZE ) * IS25LP_SECTOR_SIZE;

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_SECTOR_ERASE ))
    {
        return IS25LP_ERROR;
    }

    // Enable write operations
    if( IS25LP_OK != IS25LP_WriteEnable( handle ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low]
    uint8_t cmd[ 4 ] = {
        CMD_SECTOR_ERASE,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF )
    };

    SPI_CS_Low( handle );
    HAL_StatusTypeDef status = HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    if( HAL_OK != status )
    {
        return IS25LP_ERROR;
    }

    // Wait for erase operation to complete
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_SECTOR_ERASE ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}

/**
 * @brief  Erase a 32KB block
 */
eIS25LP_Status_t IS25LP_EraseBlock32K( sIS25LP_Handle_t *handle, uint32_t address )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if( address >= IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Align address to 32KB block boundary
    address = ( address / IS25LP_BLOCK_32K_SIZE ) * IS25LP_BLOCK_32K_SIZE;

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_BLOCK_ERASE_32K ))
    {
        return IS25LP_ERROR;
    }

    // Enable write operations
    if( IS25LP_OK != IS25LP_WriteEnable( handle ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low]
    uint8_t cmd[ 4 ] = {
        CMD_BLOCK_ERASE_32K,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF )
    };

    SPI_CS_Low( handle );
    HAL_StatusTypeDef status = HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    if( HAL_OK != status )
    {
        return IS25LP_ERROR;
    }

    // Wait for erase operation to complete
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_BLOCK_ERASE_32K ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}

/**
 * @brief  Erase a 64KB block
 */
eIS25LP_Status_t IS25LP_EraseBlock64K( sIS25LP_Handle_t *handle, uint32_t address )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Check address range
    if( address >= IS25LP_CHIP_SIZE )
    {
        return IS25LP_ERROR;
    }

    // Align address to 64KB block boundary
    address = ( address / IS25LP_BLOCK_64K_SIZE ) * IS25LP_BLOCK_64K_SIZE;

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_BLOCK_ERASE_64K ))
    {
        return IS25LP_ERROR;
    }

    // Enable write operations
    if( IS25LP_OK != IS25LP_WriteEnable( handle ))
    {
        return IS25LP_ERROR;
    }

    // Prepare command: [CMD][Address High][Address Mid][Address Low]
    uint8_t cmd[ 4 ] = {
        CMD_BLOCK_ERASE_64K,
        ( uint8_t )(( address >> 16 ) & 0xFF ),
        ( uint8_t )(( address >> 8 ) & 0xFF ),
        ( uint8_t )( address & 0xFF )
    };

    SPI_CS_Low( handle );
    HAL_StatusTypeDef status = HAL_SPI_Transmit( handle->spi_handle, cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    if( HAL_OK != status )
    {
        return IS25LP_ERROR;
    }

    // Wait for erase operation to complete
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_BLOCK_ERASE_64K ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}

/**
 * @brief  Erase entire chip
 */
eIS25LP_Status_t IS25LP_EraseChip( sIS25LP_Handle_t *handle )
{
    // Validate handle parameter
    if( NULL == handle )
    {
        return IS25LP_ERROR;
    }

    // Wait for Flash to be ready
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_CHIP_ERASE ))
    {
        return IS25LP_ERROR;
    }

    // Enable write operations
    if( IS25LP_OK != IS25LP_WriteEnable( handle ))
    {
        return IS25LP_ERROR;
    }

    uint8_t cmd = CMD_CHIP_ERASE;

    SPI_CS_Low( handle );
    HAL_StatusTypeDef status = HAL_SPI_Transmit( handle->spi_handle, &cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High( handle );

    if( HAL_OK != status )
    {
        return IS25LP_ERROR;
    }

    // Wait for erase operation to complete (this takes several seconds!)
    if( IS25LP_OK != IS25LP_WaitForReady( handle, TIMEOUT_CHIP_ERASE ))
    {
        return IS25LP_ERROR;
    }

    return IS25LP_OK;
}
