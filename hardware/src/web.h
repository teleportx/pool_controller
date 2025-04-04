#ifndef HARDWARE_WEB_H
#define HARDWARE_WEB_H

#include <TaskSchedulerDeclarations.h>

namespace web {
    constexpr unsigned int api_timeout = 1000; // API timeout in ms

    void setup(Scheduler &runner);
}

#endif //HARDWARE_WEB_H
