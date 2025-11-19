#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <Arduino.h>

/**
 * @brief 1 boyutlu Kalman filtresi implementasyonu
 * 
 * Sensör gürültüsünü azaltmak ve daha stabil okumalar elde etmek için kullanılır.
 * Her sensör değeri için ayrı bir KalmanFilter nesnesi oluşturulmalıdır.
 * 
 * Kalman filtresi 2 aşamada çalışır:
 * 1. Tahmin (Prediction): Bir önceki durumdan yeni durum tahmini
 * 2. Güncelleme (Update): Yeni ölçümle tahmini birleştirme
 * 
 * Parametreler:
 * - q: Process noise (sistem gürültüsü) - Sistemin ne kadar değişken olduğu
 * - r: Measurement noise (ölçüm gürültüsü) - Sensörün ne kadar güvenilir olduğu
 * - p: Estimation error (tahmin hatası)
 * - k: Kalman gain (Kalman kazancı) - Ölçüme ne kadar güvenilir
 */
class KalmanFilter {
private:
  float _q;  // Process noise covariance (sistem gürültüsü)
  float _r;  // Measurement noise covariance (ölçüm gürültüsü)
  float _p;  // Estimation error covariance (tahmin hatası)
  float _x;  // Filtered value (filtreli değer)
  bool _initialized;  // İlk ölçüm alındı mı?

public:
  /**
   * @brief Kalman filtresi constructor
   * @param q Process noise (varsayılan: 0.001) - Düşük değer = yavaş değişim
   * @param r Measurement noise (varsayılan: 0.1) - Düşük değer = sensöre çok güven
   * @param p Initial estimation error (varsayılan: 1.0)
   * @param initial_value Başlangıç değeri (varsayılan: 0.0)
   */
  KalmanFilter(float q = 0.001, float r = 0.1, float p = 1.0, float initial_value = 0.0);
  
  /**
   * @brief Yeni ölçümü filtrele
   * @param measurement Ham sensör ölçümü
   * @return Kalman filtreli değer
   */
  float update(float measurement);
  
  /**
   * @brief Filtreyi sıfırla
   */
  void reset();
  
  /**
   * @brief Filtrelenmiş değeri döndür
   * @return Son filtreli değer
   */
  float getValue() const;
  
  /**
   * @brief Process noise değerini ayarla
   * @param q Yeni process noise değeri (0.0001 - 1.0 arası önerilir)
   */
  void setProcessNoise(float q);
  
  /**
   * @brief Measurement noise değerini ayarla
   * @param r Yeni measurement noise değeri (0.01 - 10.0 arası önerilir)
   */
  void setMeasurementNoise(float r);
  
  /**
   * @brief Kalman kazancını döndür (debug için)
   * @return Kalman gain değeri (0-1 arası)
   */
  float getKalmanGain() const;
};

#endif // KALMAN_FILTER_H
