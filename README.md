

# ⚓ Battleship Game

A single-player Battleship game developed on the NUC140 embedded system board. The game uses **UART communication**, **GPIO**, **timers**, and **interrupts** to provide an interactive, coordinate-based shooting experience with real-time feedback via **LCD** and **LEDs**.

## 🎮 Game Features

- **Platform:** NUC140 Board
- **Gameplay:** 8x8 grid with hit/miss detection
- **Communication:** UART for map loading from PC
- **Controls:** GPIO keypad for coordinate selection
- **Display:** Real-time game status on LCD screen
- **Feedback:** LED indicators for hits, misses, and game over

## 🚀 How to Play

1. **Map Loading**: The game map is loaded via UART from a PC, displaying a "Map Loaded Successfully" message on the LCD.
2. **Start the Game**: Press the start button (SW_INT) to begin.
3. **Select Coordinates**: Use the keypad to choose X and Y coordinates.
4. **Fire Shots**: After selecting, the system checks for hits or misses, which are displayed on the LCD and indicated with LEDs.
5. **Game Over**: The game ends when all ships are hit or when the player runs out of shots.

## 🔧 System Setup

- **Clock Setup**: 12 MHz external crystal and low-speed internal oscillator for precise timing.
- **Timers**:
  - Timer0: Scans the 7-segment display and debounces keypad input.
  - Timer1: Handles ship movement in high-score mode.
- **GPIO Configuration**: 
  - LEDs and buzzer for game feedback.
  - Keypad for coordinate selection.
- **UART Communication**: Loads game maps from a PC via UART0.
- **SPI Interface**: Manages communication with the LCD for real-time game display.

## 🧠 Game Logic

- **Map Loading**: Map data is received via UART and stored in a 2D array.
- **Coordinate Selection**: The player uses the keypad to input X and Y coordinates.
- **Hit/Miss Detection**: Game logic checks for hits or misses and updates the game state on the LCD.
- **High Score Mode**: Ships move continuously in this mode, adding a dynamic challenge.

## 🛠️ Installation

1. **Hardware Setup**: Connect the NUC140 board to a PC via UART.
2. **Load Map**: Upload the Battleship map via UART from your terminal software.
3. **Start Playing**: Follow the instructions on the LCD and use the keypad to play.


## 📈 Learning Outcomes

- Mastered the configuration of UART, GPIO, and timers on embedded systems.
- Developed an interactive game with efficient use of resources.
- Gained hands-on experience with interrupt-driven embedded applications.


## 👥 Team Members

- **Shirin Shujaa** (S3983427)
- **Nguyen Dinh Quoc Bao** (S3938309)
- **Le Thien Son** (S3977955)


## 🛠️ Technologies Used

- **Embedded C**
- **NUC140 Board**
- **UART / SPI / GPIO**
- **Timers & Interrupts**

