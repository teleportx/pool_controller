#ifndef HARDWARE_PANEL_H
#define HARDWARE_PANEL_H

namespace panel {
    class IScreen {
    public:
        virtual void handle() = 0;
    };



    void loop();
    void setup();
}

#endif //HARDWARE_PANEL_H
