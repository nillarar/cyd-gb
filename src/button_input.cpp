#include "button_input.h"
#include "touch_input.h"
#include "hw_config.h"
#include "JoystickHandle.h"
#include <Arduino.h>
#include <Wire.h>

static volatile uint16_t cur_btns = 0;
static bool button_board_present = false;

// Deadzone thresholds around center (~128). Adjust if too sensitive or stiff.
#define STICK_LOW_THRESHOLD  60
#define STICK_HIGH_THRESHOLD 195

static uint8_t read_reg(uint8_t reg) {
    Wire.beginTransmission(JOYSTICK_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) return 0xFF;
    
    Wire.requestFrom((uint8_t)JOYSTICK_I2C_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

static bool is_pressed(uint8_t reg) {
    uint8_t status = read_reg(reg);
    return (status != NONE_PRESS && status != 0xFF);
}

void button_init() {
    Wire.begin(BUTTON_I2C_SDA, BUTTON_I2C_SCL);
    Wire.setClock(100000);
    Wire.beginTransmission(JOYSTICK_I2C_ADDR);
    button_board_present = (Wire.endTransmission() == 0);
    Serial.printf("[BUTTON] Nulllab Joystick (0x5A) %s\n", button_board_present ? "ready" : "not detected; touch only");
}

static uint16_t read_nulllab_buttons() {
    uint16_t buttons = 0;

    // 1. Read Joystick X/Y as D-Pad
    uint8_t x = read_reg(JOYSTICK_LEFT_X_REG);
    uint8_t y = read_reg(JOYSTICK_LEFT_Y_REG);

    // Note: If Up/Down or Left/Right feel inverted, swap the '<' and '>' signs below
    if (y != 0xFF) {
        if (y < STICK_LOW_THRESHOLD)  buttons |= GB_BTN_DOWN;
        if (y > STICK_HIGH_THRESHOLD) buttons |= GB_BTN_UP;
    }
    if (x != 0xFF) {
        if (x < STICK_LOW_THRESHOLD)  buttons |= GB_BTN_LEFT;
        if (x > STICK_HIGH_THRESHOLD) buttons |= GB_BTN_RIGHT;
    }

    // 2. Read Face Buttons (Mapped to A, B, Start, and Select/Menu)
    // Change the GB_BTN_* assignments below to match your preferred button layout
    if (is_pressed(BUTTON_A_REG))  buttons |= GB_BTN_SELECT;
    if (is_pressed(BUTTON_B_REG))  buttons |= GB_BTN_B;
    if (is_pressed(BUTTON_C_REG))  buttons |= GB_BTN_A;
    if (is_pressed(BUTTON_D_REG))  buttons |= GB_BTN_START; // Change to GB_BTN_MENU if defined in button_input.h
    // if (is_pressed(BUTTON_OK_REG)) buttons |= GB_BTN_SELECT; // Optional: stick click

    return buttons;
}

void button_update() {
    cur_btns = button_board_present ? read_nulllab_buttons() : 0;
}

uint16_t button_get_buttons() {
    return cur_btns;
}
