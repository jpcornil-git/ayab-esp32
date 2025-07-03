# ESP32 WebApp platform for Ayab

UnoR4 (esp32s3) platform to support ayab webapp development

## Features
- WiFi connectivity
    - STA mode with DHCP client for normal operations
    - AP mode with DHCP server when there is no WiFi or to configure WiFi
- mDNS server to access device by name rather than IP address (default=ayab.local) 
- websocket to serial proxy enabling ayab api over a network (with webapp or ayab-desktop)
- http server to access the ayab webapp
- SPIFFS file system to store http server files (html, css, js, ...)
- Over-The-Air (OTA) updates for esp32 firmware, ayab (RA4M1) firmware or SPIFFS (partition or individual files)

## How to build
- Clone this repository
- Command line:
    - Setup IDF environment
        ``` bash
        > source <idf path >/esp-idf/export.sh
        ```
    - Build project
        ``` bash
        > idf.py build
        ```
- VS Code:
    - Install vscode & tools: https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/prerequisites.html
    - Install esp-idf vscode extension: https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html
    - Import this project within vscode
    - **Important**: Set target to `esp32s3` (extension doesn't take CONFIG_IDF_TARGET from sdkconfig.defaults into account to setup target)
    - Build project

## Flash Update
### Install ayab-esp32
- Find *download* and *gnd* pin  on the 6-pin header next to the USB-C connector and short them with a jumper
- Connect UnoR4 USB (or reboot ESP32 to take above into account)
- Run one the following method to write the flash image produced above:
    - command line :
        ``` bash
        > esptool.py write_flash 0x0 build/ayab-esp32_flash.bin
        ```
    - web application: https://espressif.github.io/esptool-js/ (flash from 0x0 and use 460800 for the baud rate)
    - Wait until the flash update is completely finished !
- Disconnect UnoR4 USB
- Remove jumper and restart (unplug USB)
    - Note: at first boot the RA4M1 flash is updated (~3-4 seconds extra to boot) and device will fall back in AP mode (ãfter ~10s/3 failing attempt to connect to WiFi) 
- Connect to device's WiFi (SSID="AP Ayab Wifi")
    - Navigate to http://ayab.local or http://192.168.4.1 and set your WiFi credentials from the WiFi menu
        - Android phone doesn't support mDNS -> use http://192.168.4.1 instead of ayab.local
    - Restart to connect device to your WiFi network
    - Navigate to ayab.local

### Restore factory firmware
- See https://support.arduino.cc/hc/en-us/articles/16379769332892-Restore-the-USB-connectivity-firmware-on-UNO-R4-WiFi-with-espflash

### Flash recovery (Espressif's Flash Download Tool):
- This tool can be used to e.g. recover a bricked device (bootloader partition corrupted)
- https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html
- Flash new bootloader (or full flash binary) at address 0x0

## UnoR4 Serial connectivity
- Schematics: https://docs.arduino.cc/resources/schematics/ABX00087-schematics.pdf

```
            +-------+                    +-------+
USBSerial --| ESP32 |--Serial0--Serial --| RA4M1 |-- Serial1-- pin header
(USBCDC)    |       | (Tx/43, Rx/44)     |       |
            |       |--Serial1--Serial2--|       |
            |       |                    |       |
            |       |--gpio4 -- Reset  --|       |
            |       |                    |       |
            |       |--gpio9 -- MD/Boot--|       |
            +-------+                    +-------+
```



