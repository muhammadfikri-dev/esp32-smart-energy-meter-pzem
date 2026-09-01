# ⚡ Meteran Energi Listrik Cerdas PZEM-004T ESP32

[![Lisensi: MIT](https://img.shields.io/badge/Lisensi-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32%20%7C%20FreeRTOS-blue.svg)](https://espressif.com/)
[![Framework: Arduino IDE](https://img.shields.io/badge/Framework-Arduino%20IDE%202.0%2B-teal.svg)](https://www.arduino.cc/)
[![Status: Produksi](https://img.shields.io/badge/Status-Firmware%20Produksi-brightgreen.svg)](#)
[![Developer: Muhammad Fikri](https://img.shields.io/badge/Developer-Muhammad%20Fikri-blue.svg)](#)

Monitoring daya listrik AC multi-parameter (Volt, Ampere, Watt, kWh, Frekuensi), relay trip pengaman beban lebih, dan WebSocket dashboard.

---

## 🧠 Arsitektur Firmware & Fitur Produksi

- **FreeRTOS Dual-Core Multitasking:**
 - **Core 1 (Sensors & Actuation Task):** Menjalankan loop kontrol tertutup (*closed-loop control*), sampling sensor berkecepatan tinggi, dan aktuasi hardware tanpa terganggu latensi koneksi.
 - **Core 0 (Network & Telemetry Task):** Menangani konektivitas Wi-Fi, MQTT broker publish/subscribe, streaming data WebSockets, dan handler ArduinoOTA.
- **Thread Safety & Data Synchronization:** Sinkronisasi data antar-core menggunakan **Mutex Semaphore (`SemaphoreHandle_t`)** dan **FreeRTOS Queue** untuk mencegah *race condition*.
- **Digital Filtering & Kalman DSP:** Dilengkapi filter digital peredam derau (*noise reduction*) dan filter Kalman untuk akurasi data sensor analog.
- **Penyimpanan Non-Volatil NVS (Preferences):** Nilai konfigurasi, kalibrasi sensor, dan status aktuator tersimpan aman di flash internal ESP32.
- **Sistem Pengaman Mandiri (*Hardware Failsafe*):** Interlock proteksi perangkat keras otomatis saat terdeteksi anomali.
- **Over-The-Air (OTA) Updates & NTP Time:** Pembaruan firmware nirkabel via **`ArduinoOTA`** dan sinkronisasi waktu presisi via server NTP.

---

## 🔌 Skema Pinout & Koneksi Hardware

Lihat definisi pin lengkap dan konfigurasi pada file [`config.h.example`](./config.h.example).

---

## 🚀 Panduan Kompilasi & Upload Firmware

1. Buka file **`esp32-smart-energy-meter-pzem.ino`** menggunakan **Arduino IDE 2.0+** atau PlatformIO.
2. Salin template konfigurasi:
 ```bash
 cp config.h.example config.h
 ```
3. Buka `config.h` dan sesuaikan kredensial Wi-Fi, MQTT broker, dan parameter sensor.
4. Pilih board: **Tools > Board > ESP32 Arduino > ESP32 Dev Module**.
5. Pilih skema partisi: **Tools > Partition Scheme > "Default 4MB with ffat"** atau **"Minimal SPIFFS"**.
6. Klik **Upload** untuk mem-flash firmware ke perangkat ESP32 Anda.

---

## 📄 Lisensi
Didistribusikan di bawah lisensi open-source **MIT License**. Dibuat dengan ❤️ oleh **Muhammad Fikri Dev**.
