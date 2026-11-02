# Lilygo TTGO T-Display (plain ESP32, no SD slot)

Chip: ESP32 (not S3), no PSRAM. Ported from this board's existing
pin mapping in a separate Bruce firmware fork, cross-checked against the
Launcher HAL.

## Display

ST7789, 135x240, hardware SPI, no touch.

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
Apps install straight to internal flash (OTA partitions / LittleFS) through
the Online Launcher (WiFi download), the WebUI uploader, or USB Mass
Storage — the SD card was always just one transport for getting a `.bin`
onto the device, never where apps actually run from, so no other firmware
logic needed to change for this.

## Free pins

GPIO17 is unused by this board definition.

## Flash size

16MB (WROVER variant), confirmed by the board owner —
`support_files/custom_16Mb.csv`, `board_upload.flash_size = 16MB`,
`board_upload.maximum_size = 16777216`.

## Not yet verified on hardware

This port hasn't been flashed and walked through the golden path (step 7
of `boards/NEW_BOARD_GUIDE.md`) yet — button feel (debounce/hold timing),
display rotation/offsets, and the GPIO14 battery-enable guess should all
be confirmed on real hardware and this file updated accordingly.
