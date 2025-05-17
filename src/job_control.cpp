#include "job_control.h"
#include <iostream>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <sys/wait.h>

Job::Job(pid_t p, int id, const std::string& cmd, bool bg)
    : pid(p), job_id(id), command(cmd), is_background(bg), is_stopped(false) {}

JobControl::JobControl() : next_job_id(1), shell_pgid(getpid()) {
    initialize_shell_pgid();
}

void JobControl::initialize_shell_pgid() {
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void JobControl::add_job(pid_t pid, const std::string& command, bool is_background) {
    Job job(pid, next_job_id++, command, is_background);
    jobs.push_back(job);
    std::cout << "[" << job.job_id << "] " << pid << " " << command << (is_background ? " &\n" : "\n");
}

void JobControl::remove_job(pid_t pid) {
    auto it = std::remove_if(jobs.begin(), jobs.end(),
                             [pid](const Job& j) { return j.pid == pid; });
    if (it != jobs.end()) {
        std::cerr << "Removing job PID " << pid << " job_id " << it->job_id << "\n";
        jobs.erase(it, jobs.end());
    }
}

Job* JobControl::find_job_by_pid(pid_t pid) {
    for (auto& job : jobs) {
        if (job.pid == pid) {
            return &job;
        }
    }
    return nullptr;
}

Job* JobControl::find_job_by_id(int job_id) {
    for (auto& job : jobs) {
        if (job.job_id == job_id) {
            return &job;
        }
    }
    return nullptr;
}

void JobControl::print_jobs() const {
    for (const auto& job : jobs) {
        std::cout << "[" << job.job_id << "] " << job.pid << " "
                  << (job.is_stopped ? "Stopped" : "Running") << " "
                  << job.command << (job.is_background && !job.is_stopped ? " &" : "") << "\n";
    }
}

void JobControl::set_foreground(pid_t pid) {
    Job* job = find_job_by_pid(pid);
    if (job) {
        job->is_background = false;
        job->is_stopped = false;
        tcsetpgrp(STDIN_FILENO, pid);
        kill(-pid, SIGCONT);
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WIFSTOPPED(status)) {
            job->is_stopped = true;
            tcsetpgrp(STDIN_FILENO, shell_pgid);
            std::cout << "[" << job->job_id << "] Stopped " << job->command << "\n";
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            remove_job(pid);
            tcsetpgrp(STDIN_FILENO, shell_pgid);
        }
    }
}

void JobControl::handle_child_signal(pid_t pid, int status) {
    Job* job = find_job_by_pid(pid);
    if (!job) {
        std::cerr << "No job found for PID " << pid << "\n";
        return;
    }
    std::cerr << "SIGCHLD PID " << pid << " stopped: " << WIFSTOPPED(status) << " exited: " << WIFEXITED(status) << "\n";
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(pid);
        if (!job->is_background) {
            tcsetpgrp(STDIN_FILENO, shell_pgid);
        }
    } else if (WIFSTOPPED(status)) {
        job->is_stopped = true;
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        std::cout << "[" << job->job_id << "] Stopped " << job->command << "\n";
    }
}

void JobControl::restore_terminal_control() {
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}
