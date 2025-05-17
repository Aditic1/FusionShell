#include "executor.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

void execute_command(const ParsedCommand& parsed, bool& running, JobControl& job_control) {
    if (parsed.tokens.empty()) {
        return;
    }

    std::string command;
    for (const auto& t : parsed.tokens) {
        command += t + " ";
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Error: Fork failed\n";
        return;
    }

    if (pid == 0) {
        setpgid(0, 0);
        std::cerr << "Child PID " << getpid() << " PGID " << getpgid(0) << "\n";
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        std::vector<char*> args;
        for (const auto& token : parsed.tokens) {
            args.push_back(const_cast<char*>(token.c_str()));
        }
        args.push_back(nullptr);

        execvp(args[0], args.data());
        std::cerr << "Error: Command '" << parsed.tokens[0] << "' not found\n";
        exit(1);
    } else {
        setpgid(pid, pid);
        std::cerr << "Parent set PGID for PID " << pid << " to " << pid << "\n";
        job_control.add_job(pid, command, parsed.is_background);
        if (!parsed.is_background) {
            tcsetpgrp(STDIN_FILENO, pid);
            int status;
            waitpid(pid, &status, WUNTRACED);
            job_control.handle_child_signal(pid, status);
            job_control.restore_terminal_control();
        }
    }
}
