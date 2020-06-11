#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>

namespace fs = std::filesystem;

const char EOF_CHAR = char(26);

class ArgParser {
private:
    char **begin;
    char **end;
    bool empty;
public:
    ArgParser() = delete;
    ArgParser(char **begin, char **end): begin(begin), end(end) {
        empty = begin >= end;
    }

    bool cmdOptionExists(const std::string &option) {
        return !empty && std::find(begin, end, option) != end;
    }

    std::optional<std::vector<std::string>> getCmdArgs(const std::string &option) {
        if (!empty) {
            char **found = std::find(begin, end, option);
            if (found != end) {
                char **arg_start = found + 1;
                char **arg_end = arg_start;
                while (arg_end != end && *arg_end[0] != '-') {
                    arg_end++;
                }
                if (arg_start == arg_end) {
                    throw std::invalid_argument(option + " requires one or more arguments after it.");
                }
                return std::vector<std::string>(arg_start, arg_end);
            }
        }
        return {};
    }
};

bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "\nUsage:\t%s\t<input_directory>\t<output_file>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args = ArgParser(argv + 3, argv + argc);

    // (\h+)       -> ' '
    bool normalize_spaces = args.cmdOptionExists("-ns");
    // (\h+\r?\n)  -> ''
    bool remove_trailing_spaces = args.cmdOptionExists("-ntr");
    // (\r?\n)     -> '\n'
    bool normalize_newlines = args.cmdOptionExists("-nl");
    // (\r?\n)     -> ' '
    bool newlines_to_spaces = args.cmdOptionExists("-nl2s");
    bool process_text = normalize_spaces || remove_trailing_spaces || newlines_to_spaces;
    std::optional<std::vector<std::string>> file_extensions = args.getCmdArgs("--extensions");

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
            const auto &path = entry.path();
            if (!file_extensions || std::any_of(file_extensions->begin(), file_extensions->end(), [path](auto ext) {return endsWith(path.string(), ext);})) {
                std::cout << path << std::endl;
                paths.insert(path);
            }
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
                if (newlines_to_spaces) {
                    processed_line = std::regex_replace(buffer.get(), newline_regex, " ");
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
    // std::ifstream is(out_file, std::ios::binary);   // binary because EOF gets interpreted on windows in text mode
    // std::cout << "\"\"\"\n";
    // char c;
    // while (is.get(c))                  // loop getting single characters
    //     std::cout << c;
    // is.close();                        // close file
    std::cout << "\n\"\"\"\n";

    std::cout << "\nDone!\n";
}