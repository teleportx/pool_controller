#include <ESP32Encoder.h>
#include <TM1637Display.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Bounce2.h>

#include "panel.h"
#include "config.h"
#include "hardware.h"
#include "mode.h"
#include "ntp.h"


namespace panel {
    ESP32Encoder encoder;
    Bounce2::Button encoder_button = Bounce2::Button();
    Bounce2::Button action_button = Bounce2::Button();
    TM1637Display display = TM1637Display(PIN::DISPLAY_CLK, PIN::DISPLAY_DIO);


    unsigned long long last_blink = 0;
    mode::Mode now_mode;
    unsigned int now_duration, now_pointer_temperature;

    bool need_blink() {
        if (esp_timer_get_time() / 1000 - last_blink >= 250) {
            if ((esp_timer_get_time() / 1000 - last_blink) >= 500)
                last_blink = esp_timer_get_time() / 1000;

            return true;
        }

        return false;
    }

    IScreen *now_screen;

    class ScreenHome : public IScreen {
    public:
        ScreenHome();
        void handle() override;
    };

    class ScreenDone : public IScreen {
        unsigned long long start;

    public:
        ScreenDone();
        void handle() override;
    };


    class ScreenSetMode : public IScreen {
    public:
        ScreenSetMode();
        void handle() override;
    };

    class ScreenSetDuration : public IScreen {
    public:
        ScreenSetDuration();
        void handle() override;
    };

    class ScreenSetPointerTemperature : public IScreen {
    public:
        ScreenSetPointerTemperature();
        void handle() override;
    };

    ScreenHome::ScreenHome() {
        encoder.setCount(0);
    }

    void ScreenHome::handle() {
        const int temperature = round(hardware::temperature);

        uint8_t display_data[] = {
                0,
                0,
                0,
                0,
        };

        int page = abs(encoder.getCount()) / 2 % 4;
        if (page == 0) {
            display_data[2] |= display.encodeDigit(temperature / 10);
            display_data[3] |= display.encodeDigit(temperature % 10);

            if (need_blink()) {
                if (mode::mode != mode::Mode(mode::OFF))
                    display_data[3] |= SEG_DP;

                if (not WiFi.isConnected())
                    display_data[0] |= SEG_F | SEG_A | SEG_B;
            }

        } else if (page == 1) {
            display_data[2] |= SEG_E | SEG_F | SEG_A | SEG_B | SEG_G | SEG_DP;
            display_data[3] |= display.encodeDigit(mode::mode);

        } else if (page == 2) {
            display.showNumberDec(int(hardware::currency));

        } else if (page == 3) {
            display_data[0] |= display.encodeDigit(time_client.getHours() / 10);
            display_data[1] |= display.encodeDigit(time_client.getHours() % 10) | SEG_DP;
            display_data[2] |= display.encodeDigit(time_client.getMinutes() / 10);
            display_data[3] |= display.encodeDigit(time_client.getMinutes() % 10);
        }

        if (page != 2)
            display.setSegments(display_data, 4, 0);

        if (encoder_button.pressed()) {
            now_screen = new ScreenSetMode();
            delete this;
        }
    }

    ScreenDone::ScreenDone() : start(esp_timer_get_time() / 1000) {}

    void ScreenDone::handle() {
        if (esp_timer_get_time() / 1000 - start >= 1000) {
            now_screen = new ScreenHome();
            delete this;
        }

        const uint8_t display_data[] = {
                SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
                SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
                SEG_C | SEG_E | SEG_G,
                SEG_A | SEG_D | SEG_E | SEG_F | SEG_G
        };

        display.setSegments(display_data, 4, 0);
    }

    ScreenSetMode::ScreenSetMode() {
        encoder.setCount(0);
        now_mode = mode::Mode(mode::OFF);
    }

    void ScreenSetMode::handle() {
        now_mode = mode::Mode((encoder.getCount() / 2 % 4 + 4) % 4);

        uint8_t display_data[] = {
                0,
                0,
                SEG_E | SEG_F | SEG_A | SEG_B | SEG_G,
                display.encodeDigit(now_mode),
        };

        display.setSegments(display_data, 4, 0);

        if (encoder_button.pressed()) {
            if (now_mode == mode::OFF) {
                mode::set_graceful_off();
                now_screen = new ScreenDone();

            } else if (now_mode == mode::FILTERING or now_mode == mode::MAINTAINING) {
                now_screen = new ScreenSetDuration();

            } else if (now_mode == mode::HEATING) {
                now_screen = new ScreenSetPointerTemperature();
            }

            delete this;
        }
    };

    ScreenSetDuration::ScreenSetDuration() {
        encoder.setCount(2);
        now_duration = 1;
    }

    void ScreenSetDuration::handle() {
        encoder.setCount(max(2ll, min(24ll, encoder.getCount())));
        now_duration = encoder.getCount() / 2;

        uint8_t display_data[] = {
                0,
                SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
                display.encodeDigit(now_duration / 10),
                display.encodeDigit(now_duration % 10),
        };

        display.setSegments(display_data, 4, 0);

        if (encoder_button.pressed()) {
            if (now_mode == mode::FILTERING) {
                mode::set_filtering(now_duration * 60 * 60);
                now_screen = new ScreenDone();

            } else if (now_mode == mode::MAINTAINING) {
                now_screen = new ScreenSetPointerTemperature();
            }

            delete this;
        }
    }

    ScreenSetPointerTemperature::ScreenSetPointerTemperature() {
        encoder.setCount(54);
        now_pointer_temperature = 27;
    }

    void ScreenSetPointerTemperature::handle() {
        encoder.setCount(max(0ll, min(70ll, encoder.getCount())));
        now_pointer_temperature = encoder.getCount() / 2;

        uint8_t display_data[] = {
                0,
                SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,
                display.encodeDigit(now_pointer_temperature / 10),
                display.encodeDigit(now_pointer_temperature % 10),
        };

        display.setSegments(display_data, 4, 0);

        if (encoder_button.pressed()) {
            if (now_mode == mode::HEATING) {
                mode::set_heating(now_pointer_temperature);
                now_screen = new ScreenDone();

            } else if (now_mode == mode::MAINTAINING) {
                mode::set_maintaining(now_pointer_temperature, now_duration * 60 * 60);
                now_screen = new ScreenDone();
            }

            delete this;
        }
    }

    void setup() {
        ESP32Encoder::useInternalWeakPullResistors = puType::up;
        encoder.attachHalfQuad(PIN::ENCODER_CLK, PIN::ENCODER_DT);
        encoder.setCount(0);

        encoder_button.attach(PIN::ENCODER_SW, INPUT_PULLUP);
        encoder_button.interval(5);
        encoder_button.setPressedState(LOW);

        action_button.attach(PIN::ACTION_BUTTON, INPUT_PULLUP);
        action_button.interval(5);
        action_button.setPressedState(LOW);

        display.clear();
        display.setBrightness(7);

        now_screen = new ScreenHome();
    }

    void loop() {
        encoder_button.update();
        action_button.update();

        if (action_button.isPressed()) {
            mode::set_off();
        }

        now_screen->handle();
    }
}
