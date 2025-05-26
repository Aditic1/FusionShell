#include "fusionshell.h"
#include "parser.h"
#include "executor.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cerrno>

static FusionShell* shell_instance = nullptr;
static struct termios orig_termios;

History::History() : max_size(1000), current_index(0), history_file("/home/aditiubuntu/.fusionshell_history") {
    load_history();
}

void History::load_history() {
    std::ifstream file(history_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open history file for reading: " << history_file << "\n";
        return;
    }
    std::string line;
    while (std::getline(file, line) && commands.size() < max_size) {
        if (!line.empty()) {
            commands.push_back(line);
        }
    }
    current_index = commands.size();
}

void History::save_history() {
    std::ofstream file(history_file, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to save history to " << history_file << ": " << strerror(errno) << "\n";
        return;
    }
    for (const auto& cmd : commands) {
        file << cmd << "\n";
    }
    file.close();
    std::cerr << "History saved to " << history_file << "\n";
}

void History::add_command(const std::string& cmd) {
    if (cmd.empty() || (commands.size() > 0 && commands.back() == cmd)) {
        return;
    }
    commands.push_back(cmd);
    if (commands.size() > max_size) {
        commands.erase(commands.begin());
    }
    current_index = commands.size();
    save_history();
}

std::string History::get_prev_command() {
    if (commands.empty() || current_index == 0) {
        return "";
    }
    return commands[--current_index];
}

std::string History::get_next_command() {
    if (current_index >= commands.size()) {
        return "";
    }
    return commands[current_index++];
}

std::string History::get_suggestion(const std::string& prefix) {
    if (prefix.empty()) {
        return "";
    }
    for (const auto& cmd : commands) {
        if (cmd.size() >= prefix.size() && cmd.substr(0, prefix.size()) == prefix) {
            return cmd;
        }
    }
    return "";
}

FusionShell::FusionShell() : running(true), job_control(), history() {
    shell_instance = this;
    setpgid(0, 0);
    tcsetpgrp(STDIN_FILENO, getpid());
    setup_signal_handlers();
}

void FusionShell::setup_signal_handlers() {
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, [](int) {
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            if (shell_instance) {
                shell_instance->job_control.handle_child_signal(pid, status);
            }
        }
    });
}

void FusionShell::enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ISIG | ECHO);
    raw.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        std::cerr << "Failed to set raw mode: " << strerror(errno) << "\n";
    }
}

void FusionShell::disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        std::cerr << "Failed to restore terminal: " << strerror(errno) << "\n";
    }
}

// In fusionshell.cpp, replace the read_input function with this version
std::string FusionShell::read_input() {
    enable_raw_mode();
    std::string input;
    std::string suggestion;
    std::string displayed_suggestion;
    size_t cursor_pos = 0;

    tcflush(STDIN_FILENO, TCIFLUSH);
    std::cout << "fusionshell> ";
    std::cout.flush();

    // ANSI color codes
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_COMMAND = "\033[32m"; // Green
    const std::string COLOR_ARG = "\033[33m";    // Yellow
    const std::string COLOR_OP = "\033[31m";     // Red
    const std::string COLOR_FILE = "\033[34m";   // Blue
    const std::string COLOR_SUGGEST = "\033[90m"; // Gray for suggestions

    while (true) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            if (errno == EINTR) continue;
            disable_raw_mode();
            return "";
        }

        if (c == '\n' || c == '\r') {
            std::cout << "\n";
            break;
        } else if (c == 127 || c == '\b') { // Backspace
            if (!input.empty() && cursor_pos > 0) {
                input.erase(cursor_pos - 1, 1);
                cursor_pos--;
            }
        } else if (c == 3) { // Ctrl+C
            std::cout << "^C\nfusionshell> ";
            input.clear();
            cursor_pos = 0;
            suggestion.clear();
            displayed_suggestion.clear();
            std::cout.flush();
            continue;
        } else if (c == 27) { // Escape sequence
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) <= 0 || read(STDIN_FILENO, &seq[1], 1) <= 0) {
                continue;
            }
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up arrow
                    std::string prev = history.get_prev_command();
                    if (!prev.empty()) {
                        input = prev;
                        cursor_pos = input.size();
                    }
                } else if (seq[1] == 'B') { // Down arrow
                    std::string next = history.get_next_command();
                    input = next.empty() ? "" : next;
                    cursor_pos = input.size();
                } else if (seq[1] == 'C' && !suggestion.empty() && cursor_pos == input.size()) { // Right arrow
                    input = suggestion;
                    cursor_pos = input.size();
                }
            }
        } else if (std::isprint(c)) {
            input.insert(cursor_pos, 1, c);
            cursor_pos++;
        }

        // Parse input for syntax highlighting
        ParsedCommand parsed = parse_command(input);
        std::cout << "\r\033[Kfusionshell> ";

        // Display input with syntax highlighting
        bool first_token = true;
        bool parsing_file = false;
        char redirection_type = 0;

        for (const auto& cmd : parsed.commands) {
            for (size_t i = 0; i < cmd.tokens.size(); ++i) {
                if (i == 0 && first_token) {
                    std::cout << COLOR_COMMAND << cmd.tokens[i] << COLOR_RESET;
                    first_token = false;
                } else {
                    std::cout << COLOR_ARG << cmd.tokens[i] << COLOR_RESET;
                }
                if (i < cmd.tokens.size() - 1) {
                    std::cout << " ";
                }
            }
            if (!cmd.input_file.empty()) {
                std::cout << " " << COLOR_OP << "<" << COLOR_RESET << " " << COLOR_FILE << cmd.input_file << COLOR_RESET;
            }
            if (!cmd.output_file.empty()) {
                std::cout << " " << COLOR_OP << ">" << COLOR_RESET << " " << COLOR_FILE << cmd.output_file << COLOR_RESET;
            }
            if (cmd.is_background) {
                std::cout << " " << COLOR_OP << "&" << COLOR_RESET;
            }
            if (&cmd != &parsed.commands.back()) {
                std::cout << " " << COLOR_OP << "|" << COLOR_RESET;
            }
            std::cout << " ";
        }

        // Display suggestion
        suggestion = history.get_suggestion(input);
        displayed_suggestion = suggestion.empty() ? "" : suggestion.substr(cursor_pos);
        std::cout << COLOR_SUGGEST << displayed_suggestion << COLOR_RESET;

        // Move cursor to correct position
        std::cout << "\033[" << (cursor_pos + 11) << "G";
        std::cout.flush();
    }

    disable_raw_mode();
    if (!input.empty()) {
        history.add_command(input);
    }
    return input;
}
void FusionShell::run() {
    while (running) {
        job_control.restore_terminal_control();
        std::string input = read_input();

        if (input.empty()) {
            continue;
        }

        auto parsed = parse_command(input);
        if (parsed.commands.empty()) {
            continue;
        }

        bool is_background = !parsed.commands.empty() && parsed.commands.back().is_background;

        if (parsed.commands[0].tokens[0] == "exit") {
            running = false;
        } else if (parsed.commands[0].tokens[0] == "jobs") {
            job_control.print_jobs();
        } else if (parsed.commands[0].tokens[0] == "fg" && parsed.commands[0].tokens.size() > 1) {
            int job_id;
            std::stringstream ss(parsed.commands[0].tokens[1]);
            if (ss >> job_id) {
                Job* job = job_control.find_job_by_id(job_id);
                if (job) {
                    job_control.set_foreground(job->pid);
                } else {
                    std::cerr << "No such job: " << job_id << "\n";
                }
            } else {
                std::cerr << "Invalid job ID\n";
            }
        } else if (parsed.commands[0].tokens[0] == "bg" && parsed.commands[0].tokens.size() > 1) {
            int job_id;
            std::stringstream ss(parsed.commands[0].tokens[1]);
            if (ss >> job_id) {
                Job* job = job_control.find_job_by_id(job_id);
                if (job) {
                    job->is_background = true;
                    job->is_stopped = false;
                    kill(-job->pid, SIGCONT);
                    std::cout << "[" << job->job_id << "] " << job->command << " &\n";
                } else {
                    std::cerr << "No such job: " << job_id << "\n";
                }
            } else {
                std::cerr << "Invalid job ID\n";
            }
        } else {
            execute_command(parsed, running, job_control);
        }
    }
}
