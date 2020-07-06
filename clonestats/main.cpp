#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <json/JsonStreamingParser.h>
#include "CloneParser.h"
#include "../util/ArgParser.h"

void parse_json(std::map<unsigned long, std::string> &extensions,
                Statistics &stats,
                std::ifstream &json_in);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];
    std::map<unsigned long, std::string> extensions;
    Statistics stats;
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }

    parse_json(extensions, stats, json_in);
    json_in.close();

    std::cout << "Number of occurrences of repeated subsequences by file extension:\n";

    const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences = stats.occurrences;

    for (const auto &ext_entry : occurrences) {
        std::cout << "File extension: " << (ext_entry.first.empty() ? "(none)" : ext_entry.first) << "\n";

        for (const auto &size_entry : ext_entry.second) {
            std::cout << "- Size " << size_entry.first << ":\t" << size_entry.second.total << " occurrence(s) \t-\t";
            std::cout << size_entry.second.unique << " unique sequence(s)";
            std::cout << "\n";
        }
    }

    std::optional<std::string> idiom_rate = args.getCmdArg("--idioms");

    if (idiom_rate) {
        std::cout << "Idioms:\n";

        std::sort(stats.repeats.begin(), stats.repeats.end(), [](RepeatDigest &e1, RepeatDigest &e2) {
            return e1.occurrences < e2.occurrences;
        });

        auto idiom_start = (unsigned) (stats.repeats.size() * (1.0 - std::stod(*idiom_rate)));

        for (unsigned i = idiom_start; i < stats.repeats.size(); i++) {
            RepeatDigest &repeat = stats.repeats[i];
            std::cout << "- '" << repeat.text << "': " << repeat.occurrences << "\n";
        }
    }

    std::cout << "\nDone!\n";
}

void parse_json(std::map<unsigned long, std::string> &extensions,
                Statistics &stats,
                std::ifstream &json_in) {
    JsonStreamingParser parser;
    CloneListener listener = CloneListener(extensions, stats);
    parser.setListener(&listener);

    char c;
    while (json_in.get(c)) {
        parser.parse(c);
    }
}

