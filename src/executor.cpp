#include "executor.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstring>

void execute_command(const ParsedCommand& cmd, bool& running, JobControl& job_control) {
    if (cmd.commands.empty()) {
        return;
    }

    std::vector<int> pipe_fds;
    std::vector<pid_t> pids;

    for (size_t i = 0; i < cmd.commands.size(); ++i) {
        int pipefd[2] = {-1, -1};
        if (i < cmd.commands.size() - 1) {
            if (pipe(pipefd) == -1) {
                std::cerr << "Pipe failed\n";
                return;
            }
            pipe_fds.push_back(pipefd[0]);
            pipe_fds.push_back(pipefd[1]);
        }

        pid_t pid = fork();
        if (pid == -1) {
            std::cerr << "Fork failed\n";
            return;
        }

        if (pid == 0) {
            if (i > 0) {
                dup2(pipe_fds[(i-1)*2], STDIN_FILENO);
                close(pipe_fds[(i-1)*2]);
                close(pipe_fds[(i-1)*2 + 1]);
            }

            if (i < cmd.commands.size() - 1) {
                dup2(pipe_fds[i*2 + 1], STDOUT_FILENO);
                close(pipe_fds[i*2]);
                close(pipe_fds[i*2 + 1]);
            }

            for (size_t j = 0; j < pipe_fds.size(); ++j) {
                close(pipe_fds[j]);
            }

            const auto& command = cmd.commands[i];
            if (!command.input_file.empty()) {
                int fd = open(command.input_file.c_str(), O_RDONLY);
                if (fd == -1) {
                    std::cerr << "Failed to open input file: " << command.input_file << "\n";
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (!command.output_file.empty()) {
                int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1) {
                    std::cerr << "Failed to open output file: " << command.output_file << "\n";
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            std::vector<char*> args;
            for (const auto& token : command.tokens) {
                args.push_back(const_cast<char*>(token.c_str()));
            }
            args.push_back(nullptr);

            execvp(args[0], args.data());
            std::cerr << "execvp failed: " << strerror(errno) << "\n";
            exit(1);
        } else {
            std::cout << "[" << (i + 1) << "] " << pid << " ";
            for (const auto& token : cmd.commands[i].tokens) {
                std::cout << token << " ";
            }
            if (!cmd.commands[i].input_file.empty()) {
                std::cout << "< " << cmd.commands[i].input_file << " ";
            }
            if (!cmd.commands[i].output_file.empty()) {
                std::cout << "> " << cmd.commands[i].output_file << " ";
            }
            if (cmd.commands[i].is_background) {
                std::cout << "& ";
            }
            std::cout << "\n";

            pids.push_back(pid);
            job_control.add_job(pid, cmd.commands[i].tokens[0]);

            if (i > 0) {
                close(pipe_fds[(i-1)*2]);
                close(pipe_fds[(i-1)*2 + 1]);
            }

            setpgid(pid, pids[0]);
            std::cout << "Parent set PGID for PID " << pid << " to " << pids[0] << "\n";
            if (i == cmd.commands.size() - 1 && !cmd.commands[i].is_background) {
                job_control.set_foreground(pid);
            }
        }
    }

    for (int fd : pipe_fds) {
        if (fd != -1) {
            close(fd);
        }
    }

    if (!cmd.commands.back().is_background) {
        for (pid_t pid : pids) {
            int status;
            waitpid(pid, &status, WUNTRACED);
            job_control.remove_job(pid);
        }
    }
}
