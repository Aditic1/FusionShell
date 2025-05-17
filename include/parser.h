#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

struct ParsedCommand {
    std::vector<std::string> tokens;
    bool is_background;
};

ParsedCommand parse_command(const std::string& input);

#endif // PARSER_H
