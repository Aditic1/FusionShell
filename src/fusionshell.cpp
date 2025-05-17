#include "fusionshell.h"
#include "parser.h"
#include "executor.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static FusionShell* shell_instance = nullptr;

FusionShell::FusionShell() : running(true), job_control() {
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
        while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
            if (shell_instance) {
                shell_instance->job_control.handle_child_signal(pid, status);
            }
        }
    });
}

void FusionShell::run() {
    std::string input;

    while (running) {
        job_control.restore_terminal_control();
        std::cout << "fusionshell> ";
        std::cout.flush();

        if (!std::getline(std::cin, input)) {
            break;
        }

        if (input.empty()) {
            continue;
        }

        auto parsed = parse_command(input);
        if (parsed.tokens.empty()) {
            continue;
        }

        if (parsed.tokens[0] == "exit") {
            running = false;
        } else if (parsed.tokens[0] == "jobs") {
            job_control.print_jobs();
        } else if (parsed.tokens[0] == "fg" && parsed.tokens.size() > 1) {
            int job_id;
            std::stringstream ss(parsed.tokens[1]);
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
        } else if (parsed.tokens[0] == "bg" && parsed.tokens.size() > 1) {
            int job_id;
            std::stringstream ss(parsed.tokens[1]);
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
