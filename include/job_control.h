#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <string>
#include <vector>
#include <sys/types.h>

class Job {
public:
    pid_t pid;                  // Process ID
    int job_id;                 // Job number (e.g., [1], [2])
    std::string command;        // Command string (e.g., "sleep 10")
    bool is_background;         // True if running in background
    bool is_stopped;            // True if stopped (e.g., Ctrl+Z)

    Job(pid_t p, int id, const std::string& cmd, bool bg);
};

class JobControl {
private:
    std::vector<Job> jobs;
    int next_job_id;
    pid_t shell_pgid;           // Shell's process group ID

public:
    JobControl();
    void add_job(pid_t pid, const std::string& command, bool is_background);
    void remove_job(pid_t pid);
    Job* find_job_by_pid(pid_t pid);
    Job* find_job_by_id(int job_id);
    void print_jobs() const;
    void set_foreground(pid_t pid);
    void handle_child_signal(pid_t pid, int status);
    void initialize_shell_pgid();
    void restore_terminal_control(); // New: Restore terminal to shell
};

#endif // JOB_CONTROL_H
