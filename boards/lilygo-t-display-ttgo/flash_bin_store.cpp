#include "flash_bin_store.h"
#include "display.h"
#include "idf/launcher_platform.h"
#include "sd_functions.h"
#include <LittleFS.h>
#include <algorithm>
#include <globals.h>

static const char *BINSTORE_LABEL = "binstore";
static const char *BINSTORE_PATH = "/binstore";
static bool binFsMounted = false;

bool ensureFlashBinFs() {
    if (binFsMounted) return true;
    // formatOnFail=true: first boot after flashing, the partition is blank.
    binFsMounted = LittleFS.begin(true, BINSTORE_PATH, 5, BINSTORE_LABEL);
    if (!binFsMounted) { launcherConsolePrintln("Failed to mount binstore (flash .bin storage)"); }
    return binFsMounted;
}

static bool endsWithBin(const String &name) {
    String upper = name;
    upper.toUpperCase();
    return upper.endsWith(".BIN");
}

// Walks every subfolder of binstore (not just its root) looking for .bin
// files, since uploads can land inside a folder (e.g. the WebUI's own
// "Firmware" folder it may create). Each match is labelled with its path
// relative to binstore's root so files with the same name in different
// folders are still distinguishable.
static void scanBinsRecursive(const String &dirPath, std::vector<Option> &out, String &fileToUseLocal) {
    File dir = LittleFS.open(dirPath);
    if (!dir || !dir.isDirectory()) return;

    File entry = dir.openNextFile();
    while (entry) {
        String fullPath = String(entry.name());
        if (!fullPath.startsWith("/")) fullPath = "/" + fullPath;
        bool isDir = entry.isDirectory();
        entry.close();

        if (isDir) {
            scanBinsRecursive(fullPath, out, fileToUseLocal);
        } else {
            String nameOnly = fullPath.substring(fullPath.lastIndexOf('/') + 1);
            if (endsWithBin(nameOnly)) {
                String label = fullPath.startsWith("/") ? fullPath.substring(1) : fullPath;
                out.push_back(
                    {label, [fullPath, &fileToUseLocal]() { fileToUseLocal = fullPath; }});
            }
        }
        entry = dir.openNextFile();
    }
    dir.close();
}

String loopFlashBin(bool filePicker) {
    if (!ensureFlashBinFs()) {
        displayError("Flash storage not found or not formatted");
        return "";
    }

    while (true) {
        std::vector<Option> binOptions;
        String fileToUseLocal = "";

        scanBinsRecursive("/", binOptions, fileToUseLocal);
        std::sort(binOptions.begin(), binOptions.end(), sortList);

        if (binOptions.empty()) {
            displayError("No .bin in flash storage. Upload via WebUI first.");
            return "";
        }
        binOptions.push_back({"> Back", [&fileToUseLocal]() { fileToUseLocal = ""; }, ALCOLOR});

        // loopOptions runs the chosen option's lambda itself (setting fileToUseLocal)
        // before returning its index.
        int index = loopOptions(binOptions);
        if (index < 0 || binOptions[index].label == "> Back") return "";
        if (fileToUseLocal.isEmpty()) return "";

        if (filePicker) return fileToUseLocal;

        int selected = 100;
        std::vector<Option> fileOptions = {
            {"Install", [&]() { selected = 0; }},
            {"Delete",  [&]() { selected = 1; }},
            {"Back",    [&]() { selected = 9; }},
        };
        loopOptions(fileOptions);
        if (selected == 0) {
            File file = LittleFS.open(fileToUseLocal);
            updateFromFile(file, fileToUseLocal); // reboots on success; only returns on failure
        } else if (selected == 1) {
            bool doDelete = false;
            std::vector<Option> confirmOptions = {
                {"No, cancel",      [&]() { doDelete = false; }},
                {"Yes, delete it",  [&]() { doDelete = true; } },
            };
            loopOptions(confirmOptions);
            if (doDelete) LittleFS.remove(fileToUseLocal);
        }
        // selected == 9 (Back) or anything else: loop back to the file list.
    }
}
