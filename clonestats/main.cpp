#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include <ctgmath>
#include "CloneParser.h"
#include "../util/ArgParser.h"
#include "../util/stringescape.h"

void parse_json(std::map<unsigned long, FileData> &extensions,
                std::optional<std::map<unsigned long, unsigned long>> &lines,
                Statistics &stats,
                std::ifstream &json_in);

void print_occurrence_counts(const std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> &occurrences,
                             std::ostream &out);

void print_distance_matrix(const SimpleMatrix<unsigned long> &similarity_matrix, const std::map<unsigned long, FileData> &files,
                           const std::optional<std::string> &connectivity, std::ostream &out);

void print_count_matrix(const SimpleMatrix<unsigned long> &count_matrix, const std::map<unsigned long, FileData> &files,
                        std::ostream &out);

void print_results(const std::map<unsigned long, FileData> &files, const Statistics &stats, bool compute_distances,
                   bool compute_count, const std::optional<std::string> &connectivity, std::ostream &out);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];
    std::optional<std::string> out_file = args.getCmdArg("-o");
    std::optional<std::string> connectivity = args.getCmdArg("--connectivity");
    std::map<unsigned long, FileData> files;
    Statistics stats;
    std::optional<std::map<unsigned long, unsigned long>> lines;
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cerr << "json input file open fails. exit.\n";
        exit(1);
    }

    parse_json(files, lines, stats, json_in);
    json_in.close();

    bool compute_distance = args.cmdOptionExists("--distance");
    bool compute_count = args.cmdOptionExists("--count");

    if (out_file) {
        std::ofstream out(*out_file);

        if (!out) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }

        print_results(files, stats, compute_distance, compute_count, connectivity, out);
    } else {
        print_results(files, stats, compute_distance, compute_count, connectivity, std::cout);
    }

    return 0;
}

void print_results(const std::map<unsigned long, FileData> &files, const Statistics &stats, bool compute_distances,
                   bool compute_count, const std::optional<std::string> &connectivity, std::ostream &out) {
    if (compute_distances) {
        print_distance_matrix(stats.similarity_matrix, files, connectivity, out);
    } else if (compute_count) {
        print_count_matrix(stats.count_matrix, files, out);
    } else {
        // Print number of occurrences of repeated subsequences by file extension
        print_occurrence_counts(stats.occurrences, out);
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
                std::optional<std::map<unsigned long, unsigned long>> &lines,
                Statistics &stats,
                std::ifstream &json_in) {
    JsonStreamingParser parser;
    CloneListener listener = CloneListener(extensions, stats, lines);
    parser.setListener(&listener);

    char c;
    while (json_in.get(c)) {
        parser.parse(c);
    }
}

void
print_distance_matrix(const SimpleMatrix<unsigned long> &similarity_matrix, const std::map<unsigned long, FileData> &files,
                      const std::optional<std::string> &connectivity, std::ostream &out) {

    std::optional<std::ofstream> connect;

    if (connectivity) {
        connect = std::ofstream(*connectivity);

        if (!*connect) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }
    }

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

    for (int i = 0; i < similarity_matrix.size() - 1; i++) {    // discard the dummy
        for (int j = 0; j < similarity_matrix.size() - 1; j++) {
            if (j > 0) {   // no separator for the first value of the line
                out << ",";
                if (connect) *connect << ",";
            }
            unsigned long val = similarity_matrix.at(i, j) + similarity_matrix.at(j, i);
            if (i == j) {
                out << 0;
                if (connect) *connect << 1;
            } else if (val == 0) {
                out << 1e2;
                if (connect) *connect << 0;
            } else {
                out << log((double) (file_lengths[i] + file_lengths[j])) - log((double) val);
                if (connect) *connect << 1;
            }
        }
        out << "\n";
        if (connect) *connect << "\n";
    }
}

void
print_count_matrix(const SimpleMatrix<unsigned long> &count_matrix, const std::map<unsigned long, FileData> &files,
                      std::ostream &out) {

    const auto &concat_end = files.crbegin();  // special last pair
    if (!concat_end->second.name.empty()) throw std::runtime_error("Last file mapping should use the empty string as name");

    unsigned long file_count = files.size();
    unsigned long file_lengths[file_count];
    unsigned long last_file_start{};

    for (const auto &f : files) {
        unsigned int file_id = f.second.id;
        if (file_id < file_count - 1) {
            out << ",";
        }
        if (file_id > 0) {
            file_lengths[file_id - 1] = f.first - last_file_start;
        }
        out << f.second.name;
        last_file_start = f.first;
    }

    out << "\n";
    auto it = files.begin();

    for (int i = 0; i < count_matrix.size() - 1; i++) {    // discard the dummy
        out << it->second.name;
        ++it;
        for (int j = 0; j < count_matrix.size() - 1; j++) {
            out << ",";
            out << count_matrix.at(i, j) + count_matrix.at(j, i);
        }
        out << "\n";
    }
}
