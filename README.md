# 🎵 ESP32 NFC Audio Player + Stepper Motor (Vinyl Simulator)

Complete ESP32 project which simulates a vinyl player using NFC tags.
Each NFC tag (UID) is mapped to a folder on the SD card containing WAV files.
When a valid tag is detected, audio starts playing via Bluetooth and the
stepper motor rotates continuously at **33 RPM**, like a real record player.

---

## 📦 Features

- 🎶 Bluetooth A2DP audio streaming
- 💾 WAV playback from SD card (PCM 44.1 kHz, 16-bit, stereo)
- 🏷️ NFC control using PN532 (I2C)
- 🔄 Stepper motor running at constant 33 RPM
- ⚙️ Motor runs in a dedicated FreeRTOS task (non-blocking)
- 🔋 Powered via USB or battery power (Li-Po + boost)



---

## 🔁 System Flow (Logic)

1. ESP32 boots
2. NFC reader waits for a tag
3. NFC UID is read
4. UID is matched to a folder on the SD card
5. WAV file(s) from that folder start playing via Bluetooth
6. Stepper motor starts rotating at **33 RPM**
7. Removing the NFC tag stops playback and the motor

---

## 🧩 Hardware Used

| Component | Description |
|---------|------------|
| ESP32 | ESP WROOM 32 Module |
| NFC | PN532 (I2C mode) |
| Motor | 28BYJ-48 Stepper |
| Driver | ULN2003 |
| Storage | microSD card (SPI) |
| Power | USB or Li-Po + TP4056 + Boost |
| Audio | Any Bluetooth speaker |



## 🔌 Wiring

### 🏷️ PN532 NFC (I2C)

| PN532 | ESP32 |
|------|------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |


---

### 💾 SD Card (SPI – VSPI)

| SD Card | ESP32 |
|--------|------|
| CS | GPIO 5 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK | GPIO 18 |
| VCC | 3.3V |
| GND | GND |

---

### ⚙️ Stepper Motor (ULN2003)

| ULN2003 | ESP32 |
|--------|------|
| IN1 | GPIO 25 |
| IN2 | GPIO 26 |
| IN3 | GPIO 27 |
| IN4 | GPIO 14 |
| VCC | 5V |
| GND | GND |

---

## 📁 SD Card Structure

/sdcard
├── /album_01
│ ├── track01.wav
│ └── track02.wav
│
├── /album_02
│ ├── song.wav
│
└── /album_03
└── audio.wav


Each folder corresponds to one NFC UID.

---

## 🏷️ NFC UID Mapping (Example)

```cpp
UID 04 A2 B1 C9 32 → /album_01
UID 93 7F 22 11 A0 → /album_02
UID A1 B2 C3 D4 E5 → /album_03
```

## ⚙️ Stepper Motor Control (33 RPM)

The motor runs in its own FreeRTOS task so Bluetooth audio is never blocked.

#define MOTOR_STEP_DELAY_US 200
#define MOTOR_STEPS_PER_DELAY 8

Speed tuning

Faster: decrease MOTOR_STEP_DELAY_US

Smoother: keep delay ≥ 150 µs

Target speed: ≈33 RPM (Vinyl spped)


## 🎧 Audio Requirements

WAV format must be:
PCM
44.1 kHz
16-bit
Stereo

Other formats will not play correctly.



## 🖨️ .STL files to be prrinted on a 3D printer



## 🚀 Build & Flash (ESP-IDF)
idf.py set-target esp32
idf.py build
idf.py flash monitor



## 📜 License



## ✨ Author
Me
NFC-controlled vinyl-style audio player
