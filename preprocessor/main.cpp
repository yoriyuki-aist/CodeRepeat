#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

const char EOF_CHAR = char(26);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "\nUsage:\t%s\t<output_file>\t<input_directory>\n";
        exit(1);
    }

    std::string out_file = argv[1];

    std::ofstream out(out_file);
    if (!out) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }
    std::cout << "Looking up files in " << argv[2] << "\n";
    std::set<fs::path> paths;   // directory iteration order is unspecified, so we sort all the paths for consistent behaviour
    for (const auto &entry : fs::recursive_directory_iterator(argv[2])) {
        if (entry.is_regular_file()) {
            std::cout << entry.path() << std::endl;
            paths.insert(entry.path());
        }
    }
    for (const fs::path &path : paths) {
        std::ifstream in(path);
        std::cout << "opening input file " << path << "\n";
        if (!in) {
            std::cout << "input file " << path << " open fails. exit.\n";
            exit(1);
        }
        out << in.rdbuf();
        out << EOF_CHAR;
        in.close();
    }
    out.close();
    std::ifstream is(out_file, std::ios::binary);   // binary because EOF gets interpreted on windows in text mode
    char c;
    while (is.get(c))                  // loop getting single characters
        std::cout << c;
    is.close();                        // close file

    std::cout << "\nDone!\n";
}