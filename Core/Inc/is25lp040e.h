/*
 * is25lp040e.h
 *
 *  Created on: Nov 17, 2025
 *      Author: kevin
 */

#ifndef INC_IS25LP040E_H_
#define INC_IS25LP040E_H_

#include "stdint.h"
#include "stdbool.h"


#define IS25LP_PAGE_SIZE            256      // 256 Bytes
#define IS25LP_SECTOR_SIZE          4096     // 4KB
#define IS25LP_BLOCK_32K_SIZE       32768    // 32KB
#define IS25LP_BLOCK_64K_SIZE       65536    // 64KB
#define IS25LP_CHIP_SIZE            524288   // 512KB (4Mbit)
#define IS25LP_TOTAL_SECTORS        128      // 512KB / 4KB
#define IS25LP_TOTAL_PAGES          2048     // 512KB / 256B

// Manufacturer & Device IDs
#define IS25LP_MANUFACTURER_ID      0x9D     // ISSI
#define IS25LP_DEVICE_ID            0x13     // 4Mbit (512KB)
#define IS25LP_JEDEC_ID             0x6013   // Memory Type + Capacity

typedef enum
{
	IS25LP_ERROR = false,
	IS25LP_OK = true
} eIS25LP_Status_t;

typedef struct
{
    uint8_t manufacturer_id;    // 0x9D für ISSI
    uint8_t memory_type;        // 0x60
    uint8_t capacity;           // 0x13 für 4Mbit
    uint8_t unique_id[8];       // 64-bit Unique ID
    bool initialized;
} sIS25LP_DeviceInfo_t;

eIS25LP_Status_t IS25LP_Init(void);
eIS25LP_Status_t IS25LP_ReadJedecID(uint8_t *manufacturer, uint8_t *memory_type, uint8_t *capacity);
eIS25LP_Status_t IS25LP_ReadDeviceID(uint8_t *manufacturer, uint8_t *device_id);
eIS25LP_Status_t IS25LP_ReadUniqueID(uint8_t *unique_id);
eIS25LP_Status_t IS25LP_GetDeviceInfo(sIS25LP_DeviceInfo_t *info);
eIS25LP_Status_t IS25LP_Read(uint32_t address, uint8_t *buffer, uint32_t length);
eIS25LP_Status_t IS25LP_WritePage(uint32_t address, const uint8_t *buffer, uint16_t length);
eIS25LP_Status_t IS25LP_Write(uint32_t address, const uint8_t *buffer, uint32_t length);
eIS25LP_Status_t IS25LP_EraseSector(uint32_t address);
eIS25LP_Status_t IS25LP_EraseBlock32K(uint32_t address);
eIS25LP_Status_t IS25LP_EraseBlock64K(uint32_t address);
eIS25LP_Status_t IS25LP_EraseChip(void);

#endif /* INC_IS25LP040E_H_ */
