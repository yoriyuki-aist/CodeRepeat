#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "../util/stringescape.h"
#include "../util/ArgParser.h"
#include "zlib/zstr.hpp"

namespace fs = std::filesystem;

using path = std::string;
using Repeats = std::unordered_map<path, std::unordered_set<unsigned long>>;
using CharMap = std::map<unsigned long, std::string>;

struct ProcessingOptions {
    int min_repeat_length;
    bool skip_blank_repeats;
    bool skip_null_repeats;
    bool compress;
    std::string bwt_file;
    std::string json_file;
};


void
emit_verbose_repeat(std::ostream &json_out, const std::string &subtext, const std::unordered_set<unsigned long> &positions,
                    const CharMap &charmap,
                    const std::map<unsigned long, unsigned long> &linemap) {
    json_out << "{\"text\": ";
    write_escaped_string(json_out, subtext);
    json_out << ",\"locations\": [";
    bool print_separator = false;
    
 
    for (unsigned long start_pos : positions) {
        if (print_separator) json_out << ",";

        std::string filename = (--charmap.upper_bound(start_pos))->second;
        auto start_line = (--linemap.upper_bound(start_pos))->second;
        json_out << "{\"path\":\t\"" << filename << "\",\t";
        json_out << "\"start_line\": " << start_line << ",\t";
        unsigned long end_pos = start_pos + subtext.length() - 1; // if length == 1, end_pos == start_pos
        auto end_line = (--linemap.upper_bound(end_pos))->second;
        json_out << "\"end_line\":\t" << end_line << "}";
        print_separator = true;
    }

    json_out << "]}";
}



bool should_skip(std::string &text, const ProcessingOptions &opts){
   bool skip = (text.size() <= opts.min_repeat_length) ||
                        (opts.skip_blank_repeats && std::all_of(text.begin(), text.end(), [](char c) {
                            return std::isblank(c) || std::iscntrl(c);
                        })) ||
                        (opts.skip_null_repeats &&
                        std::all_of(text.begin(), text.end(), [](char c) { return c == 0; }));   
    return skip;
}


void
process_position(const CharMap &charmap, Repeats &repeats, std::string subtext, unsigned long pos, const ProcessingOptions &opts) {
    unsigned long repeat_end = pos + subtext.size() - 1;
    auto it = --charmap.upper_bound(pos);

    do {
        // name of the source file where the start of the repeat is found
        const std::string &source_file = it->second;
        it++;
        // beginning of the next file after the start of the repeat
        unsigned long file_end = it->first - 1;
        std::string repeat_subtext;

        if (it == charmap.end() || repeat_end <= file_end) {
            // there is no next file, or the repeat fits in the current file -> no more processing required
            repeat_subtext = std::move(subtext);
            subtext.clear();
            if (!should_skip(repeat_subtext, opts)) {
                repeats[repeat_subtext].insert(pos);
            }    
        } else {
            // the repeated sequence spans multiple files -> split it
            unsigned long actual_size = file_end - pos + 1;
            repeat_subtext = subtext.substr(0, actual_size);
            subtext = subtext.substr(actual_size);
            if (!should_skip(repeat_subtext, opts)) {
                repeats[repeat_subtext].insert(pos);
            }
            pos += actual_size;
        }
    } while (!subtext.empty());
}

// custom extractor for objects of type RepeatEntry
void
read(std::istream &is, Repeats &repeats, const CharMap &charmap, const ProcessingOptions &opts) {
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
        std::string repeat_subtext(subtext, repeat_size);
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
            
            std::cerr << is.tellg() <<'\r';
            process_position(charmap, repeats, repeat_subtext, pos, opts);
        }
    }
}


int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "\nUsage:\t" << argv[0]
                  << "\t<bwt_output>\t<charmap_file>\t<linemap_file>\t<output_file>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv + 4, argv + argc);

    std::string bwt_file = argv[1];
    std::string charmap_file = argv[2];
    std::string linemap_file = argv[3];
    std::string json_file = argv[4];

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

    std::map<unsigned long, unsigned long> linemap;

    std::ifstream linemap_in(linemap_file);
    if (!linemap_in) {
        std::cerr << "linemap output file open fails. exit.\n";
        exit(1);
    }
    while (linemap_in >> char_idx) {
        if (linemap_in.rdbuf()->sbumpc() != '\t') {
            std::cerr << "Unexpected character at position " << linemap_in.tellg() << " in " << linemap_file;
            break;
        }
        std::getline(linemap_in, line);
        linemap[char_idx] = std::stoul(line);
    }

    std::unordered_set<std::string> splits;
    ProcessingOptions opts{
            std::stoi(args.getCmdArg("-m").value_or("0")),
            args.cmdOptionExists("--skip-blank"),
            args.cmdOptionExists("--skip-null"),
            args.cmdOptionExists("--compress"),
            bwt_file,
            json_file
    };

    // first pass: colecting repeats splitting if necessary
    Repeats repeats;

    std::unique_ptr<std::istream> bwtp(
            opts.compress ? (std::istream *) new zstr::ifstream(opts.bwt_file) : new std::ifstream(opts.bwt_file));
    std::istream &bwt_in = *bwtp;

    if (!bwt_in) {
        std::cerr << "bwt input file open fails. exit.\n";
        exit(1);
    }
    try {
        while (bwt_in) {
            read(bwt_in, repeats, charmap, opts);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt_in.tellg() << " in " << opts.bwt_file << ": "
                  << e.what();
    }
    
    // preparation for second pass
    std::unique_ptr<std::ostream> json_outp(
            opts.compress ? (std::ostream *) new zstr::ofstream(opts.json_file) : new std::ofstream(opts.json_file));
    std::ostream &json_out = *json_outp;

    if (!json_out) {
        std::cerr << "JSON output file open fails. exit.\n";
        exit(1);
    }
    bool print_obj_separator = false;
    // output repeats: we know which subtexts come from splits, we can guarantee they all get merged
    for (const auto &repeat : repeats) {
        // after split, some "repeated sequences" may actually have a single occurrence
        if (repeat.second.size() > 1) {
            if (print_obj_separator) json_out << "\n";
                emit_verbose_repeat(json_out, repeat.first, repeat.second, charmap, linemap);
            print_obj_separator = true;
        }
    }
    return 0;
}
