#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>

namespace fs = std::filesystem;

const char EOF_CHAR = char(26);

bool cmdOptionExists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "\nUsage:\t%s\t<input_directory>\t<output_file>\t[<options...>]\n";
        exit(1);
    }

    bool normalize_spaces = cmdOptionExists(argv, argv + argc, "-ns");
    bool remove_trailing_spaces = cmdOptionExists(argv, argv + argc, "-ntr");
    bool normalize_newlines = cmdOptionExists(argv, argv + argc, "-nl");
    bool process_text = normalize_spaces || remove_trailing_spaces;

    std::string out_file = argv[2];

    std::ofstream out(out_file);
    if (!out) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }
    std::cout << "Looking up files in " << argv[1] << "\n";
    std::set<fs::path> paths;   // directory iteration order is unspecified, so we sort all the paths for consistent behaviour
    for (const auto &entry : fs::recursive_directory_iterator(argv[1])) {
        if (entry.is_regular_file()) {
            std::cout << entry.path() << std::endl;
            paths.insert(entry.path());
        }
    }

    std::regex space_regex("\\h+");
    std::regex trail_regex("\\s+(?=\r?\n)");
    std::regex newline_regex("\r?\n");

    for (const fs::path &path : paths) {
        std::ifstream in(path);
        std::cout << "opening input file " << path << "\n";

        if (!in) {
            std::cout << "input file " << path << " open fails. exit.\n";
            exit(1);
        }

        if (process_text) {
            constexpr size_t bufferSize = 1024 * 1024;
            std::unique_ptr<char[]> buffer(new char[bufferSize]);

            while (in) {
                in.read(buffer.get(), bufferSize);
                // process data in buffer
                // copies all data into buffer
                std::string processed_line;
                if (normalize_spaces) {
                    processed_line = std::regex_replace(buffer.get(), space_regex, " ");
                }
                if (remove_trailing_spaces) {
                    processed_line = std::regex_replace(buffer.get(), trail_regex, "");
                }
                if (normalize_newlines) {
                    processed_line = std::regex_replace(buffer.get(), newline_regex, "\n");
                }
                out << processed_line;
            }
        } else {
            out << in.rdbuf();
        }

        out << EOF_CHAR;
        in.close();
    }
    out.close();
    std::ifstream is(out_file, std::ios::binary);   // binary because EOF gets interpreted on windows in text mode
    std::cout << "\"\"\"\n";
    char c;
    while (is.get(c))                  // loop getting single characters
        std::cout << c;
    is.close();                        // close file
    std::cout << "\n\"\"\"\n";

    std::cout << "\nDone!\n";
}