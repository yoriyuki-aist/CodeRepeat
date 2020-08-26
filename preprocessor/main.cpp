#include <iostream>
#include <algorithm>
#include <filesystem>
#include <set>
#include "../util/ArgParser.h"

namespace fs = std::filesystem;

static const char SPACE_CHAR = ' ';
static const char EOF_CHAR = char(26);

bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "\nUsage:\t"<< argv[0] << "\t<input_directory>\t<output_file>\t<charmap_file>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv + 4, argv + argc);

    // (\h+)       -> ' '
    bool normalize_spaces = args.cmdOptionExists("-ns");
    // (\h+\r?\n)  -> ''
    bool remove_trailing_spaces = args.cmdOptionExists("-ntr");
    // (\r?\n)     -> '\n'
    bool normalize_newlines = args.cmdOptionExists("-nl");
    // (\r?\n)     -> ' '
    bool newlines_to_spaces = args.cmdOptionExists("-nl2s");
    bool eof = args.cmdOptionExists("-eof");
    bool debug = args.cmdOptionExists("--debug");
    bool verbose = args.cmdOptionExists("-v");
    bool symlink = args.cmdOptionExists("--symlinks");
    std::optional<std::vector<std::string>> file_extensions = args.getCmdArgs("--extensions");
    std::optional<std::string> linemap_file = args.getCmdArg("--linemap");

    std::string out_file = argv[2];
    std::string charmap_file = argv[3];

    std::ofstream out(out_file);
    std::ofstream charmap(charmap_file);
    std::optional<std::ofstream> linemap;

    if (linemap_file) {
        linemap.emplace(*linemap_file);

        if (!*linemap) {
            std::cerr << "linemap output file open fails. exit.\n";
            exit(1);
        }
    }

    if (!out) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }

    if (!charmap) {
        std::cout << "charmap file open fails. exit.\n";
        exit(1);
    }

    std::cout << "Looking up files in " << argv[1] << "\n";
    std::set<fs::directory_entry> files;   // directory iteration order is unspecified, so we sort all the paths for consistent behaviour
    std::filesystem::directory_options options = symlink
            ? fs::directory_options::follow_directory_symlink
            : fs::directory_options::none;
    for (const auto &entry : fs::recursive_directory_iterator(argv[1], options)) {
        if (entry.is_regular_file()) {
            const auto &path = entry.path();
            if (!file_extensions || std::any_of(file_extensions->begin(), file_extensions->end(),
                                                [path](auto ext) { return endsWith(path.string(), ext); })) {
                if (verbose) {
                    std::cout << path << std::endl;
                }
                files.insert(entry);
            }
        }
    }

    std::cout << "Processing files\n";
    for (const fs::directory_entry &file : files) {
        std::ifstream in(file.path());
        if (verbose) {
            std::cout << "opening input file " << file << "\n";
        }

        if (!in) {
            std::cerr << "input file " << file << " open fails. exit.\n";
            exit(1);
        }

        charmap << out.tellp() << "\t" << file.path().string() << "\n";
        if (linemap) {
            *linemap << out.tellp() << "\t" << 1 << "\n";
        }

        if (debug) {
            out << "==================" << file << "==================\n";
        }

        std::filebuf *inbuf = in.rdbuf();
        std::filebuf *outbuf = out.rdbuf();
        bool skip_next_space = false;
        unsigned long space_count = 0;
        unsigned long line_nb = 2;

        for (int c = inbuf->sbumpc(); c != EOF; c = inbuf->sbumpc()) {
            // process data in buffer
            if (normalize_newlines) {
                if (c == '\r') {
                    continue;
                }
            }

            if (linemap) {
                if (c == '\n') {
                    *linemap << out.tellp() << "\t" << line_nb << "\n";
                    ++line_nb;
                }
            }

            if (newlines_to_spaces) {
                if (c == '\n' || c == '\r') {
                    c = SPACE_CHAR;
                }
            }

            // space normalization must be after space-producing transformations
            if (normalize_spaces) {
                if (std::isblank(c)) {
                    if (skip_next_space) {
                        continue;
                    }
                    c = SPACE_CHAR;
                    skip_next_space = true;
                } else {
                    skip_next_space = false;
                }
            }

            if (remove_trailing_spaces) {
                if (space_count > 0 && (c == '\n' || c == '\r')) {
                    outbuf->pubseekoff(-space_count, std::ios_base::cur);   // erase spaces
                }
                if (std::isblank(c)) {
                    space_count++;
                } else {
                    space_count = 0;
                }
            }

            outbuf->sputc (c);
        }

        if (eof) {
            outbuf->sputc(EOF_CHAR);
        }

        in.close();
    }

    charmap << out.tellp() << "\t\n";   // blank file name == end
    out.close();
    charmap.close();
    if (linemap) linemap->close();

    std::cout << "\nDone!\n";
}
