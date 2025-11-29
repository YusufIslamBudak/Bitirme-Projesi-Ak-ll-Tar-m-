// ============================================================================
// JSON FORMATTER MODULE - IMPLEMENTATION
// ============================================================================

#include "JSONFormatter.h"
#include <string.h>
#include <stdio.h>

// Constructor
JSONFormatter::JSONFormatter() {
    clearBuffer();
}

// Buffer temizleme
void JSONFormatter::clearBuffer() {
    memset(jsonBuffer, 0, JSON_BUFFER_SIZE);
}

// Buffer alma
const char* JSONFormatter::getBuffer() {
    return jsonBuffer;
}

// JSON boyutu
uint16_t JSONFormatter::getJSONSize() {
    return strlen(jsonBuffer);
}

// Yardımcı: Float key-value ekle
void JSONFormatter::appendKeyValue(const char* key, float value, int decimals, bool isLast) {
    char temp[64];
    // Arduino AVR icin dtostrf kullan (snprintf %f desteklemiyor)
    char valueStr[16];
    dtostrf(value, 1, decimals, valueStr);
    snprintf(temp, sizeof(temp), "\"%s\":%s%s", key, valueStr, isLast ? "" : ",");
    strcat(jsonBuffer, temp);
}

// Yardımcı: Integer key-value ekle
void JSONFormatter::appendKeyValue(const char* key, int value, bool isLast) {
    char temp[64];
    snprintf(temp, sizeof(temp), "\"%s\":%d%s", key, value, isLast ? "" : ",");
    strcat(jsonBuffer, temp);
}

// Yardımcı: String key-value ekle
void JSONFormatter::appendKeyValue(const char* key, const char* value, bool isLast) {
    char temp[128];
    snprintf(temp, sizeof(temp), "\"%s\":\"%s\"%s", key, value, isLast ? "" : ",");
    strcat(jsonBuffer, temp);
}

// Yardımcı: Boolean key-value ekle
void JSONFormatter::appendKeyValue(const char* key, bool value, bool isLast) {
    char temp[64];
    snprintf(temp, sizeof(temp), "\"%s\":%s%s", key, value ? "true" : "false", isLast ? "" : ",");
    strcat(jsonBuffer, temp);
}

// Tam JSON oluşturma (Tüm veriler - RAW + FILTERED)
const char* JSONFormatter::createFullJSON(
    float temp_raw, float temp_filtered,
    float hum_raw, float hum_filtered,
    float press_raw, float press_filtered,
    float gas_raw, float gas_filtered,
    float lux_raw, float lux_filtered,
    int co2_raw, int co2_filtered,
    int co2_temp,
    float soil_raw, float soil_filtered,
    int soil_adc,
    float dew_point, float heat_index, float abs_humidity, float sea_level_press,
    int roof_position, bool fan_state, bool light_state, bool pump_state, int irrigation_duration,
    unsigned long uptime, bool mhz14a_ready,
    const char* last_roof_reason, const char* last_irrigation_reason
) {
    clearBuffer();
    
    strcpy(jsonBuffer, "{");
    
    // BME680 - RAW
    appendKeyValue("tr", temp_raw, 2);
    appendKeyValue("hr", hum_raw, 2);
    appendKeyValue("pr", press_raw, 2);
    appendKeyValue("gr", gas_raw, 2);
    
    // BME680 - FILTERED
    appendKeyValue("tf", temp_filtered, 2);
    appendKeyValue("hf", hum_filtered, 2);
    appendKeyValue("pf", press_filtered, 2);
    appendKeyValue("gf", gas_filtered, 2);
    
    // BH1750 - RAW & FILTERED
    appendKeyValue("lr", lux_raw, 1);
    appendKeyValue("lf", lux_filtered, 1);
    
    // MH-Z14A - RAW & FILTERED
    appendKeyValue("cr", co2_raw);
    appendKeyValue("cf", co2_filtered);
    appendKeyValue("ct", co2_temp);
    
    // Toprak Nem - RAW & FILTERED
    appendKeyValue("sr", soil_raw, 2);
    appendKeyValue("sf", soil_filtered, 2);
    appendKeyValue("sa", soil_adc);
    
    // Hesaplanan değerler
    appendKeyValue("dew", dew_point, 2);
    appendKeyValue("hi", heat_index, 2);
    appendKeyValue("ah", abs_humidity, 2);
    appendKeyValue("slp", sea_level_press, 2);
    
    // Kontrol durumları
    appendKeyValue("roof", roof_position);
    appendKeyValue("fan", fan_state);
    appendKeyValue("light", light_state);
    appendKeyValue("pump", pump_state);
    appendKeyValue("irr_dur", irrigation_duration);
    
    // Sistem bilgileri
    appendKeyValue("uptime", (int)uptime);
    appendKeyValue("mhz_rdy", mhz14a_ready);
    appendKeyValue("roof_rsn", last_roof_reason);
    appendKeyValue("irr_rsn", last_irrigation_reason, true);  // Son eleman
    
    strcat(jsonBuffer, "}");
    
    return jsonBuffer;
}

// Kompakt JSON (Sadece filtered değerler - LoRa için)
const char* JSONFormatter::createCompactJSON(
    float temp, float hum, float press, float gas,
    float lux, int co2, float soil,
    int roof, bool fan, bool light, bool pump,
    unsigned long uptime
) {
    clearBuffer();
    
    strcpy(jsonBuffer, "{");
    
    appendKeyValue("t", temp, 2);
    appendKeyValue("h", hum, 2);
    appendKeyValue("p", press, 2);
    appendKeyValue("g", gas, 2);
    appendKeyValue("l", lux, 1);
    appendKeyValue("c", co2);
    appendKeyValue("s", soil, 2);
    appendKeyValue("r", roof);
    appendKeyValue("f", fan);
    appendKeyValue("lt", light);
    appendKeyValue("pm", pump);
    appendKeyValue("up", (int)uptime, true);  // Son eleman
    
    strcat(jsonBuffer, "}");
    
    return jsonBuffer;
}

// Firebase JSON (NodeMCU için - Detaylı ama optimize)
// KEY İSİMLERİ: NodeMCU karar ağacıyla uyumlu kısa isimler!
const char* JSONFormatter::createFirebaseJSON(
    float temp, float hum, float press, float gas,
    float lux, int co2, float soil,
    float dew_point, float heat_index,
    int roof, bool fan, bool light, bool pump,
    unsigned long uptime
) {
    clearBuffer();
    
    strcpy(jsonBuffer, "{");
    
    // Sensör verileri (filtered) - KISA KEY İSİMLERİ
    appendKeyValue("temp", temp, 2);      // temperature -> temp
    appendKeyValue("hum", hum, 2);        // humidity -> hum
    appendKeyValue("pres", press, 2);     // pressure -> pres
    appendKeyValue("gas", gas, 2);        // gas (aynı)
    appendKeyValue("lux", lux, 1);        // light -> lux (çakışma önlendi!)
    appendKeyValue("co2", co2);           // co2 (aynı)
    appendKeyValue("soil", soil, 2);      // soil_moisture -> soil
    
    // Hesaplanan değerler
    appendKeyValue("dew", dew_point, 2);  // dew_point -> dew
    appendKeyValue("heat", heat_index, 2); // heat_index -> heat
    
    // Kontrol durumları
    appendKeyValue("roof", roof);          // roof_position -> roof
    appendKeyValue("fan", fan);            // fan_active -> fan
    appendKeyValue("light", light);        // light_active -> light (bool)
    appendKeyValue("pump", pump);          // pump_active -> pump
    
    // Timestamp
    appendKeyValue("uptime", (int)uptime, true);  // Son eleman
    
    strcat(jsonBuffer, "}");
    
    return jsonBuffer;
}
