#ifndef __SD_FUNCTIONS_H
#define __SD_FUNCTIONS_H
#include <globals.h>

#include "idf/idf_update.h"
#include <SPI.h>

#include <FFat.h>
#include <FS.h>
#include <SD.h>
#if !defined(SDM_SD)
#include <SD_MMC.h>
#endif
extern SPIClass sdcardSPI;

bool setupSdCard();

bool deleteFromSd(const String &path);

bool renameFile(const String &path, const String &filename);

bool copyFile(const String &path);

bool pasteFile(const String &path);

bool createFolder(String path);

void readFs(String &folder, std::vector<Option> &opt);

bool sortList(const Option &a, const Option &b);

String loopSD(bool filePicker = false);

void updateFromSD(const String &path);

// Shared install logic behind updateFromSD: parses the already-open file (raw app binary or
// multi-partition image) and flashes it. `file` just needs to be a valid, open fs::File --
// it works the same regardless of which filesystem (SD, LittleFS, ...) it was opened from.
void updateFromFile(File file, const String &path);

bool performDATAUpdate(Stream &updateSource, size_t updateSize, const char *label);

#endif
