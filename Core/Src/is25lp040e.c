/*
 * is25lp040e.c
 *
 *  Created on: Nov 17, 2025
 *      Author: kevin
 */


#include "is25lp040e.h"
#include "main.h"
#include "spi.h"

#include <string.h>

// IS25LP040E Kommandos (Standard SPI Flash Commands)
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

// Status Register Bits
#define STATUS_BUSY             0x01    // Write In Progress (WIP)
#define STATUS_WEL              0x02    // Write Enable Latch

// Timeouts (in milliseconds)
#define TIMEOUT_SPI             5       // SPI Communication
#define TIMEOUT_PAGE_PROGRAM    10      // Page Program (~3ms typ)
#define TIMEOUT_SECTOR_ERASE    200     // Sector Erase (~100ms typ)
#define TIMEOUT_BLOCK_ERASE_32K 500     // Block Erase 32KB
#define TIMEOUT_BLOCK_ERASE_64K 1000    // Block Erase 64KB
#define TIMEOUT_CHIP_ERASE      10000   // Chip Erase (~3s typ)

#define DUMMY_BYTE              0xFF

// Globale Device Info
static sIS25LP_DeviceInfo_t device_info = {0};

static void SPI_CS_Low( void )
{
	HAL_GPIO_WritePin( SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET );
}

static void SPI_CS_High( void )
{
	HAL_GPIO_WritePin( SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET );
}

/**
 * @brief  Write Enable Kommando senden
 */
static eIS25LP_Status_t IS25LP_WriteEnable(void)
{
    uint8_t cmd = CMD_WRITE_ENABLE;

    SPI_CS_Low();
    uint8_t status = HAL_SPI_Transmit( &hspi1, &cmd, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High();

    return (HAL_OK == status ) ? IS25LP_OK : IS25LP_ERROR;
}

/**
 * @brief  Status Register lesen
 */
static uint8_t IS25LP_ReadStatusRegister(void)
{
    uint8_t cmd[2] = { CMD_READ_STATUS_REG, DUMMY_BYTE };
    uint8_t response[ 2 ];

    SPI_CS_Low();
    HAL_SPI_TransmitReceive( &hspi1, cmd, response, sizeof(cmd), TIMEOUT_SPI );
    SPI_CS_High();

    return response[ 1 ];
}

/**
 * @brief  Warte bis Flash bereit ist (WIP Bit = 0)
 */
static eIS25LP_Status_t IS25LP_WaitForReady(uint32_t timeout_ms)
{
    uint32_t tickstart = HAL_GetTick( );

    while(( IS25LP_ReadStatusRegister( ) & STATUS_BUSY ) != 0 )
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
 * @brief  Flash initialisieren
 * @return IS25LP_OK bei Erfolg, sonst Fehlercode
 */
eIS25LP_Status_t IS25LP_Init(void)
{
    // CS auf High setzen (Idle State)
    SPI_CS_High( );
    HAL_Delay(10);

    // Write Protection deaktivieren (falls benötigt)
//    SPI_WP_Disable( );

    // JEDEC ID lesen zur Verifikation
    uint8_t manufacturer, memory_type, capacity;

    if( IS25LP_OK != IS25LP_ReadJedecID( &manufacturer, &memory_type, &capacity ))
    {
        return IS25LP_ERROR;
    }

    // Prüfe ob es der richtige Chip ist
    if( IS25LP_MANUFACTURER_ID != manufacturer )
    {
        return IS25LP_ERROR;  // Falscher Hersteller
    }

    if( IS25LP_DEVICE_ID != capacity )
    {
        return IS25LP_ERROR;  // Falsche Kapazität
    }

    // Device Info speichern
    device_info.manufacturer_id = manufacturer;
    device_info.memory_type = memory_type;
    device_info.capacity = capacity;
    device_info.initialized = true;

    // Unique ID lesen
    IS25LP_ReadUniqueID( device_info.unique_id );

    return IS25LP_OK;
}

/**
 * @brief  JEDEC ID lesen (0x9F Kommando)
 * @param  manufacturer: Pointer für Manufacturer ID (0x9D)
 * @param  memory_type: Pointer für Memory Type (0x60)
 * @param  capacity: Pointer für Capacity (0x13 = 4Mbit)
 * @return IS25LP_OK bei Erfolg
 *
 * @note   Rückgabe: [0x9D][0x60][0x13]
 */
eIS25LP_Status_t IS25LP_ReadJedecID( uint8_t *manufacturer, uint8_t *memory_type, uint8_t *capacity )
{
    uint8_t cmd[4] = { CMD_READ_JEDEC_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE };
    uint8_t response[ 4 ];

    SPI_CS_Low( );
    if( HAL_OK != HAL_SPI_TransmitReceive( &hspi1, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( );
        return IS25LP_ERROR;
    }
    SPI_CS_High( );

    *manufacturer = response[ 1 ];  // 0x9D (ISSI)
    *memory_type = response[ 2 ];   // 0x60
    *capacity = response[ 3 ];      // 0x13 (4Mbit)

    return IS25LP_OK;
}

/**
 * @brief  Device ID lesen (0x90 Kommando - wie in der Referenz)
 * @param  manufacturer: Pointer für Manufacturer ID
 * @param  device_id: Pointer für Device ID
 * @return IS25LP_OK bei Erfolg
 *
 * @note   Kommando: [0x90][0x00][0x00][0x00][Dummy][Dummy]
 *         Response: [--][--][--][--][Manufacturer][Device]
 */
eIS25LP_Status_t IS25LP_ReadDeviceID( uint8_t *manufacturer, uint8_t *device_id )
{
    uint8_t cmd[6] = { CMD_READ_DEVICE_ID, 0x00, 0x00, 0x00, DUMMY_BYTE, DUMMY_BYTE };
    uint8_t response[ 6 ];

    SPI_CS_Low( );
    if( HAL_OK != HAL_SPI_TransmitReceive( &hspi1, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( );
        return IS25LP_ERROR;
    }
    SPI_CS_High( );

    *manufacturer = response[ 4 ];  // Manufacturer ID
    *device_id = response[ 5 ];     // Device ID

    return IS25LP_OK;
}

/**
 * @brief  64-bit Unique ID lesen (0x4B Kommando)
 * @param  unique_id: Pointer für 8-Byte Unique ID
 * @return IS25LP_OK bei Erfolg
 *
 * @note   Kommando: [0x4B][4 Dummy Bytes][8 Unique ID Bytes]
 */
eIS25LP_Status_t IS25LP_ReadUniqueID( uint8_t *unique_id )
{
    uint8_t cmd[ 13 ] = { CMD_READ_UNIQUE_ID };
    uint8_t response[ 13 ];

    // 4 Dummy Bytes + 8 Unique ID Bytes
    for (int i = 1; i < 13; i++) {
        cmd[i] = DUMMY_BYTE;
    }

    SPI_CS_Low( );
    if( HAL_OK != HAL_SPI_TransmitReceive( &hspi1, cmd, response, sizeof(cmd), TIMEOUT_SPI ))
    {
        SPI_CS_High( );
        return IS25LP_ERROR;
    }
    SPI_CS_High( );

    // Unique ID startet bei Byte 5 (nach Kommando + 4 Dummy Bytes)
    memcpy( unique_id, &response[ 5 ], 8 );

    return IS25LP_OK;
}

/**
 * @brief  Device Info abrufen
 * @param  info: Pointer zur IS25LP_DeviceInfo_t Struktur
 * @return IS25LP_OK bei Erfolg
 */
eIS25LP_Status_t IS25LP_GetDeviceInfo( sIS25LP_DeviceInfo_t *info)
{
    if( !device_info.initialized )
    {
        return IS25LP_ERROR;
    }

    memcpy( info, &device_info, sizeof( sIS25LP_DeviceInfo_t ));
    return IS25LP_OK;
}
