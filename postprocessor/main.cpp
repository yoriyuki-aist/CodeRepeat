#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include "json/json.h"

namespace fs = std::filesystem;

bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

struct RepeatPosition {
    // The position of the repeat in the concatenated file (begins at 0)
    const unsigned long concatpos{};
    // The position of the repeat in the source file (begins at 0!)
    const unsigned long sourcepos{};
    const std::string source_file;
};


using path = std::string;
using Repeats = std::unordered_map<path, std::vector<RepeatPosition>>;
using CharMap = std::map<unsigned long, std::string>;

void write_to_json(const Repeats &repeats, const CharMap &charmap, const std::string &json_file);

void process_position(const CharMap &charmap, Repeats &repeats, std::string subtext, unsigned long pos) {
    unsigned long repeat_end = pos + subtext.size();
    auto it = --charmap.upper_bound(pos);

    do {
        // beginning of the source file where the start of the repeat is found
        unsigned long file_begin = it->first;
        // name of the source file where the start of the repeat is found
        const std::string &source_file = it->second;
        it++;
        // beginning of the next file after the start of the repeat
        unsigned long file_end = it->first;
        std::string repeat_subtext;

        if (it == charmap.end() || repeat_end <= file_end) {
            // there is no next file, or the repeat fits in the current file -> no more processing required
            repeat_subtext = std::move(subtext);
            subtext.clear();
        } else {
            // the repeated sequence spans multiple files -> split it
            unsigned long actual_size = file_end - pos;
            repeat_subtext = subtext.substr(0, actual_size);
            subtext = subtext.substr(actual_size);
            pos += actual_size;
        }

        repeats[repeat_subtext].push_back({pos, pos - file_begin, source_file});
    } while (!subtext.empty());
}

// custom extractor for objects of type RepeatEntry
void read(std::istream &is, Repeats &repeats, const CharMap &charmap) {
    std::istream::sentry s(is);
    std::string line;

    if (s) {
        std::getline(is, line, ':');

        if (line != "Repeat size") {
            throw std::runtime_error("Expected repeat size in first Repeat line");
        }

        unsigned long repeat_size;
        is >> repeat_size;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');

        if (line != "Number of occurrences") {
            throw std::runtime_error("Expected number of occurrences in second Repeat line");
        }

        unsigned long repeat_occurrences;
        is >> repeat_occurrences;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');

        if (line != "Repeat subtext") {
            throw std::runtime_error("Expected repeat subtext in third Repeat line");
        }

        is.get();   // discard the space immediately after
        char subtext[repeat_size];
        is.read(subtext, repeat_size);
        std::string repeat_subtext(subtext, subtext + repeat_size);
        std::getline(is, line);    // get rid of the end of the line
        std::getline(is, line, ':');

        if (line != "Suffix array interval of this repeat") {
            throw std::runtime_error("Expected suffix array interval in fourth Repeat line");
        }

        std::getline(is, line);    // discard suffix array interval
        std::getline(is, line, ':');

        if (line != "Text positions of this repeat") {
            throw std::runtime_error("Expected text positions in fifth Repeat line");
        }

        for (unsigned long i = 0; i < repeat_occurrences; i++) {
            unsigned long pos;
            is >> pos;
            process_position(charmap, repeats, repeat_subtext, pos);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<bwt_output>\t<charmap_file>\t<output_file>\t[<options...>]\n";
        exit(1);
    }

    std::string bwt_file = argv[1];
    std::string charmap_file = argv[2];
    std::string json_file = argv[3];
    std::ifstream charmap_in(charmap_file);

    if (!charmap_in) {
        std::cout << "charmap file open fails. exit.\n";
        exit(1);
    }

    std::map<unsigned long, std::string> charmap;
    std::string line;
    unsigned long char_idx;

    while (charmap_in >> char_idx) {
        if (charmap_in.rdbuf()->sbumpc() != '\t') {
            std::cerr << "Unexpected character at position " << charmap_in.tellg() << " in " << charmap_file;
            break;
        }
        std::getline(charmap_in, line);
        charmap[char_idx] = line;
    }

    charmap_in.close();

    std::ifstream bwt(bwt_file);

    if (!bwt) {
        std::cout << "bwt output file open fails. exit.\n";
        exit(1);
    }

    Repeats repeats;
    try {
        while (bwt) {
            read(bwt, repeats, charmap);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt.tellg() << " in " << bwt_file << ": "
                  << e.what();
    }

    bwt.close();
    std::cout << "Finished parsing BWT output\n";

    write_to_json(repeats, charmap, json_file);
}

void write_to_json(const Repeats &repeats, const CharMap &charmap, const std::string &json_file) {
    std::ofstream json_out(json_file);

    if (!json_out) {
        std::cout << "JSON output file open fails. exit.\n";
        exit(1);
    }

    Json::Value root;
    root["version"] = 0;

    for (const auto &charentry : charmap) {
        root["file_starts"][charentry.second] = charentry.first;
    }

    for (const auto &repeat : repeats) {
        Json::Value repeatjson;
        repeatjson["path"] = repeat.first;

        for (const RepeatPosition &pos : repeat.second) {
            repeatjson["positions"].append(pos.concatpos);
        }
        root["repeats"].append(repeatjson);
    }

    json_out << root;
    json_out.close();
}
