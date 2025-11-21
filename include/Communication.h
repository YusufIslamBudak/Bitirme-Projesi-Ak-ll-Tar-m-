#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include "LoRa_E32.h"
#include "JSONFormatter.h"

// LoRa veri paketi yapisi
#pragma pack(push, 1)
struct SensorDataPacket {
  // BME680 (16 byte)
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
  
  // BH1750 (4 byte)
  float lux;
  
  // MH-Z14A (3 byte)
  uint16_t co2_ppm;
  int8_t co2_temperature;
  
  // Toprak Nem (6 byte)
  float soil_moisture_percent;
  uint16_t soil_moisture_raw;
  
  // Kontrol Durumları (6 byte)
  uint8_t roof_position;        // 0-100%
  uint8_t fan_state;            // 0/1
  uint8_t light_state;          // 0/1
  uint8_t pump_state;           // 0/1
  uint16_t irrigation_duration; // saniye
  
  // Hesaplanan Değerler (12 byte)
  float dew_point;
  float heat_index;
  float absolute_humidity;
  
  // Sistem (5 byte)
  uint32_t uptime;              // saniye
  uint8_t mhz14a_ready;         // 0/1
  
  // Veri Bütünlüğü (2 byte)
  uint16_t crc;
};
#pragma pack(pop)

class Communication {
public:
  // Constructor
  Communication(LoRa_E32& lora, int m0Pin, int m1Pin);
  
  // Initialization
  void begin();
  void initLoRa();
  void initNodeMCU();  // Serial2 başlatma (NodeMCU için)
  
  // Serial operations
  void printHeader();
  void printCommandHelp();
  void clearScreen();
  void printDebug(const String& message);
  void printInfo(const String& message);
  void printWarning(const String& message);
  void printError(const String& message);
  void printSuccess(const String& message);
  
  // LoRa operations (Binary - Eski format)
  bool sendLoRaPacket(const SensorDataPacket& packet);
  void printPacketSummary(const SensorDataPacket& packet);
  
  // LoRa operations (JSON - Yeni format)
  bool sendLoRaJSON(const char* jsonString);
  
  // NodeMCU operations (Serial2 - JSON)
  void sendToNodeMCU(const char* jsonString);
  
  // Utility functions
  uint16_t calcCRC(const SensorDataPacket& packet);
  void hexDump(const uint8_t* data, size_t len);
  
private:
  LoRa_E32& _lora;
  int _m0Pin;
  int _m1Pin;
  JSONFormatter _jsonFormatter;  // JSON formatter nesnesi
};

#endif // COMMUNICATION_H
