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

void print_results(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                   std::ostream &out);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];
    std::optional<std::string> out_file = args.getCmdArg("-o");
    std::map<unsigned long, std::string> extensions;
    Statistics stats;
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cerr << "output file open fails. exit.\n";
        exit(1);
    }

    parse_json(extensions, stats, json_in);
    json_in.close();

    // Number of occurrences of repeated subsequences by file extension
    const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences = stats.occurrences;

    if (out_file) {
        std::ofstream out(*out_file);
        print_results(occurrences, out);
    } else {
        print_results(occurrences, std::cout);
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
}

void print_results(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                   std::ostream &out) {
    out << "File extension, Size, Occurrence(s), unique sequence(s)\n";
    for (const auto &ext_entry : occurrences) {
        for (const auto &size_entry : ext_entry.second) {
            out << (ext_entry.first.empty() ? "(none)" : ext_entry.first) << ',';
            out << size_entry.first << "," << size_entry.second.total << ",";
            out << size_entry.second.unique;
            out << "\n";
        }
    }
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

