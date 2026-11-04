#include "hal/bright/bright.h"
#include "hal/device.h"
#include "idf/launcher_platform.h"
#include "powerSave.h"
#include <Button.h>
#include <interface.h>

// Only 2 physical buttons on this board: BOOT (GPIO0, here named DW_BTN to
// match the user's familiar Bruce-fork control scheme) and KEY (GPIO35,
// UP_BTN). Ported 1:1 from that fork's interface.cpp -- same ESP32_Button
// library, same callback structure:
//   DW_BTN (GPIO0)  click = Select; hold = repeat-fire Next every 300ms
//   UP_BTN (GPIO35) click = Esc;    hold = repeat-fire Prev every 300ms
//                    double-click  = power off
// (Deliberately not a plain "click = Next/Prev": in the Bruce scheme,
// single clicks only ever produce Select/Esc; moving through the menu is
// always done by holding a button, which auto-repeats.)
#define DW_BTN 0
#define UP_BTN 35

static volatile bool nxtPress = false;
static volatile bool prvPress = false;
static volatile bool ecPress = false;
static volatile bool slPress = false;
static volatile bool holdNext = false;
static volatile bool holdPrev = false;

static void onButtonSingleClickCb1(void *, void *) { slPress = true; }
static void onButtonHoldCb1(void *, void *) { holdNext = true; }
static void onButtonDoubleClickCb1(void *, void *) {}

static void onButtonSingleClickCb2(void *, void *) { ecPress = true; }
static void onButtonHoldCb2(void *, void *) { holdPrev = true; }
static void onButtonDoubleClickCb2(void *, void *) { powerOff(); }

static Button *btn1 = nullptr;
static Button *btn2 = nullptr;

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

    pinMode(DW_BTN, INPUT_PULLUP);
    // GPIO35 is input-only and has no internal pull resistor on the ESP32;
    // the T-Display board itself carries an external pull-up for it, so no
    // INPUT_PULLUP here (it wouldn't take effect on this pin anyway).
    pinMode(UP_BTN, INPUT);

    if (btn1 == nullptr) {
        button_config_t bt1 = {
            .type = BUTTON_TYPE_GPIO,
            .long_press_time = 200,
            .short_press_time = 120,
            .gpio_button_config = {.gpio_num = DW_BTN, .active_level = 0},
        };
        btn1 = new Button(bt1);
        btn1->attachSingleClickEventCb(&onButtonSingleClickCb1, nullptr);
        btn1->attachLongPressStartEventCb(&onButtonHoldCb1, nullptr);
        btn1->attachDoubleClickEventCb(&onButtonDoubleClickCb1, nullptr);
    }

    if (btn2 == nullptr) {
        button_config_t bt2 = {
            .type = BUTTON_TYPE_GPIO,
            .long_press_time = 200,
            .short_press_time = 120,
            .gpio_button_config = {.gpio_num = UP_BTN, .active_level = 0},
        };
        btn2 = new Button(bt2);
        btn2->attachSingleClickEventCb(&onButtonSingleClickCb2, nullptr);
        btn2->attachLongPressStartEventCb(&onButtonHoldCb2, nullptr);
        btn2->attachDoubleClickEventCb(&onButtonDoubleClickCb2, nullptr);
    }
}

/***************************************************************************************
** Function:    _post_setup_gpio()
** Location:    main.cpp
** Description: second stage gpio setup, run after TFT and before SD card initialization
***************************************************************************************/
void _post_setup_gpio() { hal_bright_attach(TFT_BL); }

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

    static unsigned long tm = 0;
    static bool btn_pressed = false;
    static unsigned long holdTimer = 0;

    // Auto-scroll while a button is held: fire Next/Prev every 300ms.
    if (holdNext || holdPrev) {
        if (millis() - holdTimer > 300) {
            holdTimer = millis();
            if (holdNext) nxtPress = true;
            if (holdPrev) prvPress = true;
        }
        if (digitalRead(DW_BTN) == HIGH) holdNext = false;
        if (digitalRead(UP_BTN) == HIGH) holdPrev = false;
    }

    if (nxtPress || prvPress || ecPress || slPress) btn_pressed = true;

    if (millis() - tm > 200 || LongPress) {
        if (btn_pressed) {
            btn_pressed = false;
            tm = millis();

            if (wakeUpScreen()) {
                nxtPress = false;
                prvPress = false;
                ecPress = false;
                slPress = false;
                return;
            }

            AnyKeyPress = true;
            SelPress = slPress;
            EscPress = ecPress;
            NextPress = nxtPress;
            PrevPress = prvPress;

            nxtPress = false;
            prvPress = false;
            ecPress = false;
            slPress = false;
        }
    }
}

/*********************************************************************
** Function: powerOff
** location: mykeyboard.cpp
** Turns off the device (or try to)
**********************************************************************/
void powerOff() {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)UP_BTN, LOW);
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: reboot
** Handles reboot process for devices
**********************************************************************/
void reboot() { ESP.restart(); }
