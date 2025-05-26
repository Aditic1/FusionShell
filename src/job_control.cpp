#include "job_control.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

JobControl::JobControl() : next_job_id(1), shell_pgid(getpid()) {}

void JobControl::add_job(pid_t pid, const std::string& command) {
    Job job;
    job.pid = pid;
    job.job_id = next_job_id++;
    job.command = command;
    job.is_background = false;
    job.is_stopped = false;
    jobs[pid] = job;
}

void JobControl::remove_job(pid_t pid) {
    auto it = jobs.find(pid);
    if (it != jobs.end()) {
        jobs.erase(it);
    } else {
        std::cerr << "No job found for PID " << pid << "\n";
    }
}

void JobControl::print_jobs() {
    for (const auto& pair : jobs) {
        const Job& job = pair.second;
        std::cout << "[" << job.job_id << "] " << job.pid << " ";
        if (job.is_stopped) {
            std::cout << "Stopped ";
        } else if (job.is_background) {
            std::cout << "Running ";
        }
        std::cout << job.command << (job.is_background ? " &" : "") << "\n";
    }
}

void JobControl::set_foreground(pid_t pid) {
    auto it = jobs.find(pid);
    if (it != jobs.end()) {
        Job& job = it->second;
        job.is_background = false;
        job.is_stopped = false;
        tcsetpgrp(STDIN_FILENO, pid);
        std::cout << "Setting terminal to PID " << pid << "\n";
        kill(-pid, SIGCONT);
    }
}

void JobControl::handle_child_signal(pid_t pid, int status) {
    auto it = jobs.find(pid);
    if (it == jobs.end()) {
        return;
    }

    Job& job = it->second;
    std::cout << "SIGCHLD PID " << pid << " stopped: " << WIFSTOPPED(status)
              << " exited: " << WIFEXITED(status)
              << " signaled: " << WIFSIGNALED(status)
              << " continued: " << WIFCONTINUED(status) << "\n";

    if (WIFEXITED(status)) {
        std::cout << "Job exited with status " << WEXITSTATUS(status) << "\n";
        remove_job(pid);
    } else if (WIFSIGNALED(status)) {
        std::cout << "Job terminated by signal " << WTERMSIG(status) << "\n";
        remove_job(pid);
    } else if (WIFSTOPPED(status)) {
        job.is_stopped = true;
        std::cout << "[" << job.job_id << "] Stopped " << job.command << "\n";
    } else if (WIFCONTINUED(status)) {
        job.is_stopped = false;
        std::cout << "[" << job.job_id << "] Continued " << job.command << "\n";
    }
}

Job* JobControl::find_job_by_id(int job_id) {
    for (auto& pair : jobs) {
        if (pair.second.job_id == job_id) {
            return &pair.second;
        }
    }
    return nullptr;
}

void JobControl::restore_terminal_control() {
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    std::cout << "Restoring terminal to shell PGID " << shell_pgid << "\n";
}
