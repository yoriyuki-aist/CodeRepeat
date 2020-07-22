#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include <tgmath.h>
#include "CloneParser.h"
#include "../util/ArgParser.h"
#include "../util/stringescape.h"

void parse_json(std::map<unsigned long, FileData> &extensions,
                Statistics &stats,
                std::ifstream &json_in);

void print_occurrence_counts(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                             std::ostream &out);

void
print_idioms(std::vector<RepeatDigest> repeats, int min_occ, std::ostream &out);

void print_similarity_matrix(const SimpleMatrix<unsigned long> &similarity_matrix, const std::map<unsigned long, FileData> &files,
                             std::ostream &out);

void print_results(const std::map<unsigned long, FileData> &files, const Statistics &stats,
                   const std::optional<std::string> &idiom_occ, bool compute_distances, std::ostream &out);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];
    std::optional<std::string> out_file = args.getCmdArg("-o");
    std::map<unsigned long, FileData> files;
    Statistics stats;
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cerr << "json input file open fails. exit.\n";
        exit(1);
    }

    parse_json(files, stats, json_in);
    json_in.close();

    std::optional<std::string> idiom_occ = args.getCmdArg("--idioms");
    bool compute_distance = args.cmdOptionExists("--distance");

    if (out_file) {
        std::ofstream out(*out_file);

        if (!out) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }

        print_results(files, stats, idiom_occ, compute_distance, out);
    } else {
        print_results(files, stats, idiom_occ, compute_distance, std::cout);
    }

    return 0;
}

void print_results(const std::map<unsigned long, FileData> &files, const Statistics &stats,
                   const std::optional<std::string> &idiom_occ, bool compute_distances, std::ostream &out) {
    if (idiom_occ) {
        // Print list of repeated sequences with their frequency
        print_idioms(stats.repeats, std::stoi(*idiom_occ), out);
    } else if (compute_distances) {
        print_similarity_matrix(stats.similarity_matrix, files, out);
    } else {
        // Print number of occurrences of repeated subsequences by file extension
        print_occurrence_counts(stats.occurrences, out);
    }
}

void
print_idioms(std::vector<RepeatDigest> repeats, int min_occ, std::ostream &out) {
    std::sort(repeats.begin(), repeats.end(), [](const auto &r1, const auto &r2) {
        return r1.occurrences < r2.occurrences;
    });

    for (const auto &repeat : repeats) {
        std::string subtext = repeat.text;
        unsigned long occurrences = repeat.occurrences;

        if (occurrences >= min_occ) {
            out << "Idioms, " << "occurrences:" << occurrences << "\n";
            out << "Subtext starts --------------------------------------------\n";
            write_escaped_string(out, subtext);
            out << "\n";
            out << "Subtext ends   --------------------------------------------\n";
        }
    }
}

void print_occurrence_counts(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                             std::ostream &out) {
    out << "File extension,Size,Occurrence(s),unique sequence(s)\n";
    for (const auto &ext_entry : occurrences) {
        for (const auto &size_entry : ext_entry.second) {
            out << (ext_entry.first.empty() ? "(none)" : ext_entry.first) << ',';
            out << size_entry.first << "," << size_entry.second.total << ",";
            out << size_entry.second.unique;
            out << "\n";
        }
    }
}

void parse_json(std::map<unsigned long, FileData> &extensions,
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

void
print_similarity_matrix(const SimpleMatrix<unsigned long> &similarity_matrix, const std::map<unsigned long, FileData> &files,
                        std::ostream &out) {
    
    const auto &concat_end = files.crbegin();  // special last pair
    if (!concat_end->second.name.empty()) throw std::runtime_error("Last file mapping should use the empty string as name");

    unsigned long file_lengths[files.size()];
    unsigned long last_file_start{};

    for (const auto &f : files) {
        unsigned int file_id = f.second.id;
        if (file_id > 0) {
            out << ",";
            file_lengths[file_id - 1] = f.first - last_file_start;
        }
        out << f.second.name;
        last_file_start = f.first;
    }

    out << "\n";

    for (int i = 0; i < similarity_matrix.size(); i++) {
        for (int j = 0; j < similarity_matrix.size(); j++) {
            if (j > 0) out << ",";  // no separator for the first value of the line
            unsigned long val = similarity_matrix.at(i, j) + similarity_matrix.at(j, i);
            if (val == 0) {
                out << 1e50;
            } else {
                out << log((double) (file_lengths[i] + file_lengths[j])) - log((double) val);
            }
        }
        out << "\n";
    }
}

