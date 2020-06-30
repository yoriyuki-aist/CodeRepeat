#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <json/json.h>
#include <unordered_set>

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
    std::unordered_map<std::string, std::unordered_map<unsigned long, unsigned>> occurrences;
    std::unordered_map<std::string, std::unordered_map<unsigned long, unsigned>> unique_occurrences;
    std::unordered_set<std::string> encountered_exts;

    for (const Json::Value &repeat : root["repeats"]) {
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

    std::cout << "\nDone!\n";
}
