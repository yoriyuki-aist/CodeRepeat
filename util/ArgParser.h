#pragma once

#include <regex>
#include <set>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>

class ArgParser {
private:
    char **begin;
    char **end;
    bool empty;
public:
    ArgParser() = delete;

    ArgParser(char **begin, char **end) : begin(begin), end(end) {
        empty = begin >= end;
    }

    bool cmdOptionExists(const std::string &option) {
        return !empty && std::find(begin, end, option) != end;
    }

    std::optional<std::vector<std::string>> getCmdArgs(const std::string &option) {
        if (!empty) {
            char **found = std::find(begin, end, option);
            if (found != end) {
                char **arg_start = found + 1;
                char **arg_end = arg_start;
                while (arg_end != end && *arg_end[0] != '-') {
                    arg_end++;
                }
                if (arg_start == arg_end) {
                    throw std::invalid_argument(option + " requires one or more arguments after it.");
                }
                return std::vector<std::string>(arg_start, arg_end);
            }
        }
        return {};
    }

    std::optional<std::string> getCmdArg(const std::string &option) {
        if (!empty) {
            char **found = std::find(begin, end, option);
            if (found != end) {
                char **arg = found + 1;

                if (arg == end || *arg[0] == '-') {
                    throw std::invalid_argument(option + " requires an argument after it.");
                }

                return *arg;
            }
        }
        return {};
    }
};