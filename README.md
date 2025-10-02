# Rower Display Interface – Arduino Touch UI for NASA X-Hab Rower System

This project implements a user interface for a conceptual mars exercise rower, designed to help astronauts monitor workout metrics such as repetitions and resistance weight. Inspired by a NASA X-Hab challenge, the display system provides real-time feedback via a 7" capacitive touch TFT and is built on the Arduino platform for reliability and embedded simplicity.

## 🚀 Motivation

The display is part of a prototype for an **astronaut rowing machine** intended for Mars habitats. It helps visualize:
- Number of reps completed
- Weight/resistance per rep
- System responsiveness to real-time motion data (e.g., from sensors on the rower)

This aligns with goals set by the NASA X-Hab Academic Innovation Challenge, promoting physical health in space through hardware innovation.

## 🖥️ Display Features

- High-resolution 800x480 TFT display
- Capacitive touchscreen input (I2C interface)
- Large, easy-to-read fonts for readability
- Designed to show:
  - Current rep count
  - Current weight/resistance setting
  - System status
  - Previous workout data

## 🔌 Hardware Requirements

- **Arduino Mega 2560** or **Arduino Due**
- **7” TFT display** (ER-TFTM070-4V2.1) with 16-bit parallel interface
- **Capacitive Touch Panel** using I2C (address `0x38`)
- Data connection from rowing machine microcontroller or sensor suite

## 📁 Project Structure

- `sketch-code.ino`: Main Arduino sketch handling display and touch input
- `blissTouchScreen/`: Touch screen interface library
- `*.c`: Custom fonts for text and numeric display

## 🔧 Dependencies

- `UTFT` library (display control)
- `Wire.h` (I2C communication)
- `SPI.h` (optional fallback interface)

## 🧪 Setup & Usage

1. Wire the TFT screen and touch controller to the microcontroller.
2. Upload `sketch-code.ino` to the Arduino Mega or Due.
3. Connect incoming telemetry data (e.g., via UART, I2C, or SPI).
4. The screen will automatically display rep counts and weight values in real-time.

## 🧠 Difficulty

**Intermediate** — designed for students and engineers familiar with Arduino, embedded I/O, and graphical displays.

