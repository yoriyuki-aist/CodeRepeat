#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <json/json.h>
#include <unordered_set>
#include <bits/unordered_set.h>

void generate_stats(const Json::Value &repeats, std::map<unsigned long, std::string> &extensions,
                    std::unordered_map<std::string, std::map<unsigned long, unsigned int>> &occurrences,
                    std::unordered_map<std::string, std::map<unsigned long, unsigned int>> &unique_occurrences);

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    std::string json_file = argv[1];
    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }

    Json::Value root;
    json_in >> root;
    json_in.close();

    std::map<unsigned long, std::string> extensions;
    Json::Value &file_starts = root["file_starts"];

    for (auto it = file_starts.begin(); it != file_starts.end(); it++) {
        extensions[it->asUInt64()] = fs::path(it.key().asString()).extension();
    }

    // extension -> repeat size -> number of occurrences
    std::unordered_map<std::string, std::map<unsigned long, unsigned>> occurrences;
    std::unordered_map<std::string, std::map<unsigned long, unsigned>> unique_occurrences;
    generate_stats(root["repeats"], extensions, occurrences, unique_occurrences);

    std::cout << "Number of occurrences of repeated subsequences by file extension:\n";

    for (const auto &ext_entry : occurrences) {
        std::cout << "File extension: " << (ext_entry.first.empty() ? "(none)" : ext_entry.first) << "\n";

        for (const auto &size_entry : ext_entry.second) {
            std::cout << "- Size " << size_entry.first << ":\t" << size_entry.second << " occurrence(s) \t-\t";
            std::cout << unique_occurrences[ext_entry.first][size_entry.first] << " unique sequence(s)";
            std::cout << "\n";
        }
    }

    std::cout << "\nDone!\n";
}

void generate_stats(const Json::Value &repeats, std::map<unsigned long, std::string> &extensions,
                    std::unordered_map<std::string, std::map<unsigned long, unsigned int>> &occurrences,
                    std::unordered_map<std::string, std::map<unsigned long, unsigned int>> &unique_occurrences) {
    std::unordered_set<std::string> encountered_exts;

    for (const Json::Value &repeat : repeats) {
        unsigned size = repeat["text"].asString().size();

        for (const Json::Value &pos : repeat["positions"]) {
            std::string &source_ext = (--extensions.upper_bound(pos.asUInt64()))->second;
            occurrences[source_ext][size]++;

            if (encountered_exts.insert(source_ext).second) {
                unique_occurrences[source_ext][size]++;
            }
        }

        encountered_exts.clear();
    }
}
