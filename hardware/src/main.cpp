#include <Arduino.h>
#include <ArduinoOTA.h>
#include <TaskScheduler.h>

#include "config.h"
#include "hardware.h"
#include "ntp.h"
#include "mode.h"
#include "web.h"
#include "panel.h"

TaskHandle_t loop2_task;
void loop2();



Scheduler hardware_runner, network_runner;

void setup() {
    pinMode(PIN::TEMPERATURE_SENSOR, INPUT_PULLUP);
    pinMode(PIN::ACTION_BUTTON, INPUT_PULLUP);

    pinMode(PIN::PUMP_RELAY, OUTPUT);
    pinMode(PIN::HEATER_RELAY, OUTPUT);

    pinMode(PIN::DISPLAY_CLK, OUTPUT);
    pinMode(PIN::DISPLAY_DIO, OUTPUT);

    Serial.begin(115200);

    WiFi.begin(wifi_ssid, wifi_password);
    ArduinoOTA.setHostname(device_name);
    ArduinoOTA.begin();
    time_client.begin();

    hardware::setup(hardware_runner);
    panel::setup();
    mode::setup(hardware_runner);
    web::setup(network_runner);

    xTaskCreatePinnedToCore(
            [](void *) {
                while (true) {
                    loop2();
                }

            }, "loop2", 10240, nullptr, 0, &loop2_task, 0);
}

void loop() {
    network_runner.execute();
    if (WiFi.isConnected()) {
        ArduinoOTA.handle();
        time_client.update();
    }
}

void loop2() {
    hardware_runner.execute();
    panel::loop();
}