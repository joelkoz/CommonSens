#ifdef M5STICK

#include "display/m5_display_bool.h"
#include <M5StickC.h>

// Custom transform that consumes a boolean value
// and displays it on the M5StickC screen
void M5DisplayBool::set_input(bool value, uint8_t input_channel = 0) override {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(5, 5);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(5);
    if (value) {
        M5.Lcd.printf("ON");
    }
    else {
        M5.Lcd.printf("OFF");
    }
}

#endif
