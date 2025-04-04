#include <Arduino.h>
#include <ESP32Encoder.h>
#include <TM1637Display.h>
#include <ArduinoOTA.h>
#include <TaskScheduler.h>

#include "config.h"
#include "hardware.h"
#include "ntp.h"
#include "mode.h"

TaskHandle_t loop2_task;
void loop2();

ESP32Encoder encoder;
TM1637Display display = TM1637Display(PIN::DISPLAY_CLK, PIN::DISPLAY_DIO);

Scheduler hardware_runner, network_runner;

void setup() {
    pinMode(PIN::TEMPERATURE_SENSOR, INPUT);
    pinMode(PIN::ACTION_BUTTON, INPUT_PULLUP);

    pinMode(PIN::PUMP_RELAY, OUTPUT);
    pinMode(PIN::HEATER_RELAY, OUTPUT);

    pinMode(PIN::ENCODER_SW, INPUT_PULLUP);

    pinMode(PIN::DISPLAY_CLK, OUTPUT);
    pinMode(PIN::DISPLAY_DIO, OUTPUT);

    Serial.begin(115200);

    WiFi.begin(wifi_ssid, wifi_password);
    ArduinoOTA.setHostname(device_name);
    ArduinoOTA.begin();
    time_client.begin();

    hardware::setup(hardware_runner);
    mode::setup(hardware_runner);

    xTaskCreatePinnedToCore(
            [](void *) {
                while (true) {
                    loop2();
                }

            }, "loop2", 10240, nullptr, 0, &loop2_task, 0);
}

void loop() {
    network_runner.execute();
    ArduinoOTA.handle();
    time_client.update();
}

void loop2() {
    hardware_runner.execute();
}