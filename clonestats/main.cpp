#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <json/JsonStreamingParser.h>
#include "CloneParser.h"

void parse_json(std::map<unsigned long, std::string> &extensions,
                std::unordered_map<std::string, std::map<unsigned long, Occurrences>> &occurrences,
                std::ifstream &json_in);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    std::string json_file = argv[1];
    std::map<unsigned long, std::string> extensions;
    std::unordered_map<std::string, std::map<unsigned long, Occurrences>> occurrences;
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }

    parse_json(extensions, occurrences, json_in);
    json_in.close();

    std::cout << "Number of occurrences of repeated subsequences by file extension:\n";

    for (const auto &ext_entry : occurrences) {
        std::cout << "File extension: " << (ext_entry.first.empty() ? "(none)" : ext_entry.first) << "\n";

        for (const auto &size_entry : ext_entry.second) {
            std::cout << "- Size " << size_entry.first << ":\t" << size_entry.second.total << " occurrence(s) \t-\t";
            std::cout << size_entry.second.unique << " unique sequence(s)";
            std::cout << "\n";
        }
    }

    std::cout << "\nDone!\n";
}

void parse_json(std::map<unsigned long, std::string> &extensions,
                std::unordered_map<std::string, std::map<unsigned long, Occurrences>> &occurrences,
                std::ifstream &json_in) {
    JsonStreamingParser parser;
    CloneListener listener = CloneListener(extensions, occurrences);
    parser.setListener(&listener);

    char c;
    while (json_in.get(c)) {
        parser.parse(c);
    }
}

