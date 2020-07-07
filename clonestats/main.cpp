#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include "CloneParser.h"
#include "../util/ArgParser.h"

void parse_json(std::map<unsigned long, std::string> &extensions,
                Statistics &stats,
                std::ifstream &json_in);

void print_occurrence_counts(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                             std::ostream &out);

void
print_idioms(std::vector<RepeatDigest> repeats, const std::string &idiom_rate, std::ostream &out);

void print_results(const Statistics &stats, const std::optional<std::string> &idiom_rate, std::ostream &out);

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

    std::optional<std::string> idiom_rate = args.getCmdArg("--idioms");

    if (out_file) {
        std::ofstream out(*out_file);
        print_results(stats, idiom_rate, out);
    } else {
        print_results(stats, idiom_rate, std::cout);
    }
}

void print_results(const Statistics &stats, const std::optional<std::string> &idiom_rate, std::ostream &out) {
    if (idiom_rate) {
        // Print list of repeated sequences with their frequency
        print_idioms(stats.repeats, *idiom_rate, out);
    } else {
        // Print number of occurrences of repeated subsequences by file extension
        print_occurrence_counts(stats.occurrences, out);
    }
}

void
print_idioms(std::vector<RepeatDigest> repeats, const std::string &idiom_rate, std::ostream &out) {
    out << "Idiom,Occurrences\n";

    std::sort(repeats.begin(), repeats.end(), [](auto &e1, auto &e2) {
        return e1.occurrences < e2.occurrences;
    });

    auto idiom_start = (unsigned) (repeats.size() * (1.0 - std::stod(idiom_rate)));

    for (unsigned i = idiom_start; i < repeats.size(); i++) {
        const RepeatDigest &repeat = repeats[i];
        std::string subtext = repeat.text;
        size_t index = 0;

        while (true) {
            index = subtext.find('"', index);
            if (index == std::string::npos) break;

            /* Make the replacement. */
            subtext.replace(index, 1, "\"\"");

            /* Advance index forward so the next iteration doesn't pick it up as well. */
            index += 2;
        }
        out << '"' << subtext << "\"," << repeat.occurrences << "\n";
    }
}

void print_occurrence_counts(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
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

