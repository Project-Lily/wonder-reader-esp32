# Wonder Reader ESP32

This repository contains the code that will be used in the ESP32 microcontroller to control the wonder reader.

## Libraries used:
- ShiftRegister74HC595
- ESP32-audioI2S
- Keypad
- AccelStepper
- ArduinoJSON
- Arduino for ESP32

# Usage
1. Connect the ESP32 device into your computer and install the serial driver if you have to.
2. Install PlatformIO in Visual Studio Code as an extension.
3. Open the project in Visual Studio Code.
4. Duplicate `src/secret.h.example` to `src/secret.h`. Then fill out the Wi-Fi SSID and Password.
5. On the bottom left of vscode, click the "PlatformIO: Upload" button to compile the project and upload it into the ESP32.

# Wiring and Model
3D models and electrical circuit diagram can be found in this repository
https://github.com/Project-Lily/wonder-model