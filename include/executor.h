#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "job_control.h"
#include "parser.h"

void execute_command(const ParsedCommand& parsed, bool& running, JobControl& job_control);

#endif // EXECUTOR_H
