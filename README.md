# Multi-Sensor Environmental Monitoring System

## 📋 Project Description
A comprehensive environmental monitoring system using multiple sensors to measure air quality, temperature, humidity, light levels, and CO2 concentration.

## 🔧 Hardware Components

### Sensors:
- **BH1750** (GY-30) - Light Intensity Sensor (I2C)
- **BME680** - Temperature, Humidity, Pressure, Gas Sensor (I2C)
- **MH-Z14A** - CO2 Sensor (UART)

### Microcontroller:
- **Arduino Mega 2560**

## 📊 Measured Parameters

### Direct Measurements:
- Light Intensity (lux)
- Temperature (°C)
- Humidity (%)
- Atmospheric Pressure (hPa)
- Gas Resistance (KOhm)
- CO2 Level (ppm)

### Calculated Values (Scientific Formulas):
- Dew Point (Magnus-Tetens formula)
- Absolute Humidity (g/m³)
- Heat Index (NOAA formula)
- Vapor Pressure (Buck equation)
- Sea Level Pressure (Barometric formula)
- Altitude (m)
- CO2 Concentration (mg/m³) - Ideal Gas Law
- Foot-candles conversion
- Ventilation recommendations (ASHRAE 62.1)

## 🔌 Pin Connections

### I2C (BH1750 + BME680):
- **SDA** → D20
- **SCL** → D21

### UART (MH-Z14A):
- **RX** → D19 (Arduino RX1 ← MH-Z14A TX)
- **TX** → D18 (Arduino TX1 → MH-Z14A RX)

## 📚 Scientific Formulas Used

1. **Dew Point**: Magnus-Tetens equation (Alduchov & Eskridge, 1996)
2. **Absolute Humidity**: Thermodynamic equations
3. **Heat Index**: Rothfusz regression equation (NOAA, 1990)
4. **Vapor Pressure**: Buck equation (1981)
5. **Sea Level Pressure**: ISA (International Standard Atmosphere)
6. **CO2 Concentration**: Ideal Gas Law (PV = nRT)

## 🚀 Features

- ✅ Real-time multi-sensor data acquisition
- ✅ Scientific calculations based on established formulas
- ✅ MH-Z14A warm-up period tracking (3 minutes)
- ✅ Automatic sensor detection and initialization
- ✅ Air quality assessment
- ✅ Clear serial monitor output with ANSI formatting

## 🛠️ Setup

1. Install PlatformIO
2. Clone this repository
3. Connect sensors according to pin connections
4. Upload the code to Arduino Mega 2560
5. Open serial monitor at 115200 baud

## 📖 Dependencies

- Wire (I2C communication)
- BH1750 library
- Adafruit BME680 library
- Adafruit Unified Sensor library

## 📝 License

This project is part of a graduation thesis.

## 👤 Author

Yusuf

## 📅 Date

October 2025
