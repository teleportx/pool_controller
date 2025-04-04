#ifndef HARDWARE_MODE_H
#define HARDWARE_MODE_H

#include <TaskSchedulerDeclarations.h>

namespace mode {
    enum Mode {
        OFF = 0,
        FILTERING = 1,
        HEATING = 2,
        MAINTAINING = 3
    };

    constexpr unsigned int cooling_time = 5 * 60;
    constexpr double maintain_delta_temperature = 0.5;

    extern Mode mode;

    extern unsigned int start_working_time, duration, disable_heater_time;
    extern double pointer_temperature;

    void setup(Scheduler runner);

    void set_off();
    void set_filtering(unsigned int duration);
    void set_heating(double pointer_temperature);
    void set_maintaining(double pointer_temperature, unsigned int duration_value);

    void handle();
}

#endif //HARDWARE_MODE_H
