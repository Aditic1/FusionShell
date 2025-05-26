#ifndef FUSIONSHELL_H
#define FUSIONSHELL_H

#include "job_control.h"
#include <string>
#include <vector>

class History {
private:
    std::vector<std::string> commands;
    size_t max_size;
    size_t current_index;
    std::string history_file;

    void load_history();
    void save_history();

public:
    History();
    void add_command(const std::string& cmd);
    std::string get_prev_command();
    std::string get_next_command();
    std::string get_suggestion(const std::string& prefix);
};

class FusionShell {
private:
    bool running;
    JobControl job_control;
    History history;

    void setup_signal_handlers();
    void enable_raw_mode();
    void disable_raw_mode();
    std::string read_input();

public:
    FusionShell();
    void run();
};

#endif // FUSIONSHELL_H
