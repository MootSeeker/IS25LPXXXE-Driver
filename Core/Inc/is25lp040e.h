/**
 * @file    is25lp040e.h
 * @brief   Header file for IS25LP040E SPI Flash memory driver.
 *          Provides function prototypes and type definitions.
 * @author  MootSeeker
 * @date    2025-11-17
 * 
 * @copyright Copyright (c) 2025 MootSeeker
 */

#ifndef INC_IS25LP040E_H_
#define INC_IS25LP040E_H_

/**
 * @include necessary standard libraries
 */
#include "stdint.h"
#include "stdbool.h"
#include "stm32g0xx_hal.h"

/**
 * @define Device-specific constants
 * @brief Memory sizes and IDs for IS25LP040E
 */
#define IS25LP_PAGE_SIZE            256      // 256 Bytes
#define IS25LP_SECTOR_SIZE          4096     // 4KB
#define IS25LP_BLOCK_32K_SIZE       32768    // 32KB
#define IS25LP_BLOCK_64K_SIZE       65536    // 64KB
#define IS25LP_CHIP_SIZE            524288   // 512KB (4Mbit)
#define IS25LP_TOTAL_SECTORS        128      // 512KB / 4KB
#define IS25LP_TOTAL_PAGES          2048     // 512KB / 256B

/**
 * @brief Manufacturer & Device IDs
 */
#define IS25LP_MANUFACTURER_ID      0x9D     // ISSI
#define IS25LP_DEVICE_ID            0x13     // 4Mbit (512KB)
#define IS25LP_JEDEC_ID             0x6013   // Memory Type + Capacity

/**
 * @enum eIS25LP_Status_t
 * @brief Status codes for IS25LP operations
 */
typedef enum
{
	IS25LP_ERROR = false,
	IS25LP_OK = true
} eIS25LP_Status_t;

/**
 * @struct sIS25LP_GPIO_t
 * @brief GPIO pin configuration
 */
typedef struct
{
    GPIO_TypeDef *port;         // GPIO port (e.g., GPIOA)
    uint16_t pin;               // GPIO pin (e.g., GPIO_PIN_4)
} sIS25LP_GPIO_t;

/**
 * @struct sIS25LP_Handle_t
 * @brief Handle structure for IS25LP Flash instance
 * 
 * @details This structure contains all hardware-specific configuration
 *          needed to communicate with the Flash chip. It allows multiple
 *          Flash instances on different SPI buses with different GPIO pins.
 */
typedef struct
{
    SPI_HandleTypeDef *spi_handle;  // Pointer to SPI handle
    sIS25LP_GPIO_t cs_gpio;         // Chip Select (CS) GPIO configuration
    sIS25LP_GPIO_t wp_gpio;         // Write Protect (WP) GPIO configuration
    bool initialized;               // Initialization status flag
} sIS25LP_Handle_t;

/**
 * @struct sIS25LP_DeviceInfo_t
 * @brief Structure to hold IS25LP device information
 */
typedef struct
{
    uint8_t manufacturer_id;    // 0x9D für ISSI
    uint8_t memory_type;        // 0x60
    uint8_t capacity;           // 0x13 für 4Mbit
    uint8_t unique_id[8];       // 64-bit Unique ID
} sIS25LP_DeviceInfo_t;

/**
 * @brief Function prototypes for IS25LP operations
 */

/**
 * @brief  Initialize the IS25LP040E Flash memory
 * @param  handle: Pointer to IS25LP handle structure
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details Initializes the Flash memory by:
 *          - Storing hardware configuration in handle
 *          - Setting CS to idle state (high)
 *          - Reading and verifying JEDEC ID
 *          - Checking manufacturer ID (0x9D for ISSI)
 *          - Checking device capacity (0x13 for 4Mbit)
 *          - Setting initialized flag
 */
eIS25LP_Status_t IS25LP_Init(sIS25LP_Handle_t *handle);

/**
 * @brief  Read JEDEC ID from Flash memory
 * @param  handle: Pointer to IS25LP handle structure
 * @param  manufacturer: Pointer to store Manufacturer ID (0x9D for ISSI)
 * @param  memory_type: Pointer to store Memory Type (0x60)
 * @param  capacity: Pointer to store Capacity (0x13 for 4Mbit/512KB)
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details Sends 0x9F command and reads 3-byte response:
 *          Response format: [Manufacturer][Memory Type][Capacity]
 *          Example: 0x9D 0x60 0x13
 */
eIS25LP_Status_t IS25LP_ReadJedecID(sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *memory_type, uint8_t *capacity);

/**
 * @brief  Read Manufacturer and Device ID
 * @param  handle: Pointer to IS25LP handle structure
 * @param  manufacturer: Pointer to store Manufacturer ID
 * @param  device_id: Pointer to store Device ID
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details Sends 0x90 command with 3 address bytes (0x00 0x00 0x00)
 *          followed by 2 dummy bytes to read the IDs.
 */
eIS25LP_Status_t IS25LP_ReadDeviceID(sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *device_id);

/**
 * @brief  Read 64-bit Unique ID from Flash
 * @param  handle: Pointer to IS25LP handle structure
 * @param  unique_id: Pointer to 8-byte buffer for Unique ID
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details Sends 0x4B command followed by 4 dummy bytes,
 *          then reads 8 bytes of unique identifier.
 *          Each chip has a factory-programmed unique ID.
 */
eIS25LP_Status_t IS25LP_ReadUniqueID(sIS25LP_Handle_t *handle, uint8_t *unique_id);

/**
 * @brief  Get stored device information
 * @param  handle: Pointer to IS25LP handle structure
 * @param  info: Pointer to structure to receive device info
 * @retval IS25LP_OK on success, IS25LP_ERROR if not initialized
 * 
 * @details Returns device information including:
 *          - Manufacturer ID
 *          - Memory type
 *          - Capacity
 *          - 64-bit Unique ID
 */
eIS25LP_Status_t IS25LP_GetDeviceInfo(sIS25LP_Handle_t *handle, sIS25LP_DeviceInfo_t *info);

/**
 * @brief  Read data from Flash memory
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Start address (0x000000 - 0x07FFFF)
 * @param  buffer: Pointer to buffer for read data
 * @param  length: Number of bytes to read
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Can read any number of bytes
 *          - Can cross page and sector boundaries
 *          - Validates address range and buffer pointer
 *          - Waits for Flash ready before reading
 *          - Uses standard Read command (0x03)
 */
eIS25LP_Status_t IS25LP_Read(sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length);

/**
 * @brief  Fast read data from Flash memory
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Start address (0x000000 - 0x07FFFF)
 * @param  buffer: Pointer to buffer for read data
 * @param  length: Number of bytes to read
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Can read any number of bytes at higher speed
 *          - Can cross page and sector boundaries
 *          - Validates address range and buffer pointer
 *          - Requires 1 dummy byte after address
 *          - Uses Fast Read command (0x0B)
 *          - Supports higher SPI clock frequencies than standard read
 */
eIS25LP_Status_t IS25LP_FastRead(sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length);

/**
 * @brief  Write one page to Flash memory
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Start address within page
 * @param  buffer: Pointer to data to write
 * @param  length: Number of bytes to write (1-256)
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Maximum 256 bytes per write
 *          - Must not cross page boundary (256-byte aligned)
 *          - Sector must be erased (0xFF) before writing
 *          - Automatically enables write and waits for completion
 *          - Typical write time: ~3ms
 * @warning Writing across page boundary will cause wrap-around
 */
eIS25LP_Status_t IS25LP_WritePage(sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint16_t length);

/**
 * @brief  Write data to Flash memory (multi-page)
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Start address
 * @param  buffer: Pointer to data to write
 * @param  length: Number of bytes to write
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Automatically handles page boundaries
 *          - Can write any number of bytes
 *          - Splits write into multiple page operations
 *          - Sector(s) must be erased before writing
 *          - More convenient than WritePage for large data
 */
eIS25LP_Status_t IS25LP_Write(sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint32_t length);

/**
 * @brief  Erase a 4KB sector
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Any address within the sector
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Erases 4KB (4096 bytes)
 *          - Address automatically aligned to sector boundary
 *          - Sets all bytes in sector to 0xFF
 *          - Typical erase time: ~100ms
 *          - Must erase before writing new data
 */
eIS25LP_Status_t IS25LP_EraseSector(sIS25LP_Handle_t *handle, uint32_t address);

/**
 * @brief  Erase a 32KB block
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Any address within the block
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Erases 32KB (32768 bytes)
 *          - Address automatically aligned to 32KB boundary
 *          - Sets all bytes in block to 0xFF
 *          - Typical erase time: ~200ms
 *          - Faster than erasing 8 sectors individually
 */
eIS25LP_Status_t IS25LP_EraseBlock32K(sIS25LP_Handle_t *handle, uint32_t address);

/**
 * @brief  Erase a 64KB block
 * @param  handle: Pointer to IS25LP handle structure
 * @param  address: Any address within the block
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Erases 64KB (65536 bytes)
 *          - Address automatically aligned to 64KB boundary
 *          - Sets all bytes in block to 0xFF
 *          - Typical erase time: ~400ms
 *          - Fastest option for erasing large regions
 */
eIS25LP_Status_t IS25LP_EraseBlock64K(sIS25LP_Handle_t *handle, uint32_t address);

/**
 * @brief  Erase entire chip
 * @param  handle: Pointer to IS25LP handle structure
 * @retval IS25LP_OK on success, IS25LP_ERROR on failure
 * 
 * @details - Erases all 512KB of Flash memory
 *          - Sets all bytes to 0xFF
 *          - Typical erase time: 3-10 seconds
 *          - Use with caution - cannot be undone!
 * @warning This operation erases the entire chip and takes several seconds
 */
eIS25LP_Status_t IS25LP_EraseChip(sIS25LP_Handle_t *handle);

#endif /* INC_IS25LP040E_H_ */
