#include <iostream>
#include <algorithm>
#include <set>
#include <regex>
#include "../util/stringescape.h"
#include "../util/ArgParser.h"

namespace fs = std::filesystem;

using path = std::string;
using Repeats = std::unordered_map<path, std::vector<unsigned long>>;
using CharMap = std::map<unsigned long, std::string>;

void process_position(const CharMap &charmap, Repeats &repeats, std::string subtext, unsigned long pos,
                      int min_repeat_size) {
    unsigned long repeat_end = pos + subtext.size();
    auto it = --charmap.upper_bound(pos);

    do {
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

        if (repeat_subtext.size() > min_repeat_size) {
            repeats[repeat_subtext].push_back(pos);
        }
    } while (!subtext.empty());
}

// custom extractor for objects of type RepeatEntry
void read(std::istream &is, Repeats &repeats, const CharMap &charmap, int min_repeat_size, bool skip_blank) {
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

        // whether this repeated sequence should be skipped (we still have to consume the rest of the input)
        bool skip = (repeat_size <= min_repeat_size) ||
                (skip_blank && std::all_of(repeat_subtext.begin(), repeat_subtext.end(), [](char c) {return std::isblank(c);}));

        for (unsigned long i = 0; i < repeat_occurrences; i++) {
            unsigned long pos;
            is >> pos;

            if (!skip) {
                process_position(charmap, repeats, repeat_subtext, pos, min_repeat_size);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<bwt_output>\t<charmap_file>\t<output_file>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv + 4, argv + argc);
    int min_repeat_length = std::stoi(args.getCmdArg("-m").value_or("0"));
    bool skip_blank_repeats = args.cmdOptionExists("--skip-blank");

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
        std::cerr << "bwt output file open fails. exit.\n";
        exit(1);
    }

    std::ofstream json_out(json_file);

    if (!json_out) {
        std::cerr << "JSON output file open fails. exit.\n";
        exit(1);
    }

    std::cout << "Writing JSON to " << json_file << "\n";

    json_out << "{\n\t\"version\": 0,\n\t\"file_starts\": {\n";

    for (const auto &charentry : charmap) {
        json_out << "\t\t";
        write_escaped_string(json_out, charentry.second);
        json_out << ": " << charentry.first << ",\n";
    }

    json_out << "\t},\n\t\"repeats\": [\n";

    bool print_obj_separator = false;

    try {
        while (bwt) {
            Repeats repeats;
            read(bwt, repeats, charmap, min_repeat_length, skip_blank_repeats);

            for (const auto &repeat : repeats) {
                if (print_obj_separator) json_out << ",\n";

                json_out << "\t\t{\n\t\t\t\"text\": ";
                write_escaped_string(json_out, repeat.first);
#ifdef EXPORT_TEXT_LENGTH
                json_out << ",\n\t\t\t\"length\": " << repeat.first.length();
#endif
                json_out << ",\n\t\t\t\"positions\": [\n";
                bool print_separator = false;

                for (unsigned long pos : repeat.second) {
                    if (print_separator) json_out << ",\n";
                    json_out << "\t\t\t\t" << pos;
                    print_separator = true;
                }

                json_out << "\n\t\t\t]\n\t\t}";
                print_obj_separator = true;
            }
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt.tellg() << " in " << bwt_file << ": "
                  << e.what();
    }

    bwt.close();

    json_out << "\n\t]\n}";

    json_out.close();
}

