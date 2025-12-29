# Vinyl_Simulator
ESP32 Bluetooth Audio Player + NFC Reader + Motor (Vinyl Simulator)

Complete ESP32 project that combines:

📀 Bluetooth audio playback (A2DP)

💾 WAV file reading from SD card

🏷️ NFC tag reading (PN532 via I2C)

⚙️ Continuous stepper motor control (turntable / vinyl style)

This repository is designed to be cloned and built directly.

📦 Hardware Used

ESP32 Dev Module

Stepper motor driver (ULN2003 or equivalent)

28BYJ‑48 stepper motor (or similar)

PN532 NFC module (I2C mode)

SD Card module (SPI)

Bluetooth speaker

USB power supply or Li‑ion battery + boost module

🔌 Wiring
🏷️ PN532 (I2C)
PN532	ESP32
SDA	GPIO 21
SCL	GPIO 22
VCC	3.3V
GND	GND

⚠️ Do NOT use 5V on I2C

💾 SD Card (SPI)
SD	ESP32
CS	GPIO 5
MOSI	GPIO 23
MISO	GPIO 19
SCK	GPIO 18
VCC	3.3V
GND	GND
⚙️ Stepper Motor
Driver	ESP32
IN1	GPIO 25
IN2	GPIO 26
IN3	GPIO 27
IN4	GPIO 14
📁 Project Structure
esp32-nfc-audio-motor/
│
├── main/
│   ├── main.cpp
│   ├── motor.cpp
│   ├── motor.h
│   ├── nfc.cpp
│   ├── nfc.h
│   ├── audio.cpp
│   └── audio.h
│
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
⚙️ Motor – Continuous Operation

The motor spins continuously like a turntable using AccelStepper logic.

motor.setMaxSpeed(1000);
motor.setSpeed(500);
motor.runSpeed();

Speed can be adjusted without affecting Bluetooth or NFC.

🏷️ NFC Logic

Each NFC tag UID is mapped to a specific folder on the SD card.

Runtime behavior:

The PN532 continuously scans for NFC tags while the system is idle

When a tag is detected, its UID is read and matched against a UID table

Each valid UID corresponds to one folder containing WAV files

NFC scanning is paused once audio playback starts to avoid RF/timing interference

Example mapping logic:

UID 04:A1:BC:92 → /sdcard/album_rock/

UID 93:7F:21:0A → /sdcard/album_jazz/

⚙️ Motor Synchronization (33 RPM)

The stepper motor simulates a vinyl turntable.

The motor starts at the exact moment audio playback begins

Speed is calibrated to 33 RPM

Motor runs continuously while audio is playing

Motor stops immediately when audio playback ends

Motor control runs in its own task so it does not block:

Bluetooth audio

SD card reads

System responsiveness

🔄 System Flow Diagram
flowchart TD
    A[ESP32 Boot] --> B[Initialize SD Card]
    B --> C[Initialize Bluetooth A2DP]
    C --> D[Initialize NFC PN532]
    D --> E[Idle State / Waiting for NFC Tag]


    E -->|NFC Tag Detected| F[Read NFC UID]
    F --> G{UID Recognized?}


    G -->|No| E
    G -->|Yes| H[Map UID to SD Folder]


    H --> I[Load WAV File]
    I --> J[Start Bluetooth Audio Streaming]


    J --> K[Start Motor Task]
    K --> L[Stepper Motor Spins at 33 RPM]


    L --> M{Audio Finished?}
    M -->|No| L
    M -->|Yes| N[Stop Motor]
    N --> E
🔊 Bluetooth Audio

16‑bit PCM WAV files

Bluetooth A2DP streaming

Logging disabled to avoid audio glitches

🔋 Power

ESP32 powered via USB or

Li‑ion battery + 5V step‑up converter

⚠️ Stepper motor should preferably use a separate power supply.

🚀 Build & Flash
idf.py build
idf.py flash monitor
✅ Project Status

✔ Stable motor control ✔ NFC working reliably ✔ Clean Bluetooth audio ✔ SD card without dropouts

🧠 Important Notes

Avoid boot‑critical GPIOs

PN532 must always run at 3.3V

SD card and NFC use separate buses

📜 License

MIT

✨ Author

Project developed for interactive physical control using ESP32.
