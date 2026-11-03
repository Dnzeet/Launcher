# Porting Firmware LilyGO T-Display (16MB Flash + LittleFS Bypass)

Konfigurasi ini disesuaikan khusus untuk board **LilyGO T-Display (TTGO ESP32 Standard ST7789)**:
- **Flash Memory:** 16MB (App 3.2MB x 2 OTA, LittleFS ~9.5MB)
- **Display:** ST7789 (135x240 pixels)
- **Tombol Navigasi:** GPIO 0 (Button 1) & GPIO 35 (Button 2)
- **Storage Bypass:** SD Card diredireksi langsung ke **LittleFS** internal flash.

## Petunjuk Penggunaan (PlatformIO):
1. Ekstrak folder ini ke dalam workspace firmware kamu.
2. Buka folder proyek di VS Code (PlatformIO).
3. Pilih environment `lilygo-t-display-ttgo-16mb`.
4. Jalankan `Build` / `Upload`.
5. Upload File System Image via PlatformIO (`Upload Filesystem Image`).
