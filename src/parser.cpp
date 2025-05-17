#include "parser.h"
#include <sstream>
#include <vector>

ParsedCommand parse_command(const std::string& input) {
    ParsedCommand result;
    result.is_background = false;

    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;

    while (ss >> token) {
        tokens.push_back(token);
    }

    if (!tokens.empty() && tokens.back() == "&") {
        result.is_background = true;
        tokens.pop_back();
    }

    result.tokens = tokens;
    return result;
}
