#include "KalmanFilter.h"

// Constructor
KalmanFilter::KalmanFilter(float q, float r, float p, float initial_value)
  : _q(q), _r(r), _p(p), _x(initial_value), _initialized(false) {
}

// Yeni ölçümü filtrele (Kalman algoritması)
float KalmanFilter::update(float measurement) {
  // İlk ölçüm ise direkt kullan
  if (!_initialized) {
    _x = measurement;
    _initialized = true;
    return _x;
  }
  
  // Tahmin aşaması (Prediction)
  // x_pred = x (önceki durum)
  // p_pred = p + q (tahmin hatası artışı)
  _p = _p + _q;
  
  // Güncelleme aşaması (Update)
  // Kalman kazancı hesapla: k = p / (p + r)
  float k = _p / (_p + _r);
  
  // Yeni tahmin: x = x + k * (measurement - x)
  _x = _x + k * (measurement - _x);
  
  // Tahmin hatasını güncelle: p = (1 - k) * p
  _p = (1.0 - k) * _p;
  
  return _x;
}

// Filtreyi sıfırla
void KalmanFilter::reset() {
  _x = 0.0;
  _p = 1.0;
  _initialized = false;
}

// Filtrelenmiş değeri döndür
float KalmanFilter::getValue() const {
  return _x;
}

// Process noise ayarla
void KalmanFilter::setProcessNoise(float q) {
  _q = q;
}

// Measurement noise ayarla
void KalmanFilter::setMeasurementNoise(float r) {
  _r = r;
}

// Kalman kazancını döndür
float KalmanFilter::getKalmanGain() const {
  return _p / (_p + _r);
}
