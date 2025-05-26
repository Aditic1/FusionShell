#include "parser.h"
#include <string>
#include <vector>
#include <iostream>

ParsedCommand parse_command(const std::string& input) {
    ParsedCommand result;
    std::vector<Command> commands;
    Command current_command;
    current_command.is_background = false;
    std::string token;
    bool in_token = false;
    bool parsing_file = false;
    char redirection_type = 0;

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (std::isspace(c)) {
            if (in_token) {
                if (parsing_file) {
                    if (redirection_type == '>') {
                        current_command.output_file = token;
                    } else if (redirection_type == '<') {
                        current_command.input_file = token;
                    }
                    parsing_file = false;
                    redirection_type = 0;
                } else {
                    current_command.tokens.push_back(token);
                }
                token.clear();
                in_token = false;
            }
        } else if (c == '|' || c == '>' || c == '<' || c == '&') {
            if (in_token) {
                if (parsing_file) {
                    if (redirection_type == '>') {
                        current_command.output_file = token;
                    } else if (redirection_type == '<') {
                        current_command.input_file = token;
                    }
                    parsing_file = false;
                } else {
                    current_command.tokens.push_back(token);
                }
                token.clear();
                in_token = false;
            }
            if (c == '|') {
                if (!current_command.tokens.empty()) {
                    commands.push_back(current_command);
                    current_command = Command();
                    current_command.is_background = false;
                }
            } else if (c == '>') {
                parsing_file = true;
                redirection_type = '>';
            } else if (c == '<') {
                parsing_file = true;
                redirection_type = '<';
            } else if (c == '&') {
                current_command.is_background = true;
            }
        } else {
            token += c;
            in_token = true;
        }
    }
    if (in_token) {
        if (parsing_file) {
            if (redirection_type == '>') {
                current_command.output_file = token;
            } else if (redirection_type == '<') {
                current_command.input_file = token;
            }
        } else {
            current_command.tokens.push_back(token);
        }
    }
    if (!current_command.tokens.empty() || !current_command.output_file.empty() || !current_command.input_file.empty()) {
        commands.push_back(current_command);
    }

    for (const auto& cmd : commands) {
        if (cmd.tokens.empty()) {
            std::cerr << "Invalid command\n";
            result.commands.clear();
            return result;
        }
        for (const auto& token : cmd.tokens) {
            if (token.empty() || token.find_first_of("|><&") != std::string::npos) {
                std::cerr << "Invalid token: " << token << "\n";
                result.commands.clear();
                return result;
            }
        }
    }

    result.commands = commands;
    return result;
}
