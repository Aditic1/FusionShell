#ifndef FUSIONSHELL_H
#define FUSIONSHELL_H

#include "job_control.h"
#include <string>

class FusionShell {
private:
    bool running;
    JobControl job_control;

    void setup_signal_handlers();

public:
    FusionShell();
    void run();
};

#endif // FUSIONSHELL_H
