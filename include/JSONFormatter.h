// ============================================================================
// JSON FORMATTER MODULE - HEADER
// ============================================================================
// Amaç: Sensör verilerini JSON formatına dönüştürme
// Kullanım: LoRa ve NodeMCU için yapılandırılmış veri gönderimi
// Format: Kompakt JSON (Firebase ve SD Kart için optimize)
// ============================================================================

#ifndef JSONFORMATTER_H
#define JSONFORMATTER_H

#include <Arduino.h>

class JSONFormatter {
private:
    static const uint16_t JSON_BUFFER_SIZE = 512;  // JSON buffer boyutu
    char jsonBuffer[JSON_BUFFER_SIZE];              // JSON string buffer

    // Yardımcı fonksiyonlar
    void appendKeyValue(const char* key, float value, int decimals, bool isLast = false);
    void appendKeyValue(const char* key, int value, bool isLast = false);
    void appendKeyValue(const char* key, const char* value, bool isLast = false);
    void appendKeyValue(const char* key, bool value, bool isLast = false);

public:
    JSONFormatter();
    
    // JSON oluşturma (Tüm veriler)
    const char* createFullJSON(
        // BME680 verileri
        float temp_raw, float temp_filtered,
        float hum_raw, float hum_filtered,
        float press_raw, float press_filtered,
        float gas_raw, float gas_filtered,
        
        // BH1750 verileri
        float lux_raw, float lux_filtered,
        
        // MH-Z14A verileri
        int co2_raw, int co2_filtered,
        int co2_temp,
        
        // Toprak nem verileri
        float soil_raw, float soil_filtered,
        int soil_adc,
        
        // Hesaplanan değerler (filtered verilerden)
        float dew_point,
        float heat_index,
        float abs_humidity,
        float sea_level_press,
        
        // Kontrol durumları
        int roof_position,
        bool fan_state,
        bool light_state,
        bool pump_state,
        int irrigation_duration,
        
        // Sistem bilgileri
        unsigned long uptime,
        bool mhz14a_ready,
        const char* last_roof_reason,
        const char* last_irrigation_reason
    );
    
    // JSON oluşturma (Sadece filtered değerler - Kompakt)
    const char* createCompactJSON(
        float temp, float hum, float press, float gas,
        float lux, int co2, float soil,
        int roof, bool fan, bool light, bool pump,
        unsigned long uptime
    );
    
    // JSON oluşturma (NodeMCU için - Firebase optimize)
    const char* createFirebaseJSON(
        float temp, float hum, float press, float gas,
        float lux, int co2, float soil,
        float dew_point, float heat_index,
        int roof, bool fan, bool light, bool pump,
        unsigned long uptime
    );
    
    // Buffer'ı temizleme
    void clearBuffer();
    
    // Buffer'ı alma
    const char* getBuffer();
    
    // JSON boyutunu alma
    uint16_t getJSONSize();
};

#endif // JSONFORMATTER_H
