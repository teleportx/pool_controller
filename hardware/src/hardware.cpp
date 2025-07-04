#include <OneWire.h>
#include <ACS712.h>

#include "hardware.h"
#include "config.h"
#include "mode.h"

bool Status::changed() {
    if (last != now) {
        last = now;
        return true;
    }
    return false;
}

void Status::set_now(bool value) {
    mtx.lock();
    now = value;
    mtx.unlock();
}

bool Status::get_now() const {
    return now;
}

bool Status::get_last() const {
    return last;
}

namespace hardware {
    Status pump_status, heater_status;

    OneWire temperature_sensor(PIN::TEMPERATURE_SENSOR);
    double temperature = -177, currency;

    ACS712 ACS(PIN::CURRENCY_SENSOR, 3.3, 4095, 100);

    void handle_relays();
    void handle_sensors();

    Task handle_relays_task(2500, TASK_FOREVER, &handle_relays);
    Task handle_sensors_task(5000, TASK_FOREVER, &handle_sensors);

    void setup(Scheduler &runner) {
        ACS.autoMidPoint();
        Serial.print("MidPoint: ");
        Serial.print(ACS.getMidPoint());
        Serial.print(". Noise mV: ");
        Serial.println(ACS.getNoisemV());

        runner.addTask(handle_relays_task);
        runner.addTask(handle_sensors_task);

        handle_relays_task.enable();
        handle_sensors_task.enable();
    }

    void handle_relays() {
        if (pump_status.changed()) {
            digitalWrite(PIN::PUMP_RELAY, pump_status.get_last());

            if (not pump_status.get_last()) {
                Serial.println("Disabling heater because pump not working.");
                digitalWrite(PIN::HEATER_RELAY, LOW);
                heater_status.set_now(false);
            }
        }

        if ((temperature <= critical_temperature or heater_status.get_last()) and heater_status.changed()) {
            digitalWrite(PIN::HEATER_RELAY, heater_status.get_last());

            if (heater_status.get_last()) {
                Serial.println("Enabling pump because heater working.");
                digitalWrite(PIN::PUMP_RELAY, HIGH);
                pump_status.set_now(true);
            }
        }
    }

    void handle_sensors() {
        byte ds_data[2];

        temperature_sensor.reset();
        temperature_sensor.write(0xCC); // Send command: skip address find
        temperature_sensor.write(0x44); // Send command: register temperature

        delay(250); // Waiting for registering

        temperature_sensor.reset();
        temperature_sensor.write(0xCC);
        temperature_sensor.write(0xBE); // Send command: send me temperature

        ds_data[0] = temperature_sensor.read(); // Read low byte
        ds_data[1] = temperature_sensor.read(); // Read highest byte

        double now_temperature = ((ds_data[1] << 8) | ds_data[0]) * 0.0625;
        if (abs(temperature - now_temperature) < max_temperature_read_delta or temperature < -170) {
            temperature = now_temperature;
            Serial.println("Temperature: " + String(temperature));

        } else {
            Serial.println("Failed read temperature difference too big.");
        }

        currency = ACS.mA_AC_sampling() / 1000;
        Serial.println("Currency: " + String(currency * 1000) + " mA");
        Serial.println(analogRead(PIN::CURRENCY_SENSOR));

        // High temperature protection
        if (temperature > critical_temperature) {
            Serial.println("Critical temperature.");
            mode::set_off();
        }
    }
}





