# CH390 Migration Summary

This document summarizes all the changes made to replace the W5500 Ethernet module with the CH390 module in the 4G router project.

## Changes Made

### 1. Component Dependencies

- Added `idf_component.yml` file in the project root to include the CH390 component:
  ```yaml
  dependencies:
    idf: '>=5.0'
    espressif/ch390: '0.3.0'
  ```

### 2. Code Modifications

- Updated `managed_components/espressif__iot_bridge/src/bridge_eth.c` to add CH390 support:
  - Added include headers for CH390: `esp_eth_phy_ch390.h` and `esp_eth_mac_ch390.h`
  - Added `esp_spi_eth_new_ch390()` function to initialize the CH390 module
  - Updated the `esp_bridge_spi_eth_module` array to include CH390 support

### 3. Configuration Changes

- Modified `sdkconfig` to disable W5500 and enable CH390:
  ```
  # CONFIG_ETH_SPI_ETHERNET_W5500 is not set
  CONFIG_ETH_SPI_ETHERNET_CH390=y
  ```

### 4. Documentation Updates

- Updated `README.md` to include CH390 configuration parameters
- Created `CH390_USAGE.md` with detailed instructions for using the CH390 module
- Created this summary document

## Hardware Configuration

The CH390 module is configured to use the following GPIO pins:
- SPI Host: 2
- SCLK: GPIO6
- MOSI: GPIO7
- MISO: GPIO15
- CS: GPIO5
- INT: GPIO4
- RST: GPIO16
- PHY Address: 1
- SPI Clock Speed: 25MHz

## Verification Steps

To verify that the CH390 module is working correctly:

1. Build the project:
   ```bash
   idf.py build
   ```

2. Flash the project to the ESP32:
   ```bash
   idf.py flash monitor
   ```

3. Check the serial output for Ethernet connection messages

4. Connect an Ethernet cable to the CH390 module and verify network connectivity

## Benefits of CH390

The CH390 offers several advantages over the W5500:
- Lower cost
- Integrated PHY (no external PHY chip required)
- Better power efficiency
- Compatible with the same SPI interface

## Troubleshooting

If you encounter issues with the CH390 module:

1. Verify all hardware connections
2. Check that the GPIO pins are not conflicting with other peripherals
3. Ensure the SPI clock speed is appropriate (25MHz is recommended)
4. Verify that the CH390 component is properly installed