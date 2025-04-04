#ifndef HARDWARE_HARDWARE_H
#define HARDWARE_HARDWARE_H

#include <TaskSchedulerDeclarations.h>
#include <mutex>

class Status {
    bool last = false, now = false;
    std::mutex mtx;
public:

    bool changed();
    void set_now(bool value);
    bool get_now() const;
    bool get_last() const;
};


namespace hardware {
    constexpr double critical_temperature = 36;

    extern Status pump_status, heater_status;
    extern double temperature, currency;

    void setup(Scheduler &runner);
}

#endif //HARDWARE_HARDWARE_H
