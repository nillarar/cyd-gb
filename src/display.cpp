#include "display.h"
#include "hw_config.h"
#include <Arduino.h>

TFT_eSPI tft = TFT_eSPI();

// Aspect-ratio-correct landscape scaling: 160x144 -> 266x240 centered on 320x240
#define RENDER_W 266
#define RENDER_X ((320 - RENDER_W) / 2)
static uint16_t scaled[RENDER_W];

void display_init() {
    pinMode(TFT_PIN_BL, OUTPUT);
    digitalWrite(TFT_PIN_BL, HIGH);
    tft.init();
    tft.invertDisplay(false);
    tft.setSwapBytes(true);
    // Rotation 1 or 3 = Landscape (320x240). Use 3 if you want the USB port on the opposite side.
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_PIN_BL, 0);
    ledcWrite(0, 255);
    Serial.printf("[TFT] %dx%d OK\n", tft.width(), tft.height());
}

void display_set_backlight(uint8_t level) { ledcWrite(0, level); }
void display_clear(uint16_t color) { tft.fillScreen(color); }

// Game scanline -> full 240px height, aspect-correct 266px width centered
void display_push_gb_line(uint8_t y, uint16_t* buf) {
    if (y >= GB_SCREEN_H) return;

    // Scale 160 -> 266 (~1.66x: duplicate 2 out of every 3 pixels)
    int idx = 0;
    for (int x = 0; x < GB_SCREEN_W && idx < RENDER_W; x++) {
        scaled[idx++] = buf[x];
        if ((x % 3) != 2 && idx < RENDER_W) {
            scaled[idx++] = buf[x];
        }
    }
    while (idx < RENDER_W) scaled[idx++] = buf[GB_SCREEN_W - 1];

    int y0 = y * 240 / GB_SCREEN_H;
    int y1 = (y + 1) * 240 / GB_SCREEN_H;
    if (y1 == y0) y1 = y0 + 1;

    for (int sy = y0; sy < y1 && sy < 240; sy++) {
        tft.pushImage(RENDER_X, sy, RENDER_W, 1, scaled);
    }
}

// ─── Touch controls disabled (full-screen landscape mode) ───────────────────
void display_draw_controls() {
    // Intentionally empty to hide on-screen buttons
}

void display_draw_menu_icon() {
    // Intentionally empty
}

void display_draw_pixel_logo(int x, int y, int scale) {
    static const uint8_t pixels[8] = {
        0xFF, 0x81, 0xBD, 0xA5, 0xBD, 0x99, 0x99, 0xFF
    };
    tft.fillRect(x, y, 8 * scale, 8 * scale, 0x1082);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (pixels[row] & (0x80 >> col))
                tft.fillRect(x + col * scale, y + row * scale, scale, scale, 0xBFE0);
        }
    }
}

void display_show_boot_animation() {
    tft.fillScreen(0x0842);
    display_draw_pixel_logo(128, 36, 8);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, 0x0842);
    tft.drawString("CYD-GB", 160, 126, 4);
    tft.setTextColor(0x7BEF, 0x0842);
    tft.drawString("GAME BOY / BLE HUB", 160, 152, 2);
    for (int i = 0; i < 8; ++i) {
        tft.fillRoundRect(72 + i * 23, 182, 17, 6, 2, 0xBFE0);
        delay(55);
    }
    tft.setTextColor(0xBFE0, 0x0842);
    tft.drawString("READY TO PLAY", 160, 212, 2);
    delay(350);
}
