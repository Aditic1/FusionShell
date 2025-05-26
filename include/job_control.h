#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <string>
#include <vector>
#include <map>

struct Job {
    pid_t pid;
    int job_id;
    std::string command;
    bool is_background;
    bool is_stopped;
};

class JobControl {
private:
    std::map<pid_t, Job> jobs;
    int next_job_id;
    pid_t shell_pgid;

public:
    JobControl();
    void add_job(pid_t pid, const std::string& command);
    void remove_job(pid_t pid);
    void print_jobs();
    void set_foreground(pid_t pid);
    void handle_child_signal(pid_t pid, int status);
    Job* find_job_by_id(int job_id);
    void restore_terminal_control();
};

#endif // JOB_CONTROL_H

