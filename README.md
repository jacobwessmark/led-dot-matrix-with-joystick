# ESP32 LED Matrix with Joystick Control

This project controls an 8x8 LED matrix using an ESP32, daisy-chained shift registers, and a joystick. The matrix uses a common anode setup, where rows are connected to ground, and columns are supplied with positive voltage to turn on individual LEDs. The shift registers are daisy-chained and controlled using 16-bit arrays for seamless row and column management.

## Pin Definitions

### Joystick ADC Pins
- **SW_PIN**: GPIO 15  
- **ADC_1**: ADC1 Channel 1 (GPIO 2) - X-axis input
- **ADC_2**: ADC1 Channel 2 (GPIO 3) - Y-axis input

### Shift Register Pins
- **SER_PIN**: GPIO 41 - Serial Data Input
- **RCLK_PIN**: GPIO 39 - Register Clock (Latch)
- **SRCLK_PIN**: GPIO 42 - Shift Register Clock

## LED Matrix Configuration

### Common Anode Setup
- **Rows** are connected to ground (set low).
- **Columns** are connected to high voltage (set high) to turn on LEDs.

> **Note**: The orientation of rows and columns might vary depending on the user's perception of the matrix layout.


### Shift Register to LED Matrix Mapping

#### Daisy-Chained Shift Registers
The shift registers are daisy-chained, allowing us to control the entire 8x8 matrix with 16-bit arrays:
- **1st 8 Bits**: Controls rows
- **2nd 8 Bits**: Controls columns

#### 1st Shift Register (Row Control)
- **QA (Pin 9)**: Row 1
- **QB (Pin 14)**: Row 2
- **QC (Pin 8)**: Row 3
- **QD (Pin 12)**: Row 4
- **QE (Pin 1)**: Row 5
- **QF (Pin 7)**: Row 6
- **QG (Pin 2)**: Row 7
- **QH (Pin 5)**: Row 8

#### 2nd Shift Register (Column Control)
- **QA (Pin 13)**: Column 1
- **QB (Pin 3)**: Column 2
- **QC (Pin 4)**: Column 3
- **QD (Pin 10)**: Column 4
- **QE (Pin 6)**: Column 5
- **QF (Pin 11)**: Column 6
- **QG (Pin 15)**: Column 7
- **QH (Pin 16)**: Column 8

## 16-Bit Array Control

The LED matrix is driven using 16-bit arrays, where each bit represents a row or column state. This 16-bit control stream is shifted out to the daisy-chained shift registers, enabling precise control of each LED’s state by row and column.

## Usage

This setup allows control of the LED matrix using a joystick and the ESP32 with defined ADC and GPIO configurations. Adjustments to the joystick and button settings can modify the behavior of individual LEDs within the matrix.
