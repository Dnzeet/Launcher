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

## SD card

**This board has no SD card slot at all.** `DISABLE_SDCARD_ICON=1` is set
and every SD pin is `-1` — no SD driver is compiled in, matching how
`lilygo-t-watch-s3` and `seeedstudio-sensecap` (also SD-less) are set up.

## On-device flash storage for .bin files ("BIN" menu / binstore)

The main menu's "SD" icon (browse files on-screen -> pick a `.bin` -> Install)
has no flash-storage equivalent built into the Launcher upstream, so this
board adds one:

- `support_files/custom_16Mb_ttgo.csv` reserves an extra partition,
  `binstore` (LittleFS, 4MB @ offset `0x1A0000`), on top of the normal
  `custom_16Mb.csv` layout. The ~10MB left over is still free for the
  Partition Manager to carve up for installed apps as usual.
- `flash_bin_store.h`/`.cpp` (board-local, only compiled for this board)
  mounts that partition at `/binstore` and provides an on-screen file list
  mirroring `loopSD()` (pick a `.bin` -> Install/Delete), reusing the exact
  same install logic as the SD path via the new shared `updateFromFile()`
  (split out of `updateFromSD()` in `src/sd_functions.cpp`/`.h` — a
  filesystem-agnostic version of the same function, used by every board,
  behavior unchanged for everyone else).
- The main menu gets a new **"BIN"** icon in place of "SD"
  (`src/main.cpp`, guarded by `#if defined(HAS_FLASH_BIN_STORE)`, so it
  only exists on this board).
- To get a `.bin` onto the device in the first place (no SD card to copy it
  to): the WebUI's plain file upload, which normally always targets the SD
  card, is redirected to `/binstore` for this board
  (`src/webInterface.cpp`, same build flag guard). So the flow is: connect
  to the device's WebUI once over WiFi and upload the `.bin`, then install
  it from the device screen any time after — no WiFi/SD needed for the
  install step itself.
- "OTA" (Online Installer) and plain "WUI" installs still work exactly as
  before and are unaffected — "BIN" is specifically for sideloading a
  `.bin` that isn't in the online catalog (e.g. a custom-built firmware).

This whole feature is additive and gated behind `HAS_FLASH_BIN_STORE`
(only defined for this board), so it should have zero effect on any other
board's build or behavior.

## Free pins

GPIO17 is unused by this board definition.

## Flash size

16MB (WROVER variant), confirmed by the board owner —
`support_files/custom_16Mb.csv`, `board_upload.flash_size = 16MB`,
`board_upload.maximum_size = 16777216`.

## Not yet verified on hardware

Display offsets were wrong on the first hardware test (fixed above, see
Display section). Still unconfirmed on real hardware: button feel
(debounce/hold timing), whether the GPIO14 battery-enable guess is
actually needed on this PCB revision, and the entire "BIN" / binstore
flash-storage feature above (new partition mount, WebUI upload redirect,
on-screen browser, install/delete) -- written against the existing SD
code paths for consistency but not yet flashed and exercised end-to-end
on real hardware.
