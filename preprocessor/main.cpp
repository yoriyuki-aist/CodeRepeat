#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>

namespace fs = std::filesystem;

static const char SPACE_CHAR = ' ';
static const char EOF_CHAR = char(26);

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

bool isSpace(char c);

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
    std::set<fs::directory_entry> files;   // directory iteration order is unspecified, so we sort all the paths for consistent behaviour
    for (const auto &entry : fs::recursive_directory_iterator(argv[1])) {
        if (entry.is_regular_file()) {
            const auto &path = entry.path();
            if (!file_extensions || std::any_of(file_extensions->begin(), file_extensions->end(), [path](auto ext) {return endsWith(path.string(), ext);})) {
                std::cout << path << std::endl;
                files.insert(entry);
            }
        }
    }

    for (const fs::directory_entry &file : files) {
        std::ifstream in(file.path());
        std::cout << "opening input file " << file << "\n";

        if (!in) {
            std::cerr << "input file " << file << " open fails. exit.\n";
            exit(1);
        }

        out << "==================" << file << "==================\n";

        constexpr size_t buffer_size = 1024 * 1024;
        std::unique_ptr<char[]> buffer(new char[buffer_size]);
        std::unique_ptr<char[]> out_buffer(new char[buffer_size]);

        while (in) {
//            if (process_text) {
                in.read(buffer.get(), buffer_size);
                // process data in buffer
                bool skip_next_space = false;
                int space_start = -1;
                for (int in_idx = 0, out_idx = 0; in_idx < buffer_size; in_idx++) {
                    char c = buffer[in_idx];
                    if (remove_trailing_spaces) {
                        if (space_start >= 0 && (c == '\n' || c == '\r')) {
                            out_idx = space_start;   // erase spaces
                        }
                        if (std::isblank(c)) {
                            if (space_start < 0) {
                                space_start = out_idx;
                            }
                        } else {
                            space_start = -1;
                        }
                    }
                    if (normalize_newlines) {
                        if (c == '\r') {
                            continue;
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
                    out_buffer[out_idx] = c;
                    out_idx++;

                    if (c == '\0') {
                        break;
                    }
                }
//            } else {
//                in.read(out_buffer.get(), buffer_size);
//            }
            out << out_buffer.get();
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
