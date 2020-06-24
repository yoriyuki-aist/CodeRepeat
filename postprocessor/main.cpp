#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <bits/unordered_set.h>
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
    const unsigned int concatpos{};
    // The position of the repeat in the source file (begins at 0!)
    const unsigned int sourcepos{};
    // the fraction of the total file's length that the repeat represents
    const double relative_size;
    const std::string source_file;
};

struct RepeatEntry {
    int size{};
    int occurrences{};
    std::string subtext;
    std::vector<RepeatPosition> positions;
};

struct FileRepeats {
    std::vector<RepeatEntry> repeats;
    fs::path path;
};

using extension = std::string;
using ProcessedRepeats = std::unordered_map<extension, std::vector<FileRepeats>>;
using CharMap = std::map<unsigned int, std::string>;

Json::Value
to_json(const ProcessedRepeats &repeats,
        const CharMap &map);

ProcessedRepeats process(const std::vector<RepeatEntry> &vector);

RepeatPosition find_repeat_position(unsigned int concatpos, const CharMap &charmap, int repeat_length) {
    auto next_entry = charmap.upper_bound(concatpos);
    unsigned int file_end = next_entry->first;
    const std::pair<unsigned int, std::string> &entry = *(--next_entry);
    unsigned int file_begin = entry.first;
    unsigned int file_size = file_end - file_begin;
    double relative_length = (double) repeat_length / file_size;
    return {concatpos, concatpos - entry.first, relative_length, entry.second};
}

// custom extractor for objects of type RepeatEntry
std::istream &read(std::istream &is, RepeatEntry &repeat, const CharMap &charmap) {
    repeat.positions.clear();
    std::istream::sentry s(is);
    std::string line;
    if (s) {
        std::getline(is, line, ':');
        if (line != "Repeat size") {
            throw std::runtime_error("Expected repeat size in first Repeat line");
        }
        is >> repeat.size;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');
        if (line != "Number of occurrences") {
            throw std::runtime_error("Expected number of occurrences in second Repeat line");
        }
        is >> repeat.occurrences;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');

        if (line != "Repeat subtext") {
            throw std::runtime_error("Expected repeat subtext in third Repeat line");
        }

        is.get();   // discard the space immediately after
        char subtext[repeat.size];
        is.read(subtext, repeat.size);
        repeat.subtext = std::string(subtext, subtext + repeat.size);
        std::getline(is, line);    // get rid of the end of the line
        std::getline(is, line, ':');

        if (line !="Suffix array interval of this repeat") {
            throw std::runtime_error("Expected suffix array interval in fourth Repeat line");
        }

        std::getline(is, line);    // discard suffix array interval

        std::getline(is, line, ':');
        if (line != "Text positions of this repeat") {
            throw std::runtime_error("Expected text positions in fifth Repeat line");
        }
        unsigned int pos;
        while (is >> pos) {
            repeat.positions.push_back(find_repeat_position(pos, charmap, repeat.size));
        }
        is.clear();
    }
    return is;
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

    std::map<unsigned int, std::string> charmap;
    std::string line;
    unsigned int char_idx;

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

    std::vector<RepeatEntry> repeats;
    RepeatEntry repeat;
    try {
        while (read(bwt, repeat, charmap)) {
            repeats.push_back(repeat);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt.tellg() << " in " << bwt_file << ": "
                  << e.what();
    }

    bwt.close();

    ProcessedRepeats processedRepeats = process(repeats);

    std::ofstream json_out(json_file);

    if (!json_out) {
        std::cout << "json output file open fails. exit.\n";
        exit(1);
    }

    json_out << to_json(processedRepeats, charmap);

    json_out.close();
}

ProcessedRepeats process(const std::vector<RepeatEntry> &vector) {
    std::unordered_map<std::string, std::unordered_set<const RepeatEntry*>> file2repeats;

    for (const RepeatEntry &repeat : vector) {
        for (const RepeatPosition &position : repeat.positions) {
            file2repeats[position.source_file].insert(&repeat);
        }
    }

    std::unordered_map<extension, std::vector<FileRepeats>> result;

    for (const auto &f2r : file2repeats) {
        fs::path file(f2r.first);
        std::vector<RepeatEntry> repeats;

        for (const auto &repeat : f2r.second) {
            repeats.push_back(*repeat);
        }

        result[file.extension()].push_back({repeats, file});
    }

    return result;
}

Json::Value to_json(const ProcessedRepeats &repeats, const CharMap &charmap) {
    Json::Value root;
    root["version"] = 0;

    for (const auto &extension : repeats) {
        for (const auto &file_repeats : extension.second) {
            Json::Value filejson;
            filejson["path"] = std::string(file_repeats.path);

            for (const auto &repeat : file_repeats.repeats) {
                Json::Value repeatjson;
                repeatjson["size"] = repeat.size;
                repeatjson["occurrences"] = repeat.occurrences;
                repeatjson["subtext"] = repeat.subtext;

                for (const RepeatPosition &pos : repeat.positions) {
                    Json::Value posjson;
                    posjson["concat_position"] = pos.concatpos;
                    posjson["source_position"] = pos.sourcepos;
                    posjson["source_file"] = pos.source_file;
                    posjson["relative_size"] = pos.relative_size;
                    repeatjson["positions"].append(posjson);
                }

                filejson["repeats"].append(repeatjson);
            }

            root[extension.first].append(filejson);
        }
    }

    return root;
}
