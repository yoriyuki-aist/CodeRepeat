#include <iostream>
#include <algorithm>
#include <set>
#include <regex>
#include <unordered_set>
#include "../util/stringescape.h"
#include "../util/ArgParser.h"
#include "zlib/zstr.hpp"

namespace fs = std::filesystem;

using path = std::string;
using Repeats = std::unordered_map<path, std::vector<unsigned long>>;
using CharMap = std::map<unsigned long, std::string>;

struct ProcessingOptions {
    int min_repeat_length;
    bool skip_blank_repeats;
    bool skip_null_repeats;
    bool verbose;
    bool compress;
    std::string bwt_file;
    std::string json_file;
};

void filter(const std::map<unsigned long, std::string> &charmap, std::unordered_set<std::string> &splits,
            const ProcessingOptions &opts, const std::optional<std::map<unsigned long, unsigned long>> &linemap);

void emit_repeat(std::ostream &json_out, const std::string &subtext, const std::vector<unsigned long> &positions);

void
emit_verbose_repeat(std::ostream &json_out, const std::string& subtext, const std::vector<unsigned long>& positions, const CharMap &charmap,
                    const std::optional<std::map<unsigned long, unsigned long>> &linemap);

void
process_position(const CharMap &charmap, Repeats &repeats, std::string subtext, unsigned long pos, int min_repeat_size,
                 std::unordered_set<std::string> &splits) {
    unsigned long repeat_end = pos + subtext.size();
    auto it = --charmap.upper_bound(pos);
    bool split = false;

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
            split = true;
        }

        if (repeat_subtext.size() > min_repeat_size) {
            repeats[repeat_subtext].push_back(pos);

            if (split) {
                splits.insert(repeat_subtext);
            }
        }
    } while (!subtext.empty());
}

// custom extractor for objects of type RepeatEntry
void
read(std::istream &is, Repeats &repeats, const CharMap &charmap, std::unordered_set<std::string> &splits,
     const ProcessingOptions &opts) {
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
        bool skip = (repeat_size <= opts.min_repeat_length) ||
                    (opts.skip_blank_repeats && std::all_of(repeat_subtext.begin(), repeat_subtext.end(), [](char c) {
                        return std::isblank(c) || std::iscntrl(c);
                    })) ||
                    (opts.skip_null_repeats &&
                     std::all_of(repeat_subtext.begin(), repeat_subtext.end(), [](char c) { return c == 0; }));

        for (unsigned long i = 0; i < repeat_occurrences; i++) {
            unsigned long pos;
            is >> pos;

            if (!skip) {
                process_position(charmap, repeats, repeat_subtext, pos, opts.min_repeat_length, splits);
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

    std::string bwt_file = argv[1];
    std::string charmap_file = argv[2];
    std::string json_file = argv[3];
    std::optional<std::string> linemap_file = args.getCmdArg("--linemap");

    std::ifstream charmap_in(charmap_file);

    if (!charmap_in) {
        std::cerr << "charmap file open fails. exit.\n";
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

    std::optional<std::map<unsigned long, unsigned long>> linemap;

    if (linemap_file) {
        std::ifstream linemap_in(*linemap_file);
        linemap.emplace();

        if (!linemap_in) {
            std::cerr << "linemap output file open fails. exit.\n";
            exit(1);
        }

        while (linemap_in >> char_idx) {
            if (linemap_in.rdbuf()->sbumpc() != '\t') {
                std::cerr << "Unexpected character at position " << linemap_in.tellg() << " in " << *linemap_file;
                break;
            }
            std::getline(linemap_in, line);
            (*linemap)[char_idx] = std::stoul(line);
        }
    }

    std::unordered_set<std::string> splits;
    ProcessingOptions opts{
            std::stoi(args.getCmdArg("-m").value_or("0")),
            args.cmdOptionExists("--skip-blank"),
            args.cmdOptionExists("--skip-null"),
            args.cmdOptionExists("--verbose"),
            args.cmdOptionExists("--compress"),
            bwt_file,
            json_file
    };

    if (opts.verbose && !linemap_file) {
        throw std::runtime_error("Verbose output requires line mappings from the preprocessor");
    }

    // first pass: collect repeated subtexts that get split between files
    // if there is no such repeated subtext, this is the only pass
    filter(charmap, splits, opts, linemap);

    // second pass: we know which subtexts come from splits, we can guarantee they all get merged
    if (!splits.empty()) {
        filter(charmap, splits, opts, linemap);
    }

    return 0;
}

void filter(const std::map<unsigned long, std::string> &charmap, std::unordered_set<std::string> &splits,
            const ProcessingOptions &opts, const std::optional<std::map<unsigned long, unsigned long>> &linemap) {
    std::unique_ptr<std::istream> bwtp(opts.compress ? (std::istream*) new zstr::ifstream(opts.bwt_file) : new std::ifstream(opts.bwt_file));
    std::istream &bwt_in = *bwtp;

    if (!bwt_in) {
        std::cerr << "bwt input file open fails. exit.\n";
        exit(1);
    }

    std::unique_ptr<std::ostream> json_outp(opts.compress ? (std::ostream*) new zstr::ofstream(opts.json_file) : new std::ofstream(opts.json_file));
    std::ostream &json_out = *json_outp;

    if (!json_out) {
        std::cerr << "JSON output file open fails. exit.\n";
        exit(1);
    }

    std::cerr << "Writing JSON to " << opts.json_file << "\n";


    if (!opts.verbose) {
        json_out << "{\n";
        json_out << "\t\"version\": " << (opts.verbose ? 1 : 0) << ",\n";
        json_out << "\t\"file_starts\": {\n";

        bool sep = false;
        for (const auto &charentry : charmap) {
            if (sep) json_out << ",\n";
            else sep = true;

            json_out << "\t\t";
            write_escaped_string(json_out, charentry.second);
            json_out << ": " << charentry.first;
        }

        json_out << "\n\t},\n";

        if (linemap) {
            json_out << "\t\"line_starts\": {\n";
            sep = false;

            for (const auto &lineentry : *linemap) {
                if (sep) json_out << ",\n";
                else sep = true;

                json_out << "\t\t\"" << lineentry.second << "\": " << lineentry.first;
            }

            json_out << "\n\t},\n";
        }
        json_out << "\t\"repeats\": [";
    }

    bool print_obj_separator = false;

    Repeats late;

    try {
        while (bwt_in) {
            Repeats repeats;
            read(bwt_in, repeats, charmap, splits, opts);

            for (const auto &repeat : repeats) {
                // if the subtext can come from a split, we wait until the end to merge every position
                if (splits.find(repeat.first) == splits.end()) {
                    if (print_obj_separator) {
                        if (!opts.verbose) json_out << ",";
                        json_out << "\n";
                    }
                    if (opts.verbose) {
                        emit_verbose_repeat(json_out, repeat.first, repeat.second, charmap, linemap);
                    } else {
                        emit_repeat(json_out, repeat.first, repeat.second);
                    }
                    print_obj_separator = true;
                } else {
                    std::vector<unsigned long> &late_positions = late[repeat.first];
                    late_positions.insert(late_positions.begin(), repeat.second.begin(), repeat.second.end());
                }
            }
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt_in.tellg() << " in " << opts.bwt_file << ": "
                  << e.what();
    }

    for (const auto &repeat : late) {
        // after split, some "repeated sequences" may actually have a single occurrence
        if (repeat.second.size() > 1) {

            if (opts.verbose) {
                if (print_obj_separator) json_out << "\n";
                emit_verbose_repeat(json_out, repeat.first, repeat.second, charmap, linemap);
            } else {
                if (print_obj_separator) json_out << ",\n";
                emit_repeat(json_out, repeat.first, repeat.second);
            }

            print_obj_separator = true;
        }
    }

    if (!opts.verbose) {
        json_out << "\n\t]\n}";
    }

    // bwt_in auto close
    // json_out auto close
}

void
emit_verbose_repeat(std::ostream &json_out, const std::string& subtext, const std::vector<unsigned long>& positions, const CharMap &charmap,
                    const std::optional<std::map<unsigned long, unsigned long>> &linemap) {
    json_out << "{\"text\": ";
    write_escaped_string(json_out, subtext);
    json_out << ",\"locations\": [";
    bool print_separator = false;

    for (unsigned long start_pos : positions) {
        if (print_separator) json_out << ",";

        std::string filename = (--charmap.upper_bound(start_pos))->second;
        auto start_line = (--linemap->upper_bound(start_pos))->second;
        json_out << "{\"path\":\t\"" << filename << "\",\t";
        json_out << "\"start_line\": " << start_line << ",\t";
        unsigned long end_pos = start_pos + subtext.length() - 1; // if length == 1, end_pos == start_pos
        auto end_line = (--linemap->upper_bound(end_pos))->second;
        json_out << "\"end_line\":\t" << end_line << "}";
        print_separator = true;
    }

    json_out << "]}";
}

void
emit_repeat(std::ostream &json_out, const std::string &subtext, const std::vector<unsigned long> &positions) {
    json_out << "\t\t{\n\t\t\t\"text\": ";
    write_escaped_string(json_out, subtext);
#ifdef EXPORT_TEXT_LENGTH
    json_out << ",\n\t\t\t\"length\": " << subtext.length();
#endif
    json_out << ",\n\t\t\t\"positions\": [\n";
    bool print_separator = false;

    for (unsigned long pos : positions) {
        if (print_separator) json_out << ",\n";
        json_out << "\t\t\t\t" << pos;
        print_separator = true;
    }

    json_out << "\n\t\t\t]\n\t\t}";
}

