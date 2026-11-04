# Lilygo TTGO T-Display (plain ESP32, no SD slot)

Chip: ESP32 (not S3), no PSRAM. Ported from this board's existing
pin mapping in a separate Bruce firmware fork, cross-checked against the
Launcher HAL.

## Display

ST7789, 135x240, hardware SPI, no touch. This is the same physical panel
as `m5stack-cardputer`, so its offsets apply here too: `TFT_IPS=1`,
`TFT_COL_OFS1=52` / `TFT_ROW_OFS1=40`, `TFT_COL_OFS2=53` / `TFT_ROW_OFS2=40`
(an initial port without these mis-rendered to just the top-left corner of
the panel, confirmed on hardware).

| Signal | GPIO |
|--------|------|
| MOSI   | 19   |
| SCLK   | 18   |
| CS     | 5    |
| DC     | 16   |
| RST    | 23   |
| BL     | 4    |

## Buttons

Only 2 physical buttons exist on this PCB:
- **BOOT** — GPIO0 (also the flashing-mode strapping pin, has a board pull-up)
- **KEY** — GPIO35 (input-only pin, no internal pull resistor available on
  this chip; relies on the board's own external pull-up)

Mapped via `HAS_2_BUTTONS` / `BUTTONS_IDF_COMPONENT`: BTN1 (GPIO0) short
click = Next, double-click/hold = Sel; BTN2 (GPIO35) short click = Prev,
double-click/hold = Esc.

## Battery

Resistive divider into GPIO34 (`ANALOG_BAT_PIN`), no gauge/PMIC chip —
uses the shared weak `getBattery()`/`isCharging()` defaults. GPIO14 is
driven HIGH at boot on the off chance this PCB revision gates the divider
behind it (harmless no-op on revisions that don't).

## SD card -> flash, via the project's own SD_VIA_LITTLEFS mechanism

**This board has no SD card slot at all.** Rather than bolting on a
separate, custom "flash file manager" next to the real SD one, this uses
a mechanism the Launcher project already ships for exactly this situation
(`src/sd_functions.cpp`, `src/webInterface.cpp`, `include/globals.h`):

- `-DSDM=LittleFS` — `SDM` is the one object every file-manager and
  install function in this codebase already goes through
  (`loopSD`/`readFs`/`updateFromSD`/`deleteFromSd`/`renameFile`/
  `copyFile`/`pasteFile`/`createFolder`, the WebUI's whole "Files" tab).
  Normally it's `#define`d to the real `SD` object; this board defines
  it to `LittleFS` instead before that default takes hold (see the
  `#if !defined(SDM)` guard in `globals.h`).
- `-DSD_VIA_LITTLEFS=1` — makes `setupSdCard()` just mount the
  `flashfs` partition (`LittleFS.begin(true, "/sdcard", 10, "flashfs")`)
  instead of touching real SD/SPI hardware.
- `support_files/custom_16Mb_ttgo.csv` reserves that partition,
  `flashfs` (LittleFS, 4MB @ offset `0x1A0000`) — the label has to be
  exactly `flashfs`, it's what `setupSdCard()` looks for.

The upshot: the **same "SD" menu icon** every other board has — browse
folders, rename, copy/paste, create folders, delete, pick a `.bin` to
install — works completely unchanged here, just backed by internal flash
instead of a card. No separate menu icon, no separate file-manager code,
no separate install path to keep in sync with upstream. The WebUI's
"Files" tab, upload, and OTA install all work the same way too, since
they all go through `SDM`/`setupSdCard()` already.

4MB for `flashfs` is a deliberate trade-off against the ~10.4MB left
free for the Partition Manager to actually install apps into: a bigger
`flashfs` means less room for installed firmwares (each full install +
its data partition is typically ~4MB), so 4MB is enough to stage one
firmware image at a time (upload -> install -> delete -> upload the
next) while leaving room for 2+ installed, switchable firmwares. The
Partition Manager's resize (<->) button in the WebUI can resize
`flashfs` and any installed app's data partition (e.g. a too-small
default `spiffs`) against each other after the fact, without rebuilding.

The device's "backup to SD" feature (`/bkp`, `src/backup_manager.cpp`)
also goes entirely through `SDM`, so it works here too, backed by
`flashfs` like everything else — no gap left on this board.

## Free pins

GPIO17 is unused by this board definition.

## Flash size

16MB (WROVER variant), confirmed by the board owner —
`support_files/custom_16Mb_ttgo.csv`, `board_upload.flash_size = 16MB`,
`board_upload.maximum_size = 16777216`.

## Not yet verified on hardware

Display offsets were wrong on the first hardware test (fixed above, see
Display section). An earlier, custom flash-storage implementation (a
separate "BIN" menu/partition, since replaced by SD_VIA_LITTLEFS above)
ran into a real issue worth remembering: installing a firmware without
enough free space left the running firmware's partition-table view stale
until a manual reboot (install likely commits a partition-table change
to reserve space before writing the binary, so a failure partway through
still leaves that change applied). Always check the Partition Manager's
free space before installing is the safe habit; this hasn't been
specifically re-tested against the SD_VIA_LITTLEFS path since switching
to it, since it is the same underlying install codepath as the SD icon.
Also still unconfirmed on real hardware: button feel (debounce/hold
timing), and whether the GPIO14 battery-enable guess is actually needed
on this PCB revision.
