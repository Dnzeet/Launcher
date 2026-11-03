#include "flash_bin_store.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/buttons.h"
#include "idf/launcher_platform.h"
#include "powerSave.h"
#include <interface.h>

// Only 2 physical buttons on this board: BOOT (GPIO0) and KEY (GPIO35).
// BTN1 (GPIO0) short click -> Next, double click/hold -> Sel
// BTN2 (GPIO35) short click -> Prev, double click/hold -> Esc
#define BTN1 0
#define BTN2 35

static DeviceButtons buttonsCfg() {
    DeviceButtons cfg{BTN1, BTN2};
    // GPIO35 is input-only and has no internal pull resistor on the ESP32;
    // the T-Display board itself carries an external pull-up for it.
    cfg.pullup = false;
    return cfg;
}

/***************************************************************************************
** Function:    _setup_gpio()
** Location:    main.cpp
** Description: initial setup for the device
***************************************************************************************/
void _setup_gpio() {
    // Some T-Display PCB revisions gate the battery divider behind this pin;
    // harmless to drive high on revisions that don't need it.
    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH);

    hal_buttons_init_2(buttonsCfg(), 600);
}

/***************************************************************************************
** Function:    _post_setup_gpio()
** Location:    main.cpp
** Description: second stage gpio setup, run after TFT and before SD card initialization
***************************************************************************************/
void _post_setup_gpio() {
    hal_bright_attach(TFT_BL);
    // Mount binstore now, at boot -- so it's already ready whether the WebUI
    // Files tab or the on-device "BIN" menu is the first thing to use it.
    ensureFlashBinFs();
}

/***************************************************************************************
** Function: _late_setup_gpio()
** Location: main.cpp
** Description: third stage gpio, run befor bootscreen animation
***************************************************************************************/
void _late_setup_gpio() {}

// getBattery() / isCharging(): not overridden here -- the shared weak
// defaults in src/mykeyboard.cpp already read ANALOG_BAT_PIN and estimate
// charge state from it, which is exactly this board's setup (plain resistive
// divider, no gauge/PMIC chip).

/*********************************************************************
** Function: setBrightness
** location: settings.cpp
** set brightness value
**********************************************************************/
void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

/*********************************************************************
** Function: InputHandler
** Handles the variables PrevPress, NextPress, SelPress, AnyKeyPress and EscPress
**********************************************************************/
void InputHandler(void) {
    checkPowerSaveTime();
    hal_buttons_poll_2();
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN1, LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: reboot
** Handles reboot process for devices
**********************************************************************/
void reboot() { ESP.restart(); }
