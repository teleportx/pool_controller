#include <mutex>

#include "mode.h"
#include "hardware.h"
#include "ntp.h"

namespace mode {
    Mode mode(OFF);
    std::mutex mtx;

    unsigned int start_working_time = 0, duration = 0, disable_heater_time = 0;
    double pointer_temperature = 35;

    void handle();
    Task handle_task(2500, TASK_FOREVER, &handle);

    void setup(Scheduler &runner) {
        runner.addTask(handle_task);
        handle_task.enable();
    }

    void set_off() {
        mtx.lock();

        mode = Mode(OFF);
        Serial.println("Set mode OFF.");

        hardware::pump_status.set_now(false);
        hardware::heater_status.set_now(false);

        mtx.unlock();
    }

    void set_filtering(unsigned int duration_value) {
        mtx.lock();

        mode = Mode(FILTERING);
        Serial.println("Set mode FILTERING.");

        start_working_time = time_client.getEpochTime();
        duration = duration_value;

        hardware::pump_status.set_now(true);
        hardware::heater_status.set_now(false);

        mtx.unlock();
    }

    void set_heating(double pointer_temperature_value) {
        if (pointer_temperature_value > hardware::critical_temperature) return;
        mtx.lock();

        mode = Mode(HEATING);
        Serial.println("Set mode HEATING.");

        start_working_time = time_client.getEpochTime();
        pointer_temperature = pointer_temperature_value;

        if ((hardware::temperature + maintain_delta_temperature) < pointer_temperature) {
            hardware::pump_status.set_now(true);
            hardware::heater_status.set_now(true);
        }

        mtx.unlock();
    }

    void set_maintaining(double pointer_temperature_value, unsigned int duration_value) {
        if (pointer_temperature_value > hardware::critical_temperature) return;
        set_heating(pointer_temperature_value);

        mtx.lock();
        mode = Mode(MAINTAINING);
        Serial.println("Set mode MAINTAINING.");
        duration = duration_value;
        mtx.unlock();
    }

    void handle() {
        if (mode == Mode(FILTERING)) {
            if ((time_client.getEpochTime() - start_working_time) >= duration) {
                Serial.println("Filtering: time's up. Disabling pump.");
                set_off();
            }

        } else if (mode == Mode(HEATING)) {
            if (hardware::temperature >= pointer_temperature) {
                Serial.println("Heating: pointer_temperature reached, start cooling heater. Disabling heater.");
                set_filtering(cooling_time); // Cooling heater after work}
            }

        } else if (mode == Mode(MAINTAINING)) {
            if ((time_client.getEpochTime() - start_working_time) >= duration) {
                if (hardware::heater_status.get_last()) {
                    Serial.println("Maintain: time's up. Start cooling because heater was running. Disabling heater.");
                    set_filtering(cooling_time); // Cooling heater after work

                } else {
                    Serial.println("Maintain: time's up.");
                    set_off();
                }

            } else if (hardware::temperature + maintain_delta_temperature < pointer_temperature) {
                Serial.println("Maintain: low temperature, start heating.");
                hardware::pump_status.set_now(true);
                hardware::heater_status.set_now(true);
                disable_heater_time = 0;

            } else if (hardware::temperature > pointer_temperature and disable_heater_time == 0) {
                Serial.println("Maintain: pointer_temperature reached, start cooling heater. Disabling heater.");
                hardware::pump_status.set_now(true);
                hardware::heater_status.set_now(false);
                disable_heater_time = time_client.getEpochTime();

            } else if (disable_heater_time != 0) { // Cooling heater after work
                if ((time_client.getEpochTime() - disable_heater_time) >= cooling_time) {
                    Serial.println("Maintain: cooling time's up. Disabling pump.");
                    hardware::pump_status.set_now(false);
                    hardware::heater_status.set_now(false);
                    disable_heater_time = 0;
                }
            }
        }
    }
}
