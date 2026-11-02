#ifndef FLASH_BIN_STORE_H
#define FLASH_BIN_STORE_H
#include <Arduino.h>

// This board has no SD card. Flash_bin_store mirrors the "SD" menu's browse ->
// pick -> install flow, but against the "binstore" LittleFS partition on
// internal flash instead (see support_files/custom_16Mb_ttgo.csv). The WebUI's
// plain file-manager upload is redirected to the same partition (see
// webInterface.cpp, guarded by HAS_FLASH_BIN_STORE), so a .bin gets onto the
// device by uploading it there once over WiFi, then it can be installed
// directly from the device screen any time after, with no WiFi/SD needed.

// Mounts (and formats on first boot) the "binstore" LittleFS partition at
// "/binstore". Safe to call repeatedly -- a no-op once already mounted.
bool ensureFlashBinFs();

// On-device browser for .bin files sitting in "/binstore": pick one to
// install (reuses the same install logic as the SD browser) or delete it.
// Mirrors loopSD(filePicker) -- pass true to just return the chosen path
// instead of showing the Install/Delete action menu.
String loopFlashBin(bool filePicker = false);

#endif
