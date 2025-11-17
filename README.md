# IS25LP040E SPI Flash Driver for STM32

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![STM32](https://img.shields.io/badge/STM32-G0B1-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32g0-series.html)
[![Flash Memory](https://img.shields.io/badge/Flash-IS25LP040E-green.svg)](https://www.issi.com/WW/pdf/IS25LP040E.pdf)

A comprehensive and production-ready SPI Flash memory driver for the **ISSI IS25LP040E** (4Mbit/512KB) Flash memory chip, designed for STM32 microcontrollers using HAL libraries.

![Alt](https://repobeats.axiom.co/api/embed/c17c7bd69544ac58bca96221504a7c0260ed8360.svg "Repobeats analytics image")

---

## 📋 Table of Contents

- [Features](#-features)
- [Hardware Specifications](#-hardware-specifications)
- [Supported Operations](#-supported-operations)
- [Getting Started](#-getting-started)
  - [Prerequisites](#prerequisites)
  - [Hardware Setup](#hardware-setup)
  - [Integration](#integration)
- [Usage Examples](#-usage-examples)
- [API Reference](#-api-reference)
- [Project Structure](#-project-structure)
- [Configuration](#-configuration)
- [Contributing](#-contributing)
- [License](#-license)
- [Author](#-author)

---

## ✨ Features

- ✅ **Full Doxygen Documentation** - Well-documented code with comprehensive comments
- ✅ **HAL-Based Implementation** - Uses STM32 HAL for maximum portability
- ✅ **Modular Handle-Based Architecture** - Support for multiple Flash instances
- ✅ **Complete Flash Operations** - Read, Fast Read, Write, Erase (Sector/Block/Chip)
- ✅ **Device Identification** - JEDEC ID, Manufacturer ID, Unique 64-bit ID
- ✅ **Error Handling** - Robust error checking and timeout management
- ✅ **Memory Management** - Automatic page and sector boundary handling
- ✅ **Production Ready** - Tested and verified implementation
- ✅ **Clean Code** - Follows modern C coding standards

---

## 🔧 Hardware Specifications

### IS25LP040E Flash Memory
- **Manufacturer**: ISSI (Integrated Silicon Solution, Inc.)
- **Capacity**: 4 Mbit (512 KB / 0.5 MB)
- **Interface**: Standard SPI (up to 104 MHz)
- **Operating Voltage**: 2.3V - 3.6V
- **Page Size**: 256 Bytes
- **Sector Size**: 4 KB
- **Block Size**: 32 KB / 64 KB
- **JEDEC ID**: 0x9D6013

### STM32 Target
- **MCU**: STM32G0B1KEU6N
- **Core**: ARM Cortex-M0+
- **Flash**: 512 KB
- **RAM**: 144 KB
- **IDE**: STM32CubeIDE

---

## 🚀 Supported Operations

### Device Information
- ✅ Initialize Flash memory (`IS25LP_Init`)
- ✅ Read JEDEC ID (`IS25LP_ReadJedecID`)
- ✅ Read Manufacturer/Device ID (`IS25LP_ReadDeviceID`)
- ✅ Read 64-bit Unique ID (`IS25LP_ReadUniqueID`)
- ✅ Get Device Information (`IS25LP_GetDeviceInfo`)

### Memory Operations
- ✅ Read data from any address (`IS25LP_Read`)
- ✅ Fast read for higher speeds (`IS25LP_FastRead`)
- ✅ Write single page (256 bytes) (`IS25LP_WritePage`)
- ✅ Write multiple pages (`IS25LP_Write`)
- ✅ Erase 4KB sector (`IS25LP_EraseSector`)
- ✅ Erase 32KB block (`IS25LP_EraseBlock32K`)
- ✅ Erase 64KB block (`IS25LP_EraseBlock64K`)
- ✅ Erase entire chip (`IS25LP_EraseChip`)

---

## 🎯 Getting Started

### Prerequisites

- **STM32CubeIDE** (or any STM32-compatible toolchain)
- **STM32CubeMX** for configuration
- **IS25LP040E** Flash memory chip
- **STM32G0** series microcontroller (or compatible)

### Hardware Setup

Connect the IS25LP040E to your STM32 via SPI:

| IS25LP040E Pin | STM32 Pin | Function |
|----------------|-----------|----------|
| CS (Pin 1)     | GPIO      | Chip Select (NSS) |
| SO (Pin 2)     | SPI MISO  | Data Out |
| WP# (Pin 3)    | 3.3V/GPIO | Write Protect (High) |
| GND (Pin 4)    | GND       | Ground |
| SI (Pin 5)     | SPI MOSI  | Data In |
| SCK (Pin 6)    | SPI CLK   | Clock |
| HOLD# (Pin 7)  | 3.3V/GPIO | Hold (High) |
| VCC (Pin 8)    | 3.3V      | Power Supply |

### Integration

1. **Clone the repository**:
   ```bash
   git clone https://github.com/MootSeeker/IS258LPXXXE-Driver.git
   ```

2. **Copy driver files** to your project:
   - `Core/Inc/is25lp040e.h`
   - `Core/Src/is25lp040e.c`

3. **Configure SPI in STM32CubeMX**:
   - Enable SPI1 (or your preferred SPI)
   - Configure as Master, Full-Duplex
   - Set clock speed (up to 104 MHz supported)
   - Configure CS pin as GPIO Output

4. **Include header** in your `main.c`:
   ```c
   #include "is25lp040e.h"
   ```

5. **Initialize the driver**:
   ```c
   if (IS25LP_Init() == IS25LP_OK) {
       // Flash initialized successfully
   }
   ```

---

## 💡 Usage Examples

### Basic Initialization

```c
#include "is25lp040e.h"

sIS25LP_Handle_t flash_handle;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    
    // Configure Flash handle
    flash_handle.spi_handle = &hspi1;
    flash_handle.cs_gpio.port = SPI1_NSS_GPIO_Port;
    flash_handle.cs_gpio.pin = SPI1_NSS_Pin;
    flash_handle.wp_gpio.port = FLASH_WP_GPIO_Port;
    flash_handle.wp_gpio.pin = FLASH_WP_Pin;
    flash_handle.initialized = false;
    
    // Initialize Flash memory
    if (IS25LP_Init(&flash_handle) == IS25LP_OK) {
        // Success - Flash ready to use
    }
    
    while (1) {
        // Your code here
    }
}
```

### Read Device Information

```c
sIS25LP_DeviceInfo_t info;

if (IS25LP_GetDeviceInfo(&flash_handle, &info) == IS25LP_OK) {
    printf("Manufacturer: 0x%02X\n", info.manufacturer_id);
    printf("Memory Type: 0x%02X\n", info.memory_type);
    printf("Capacity: 0x%02X\n", info.capacity);
    printf("Unique ID: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", info.unique_id[i]);
    }
    printf("\n");
}
```

### Write and Read Data

```c
uint8_t write_data[256] = "Hello, Flash Memory!";
uint8_t read_data[256];
uint32_t address = 0x1000;

// Erase sector before writing
if (IS25LP_EraseSector(&flash_handle, address) == IS25LP_OK) {
    // Write data
    if (IS25LP_Write(&flash_handle, address, write_data, 256) == IS25LP_OK) {
        // Read back data
        if (IS25LP_Read(&flash_handle, address, read_data, 256) == IS25LP_OK) {
            // Verify data
            if (memcmp(write_data, read_data, 256) == 0) {
                // Success!
            }
        }
    }
}
```

### Erase Operations

```c
// Erase single 4KB sector
IS25LP_EraseSector(&flash_handle, 0x0000);

// Erase 32KB block
IS25LP_EraseBlock32K(&flash_handle, 0x8000);

// Erase 64KB block
IS25LP_EraseBlock64K(&flash_handle, 0x10000);

// Erase entire chip (use with caution!)
IS25LP_EraseChip(&flash_handle);
```

---

## 📚 API Reference

### Initialization Functions

```c
eIS25LP_Status_t IS25LP_Init(sIS25LP_Handle_t *handle);
```
Initialize the Flash memory and verify connection using the provided handle.

### Device Information Functions

```c
eIS25LP_Status_t IS25LP_ReadJedecID(sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *memory_type, uint8_t *capacity);
eIS25LP_Status_t IS25LP_ReadDeviceID(sIS25LP_Handle_t *handle, uint8_t *manufacturer, uint8_t *device_id);
eIS25LP_Status_t IS25LP_ReadUniqueID(sIS25LP_Handle_t *handle, uint8_t *unique_id);
eIS25LP_Status_t IS25LP_GetDeviceInfo(sIS25LP_Handle_t *handle, sIS25LP_DeviceInfo_t *info);
```

### Memory Operations

```c
eIS25LP_Status_t IS25LP_Read(sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length);
eIS25LP_Status_t IS25LP_FastRead(sIS25LP_Handle_t *handle, uint32_t address, uint8_t *buffer, uint32_t length);
eIS25LP_Status_t IS25LP_WritePage(sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint16_t length);
eIS25LP_Status_t IS25LP_Write(sIS25LP_Handle_t *handle, uint32_t address, const uint8_t *buffer, uint32_t length);
```

### Erase Operations

```c
eIS25LP_Status_t IS25LP_EraseSector(sIS25LP_Handle_t *handle, uint32_t address);    // 4KB
eIS25LP_Status_t IS25LP_EraseBlock32K(sIS25LP_Handle_t *handle, uint32_t address);  // 32KB
eIS25LP_Status_t IS25LP_EraseBlock64K(sIS25LP_Handle_t *handle, uint32_t address);  // 64KB
eIS25LP_Status_t IS25LP_EraseChip(sIS25LP_Handle_t *handle);                        // Full chip
```

### Return Values

```c
typedef enum {
    IS25LP_ERROR = false,
    IS25LP_OK = true
} eIS25LP_Status_t;
```

---

## 📁 Project Structure

```
IS25LP040E/
├── Core/
│   ├── Inc/
│   │   ├── is25lp040e.h          # Driver header file
│   │   ├── main.h                # Main application header
│   │   ├── spi.h                 # SPI configuration
│   │   └── gpio.h                # GPIO configuration
│   ├── Src/
│   │   ├── is25lp040e.c          # Driver implementation
│   │   ├── main.c                # Main application
│   │   ├── spi.c                 # SPI initialization
│   │   └── gpio.c                # GPIO initialization
│   └── Startup/
│       └── startup_stm32g0b1keuxn.s
├── Drivers/
│   ├── CMSIS/                    # ARM CMSIS libraries
│   └── STM32G0xx_HAL_Driver/    # STM32 HAL drivers
├── Debug/                        # Build output (ignored)
├── .project                      # Eclipse project file
├── .cproject                     # Eclipse C project settings
├── .mxproject                    # STM32CubeMX project
├── G0_IS258LPXXXE.ioc           # CubeMX configuration
├── STM32G0B1KEUXN_FLASH.ld      # Linker script (Flash)
├── STM32G0B1KEUXN_RAM.ld        # Linker script (RAM)
├── LICENSE                       # MIT License
└── README.md                     # This file
```

---

## ⚙️ Configuration

### SPI Settings (STM32CubeMX)

- **Mode**: Master, Full-Duplex
- **NSS**: Software/GPIO Output
- **Data Size**: 8 Bits
- **First Bit**: MSB First
- **Clock Polarity**: Low (CPOL = 0)
- **Clock Phase**: 1 Edge (CPHA = 0)
- **Baud Rate**: Adjust based on your requirements (max 104 MHz)

### Memory Constants

Defined in `is25lp040e.h`:

```c
#define IS25LP_PAGE_SIZE            256      // 256 Bytes
#define IS25LP_SECTOR_SIZE          4096     // 4KB
#define IS25LP_BLOCK_32K_SIZE       32768    // 32KB
#define IS25LP_BLOCK_64K_SIZE       65536    // 64KB
#define IS25LP_CHIP_SIZE            524288   // 512KB (4Mbit)
```

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 👤 Author

**MootSeeker**

- GitHub: [@MootSeeker](https://github.com/MootSeeker)
- Repository: [IS258LPXXXE-Driver](https://github.com/MootSeeker/IS258LPXXXE-Driver)

---

## 🙏 Acknowledgments

- STMicroelectronics for the STM32 HAL libraries
- ISSI for the IS25LP040E Flash memory datasheet
- STM32 community for continuous support

---

**⭐ If this project helped you, please consider giving it a star!**
