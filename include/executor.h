#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"
#include "job_control.h"

void execute_command(const ParsedCommand& cmd, bool& running, JobControl& job_control);

#endif // EXECUTOR_H
