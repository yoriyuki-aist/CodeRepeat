#include <iostream>
#include <fstream>
#include <algorithm>

const char EOF_CHAR = char(26);

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "\nUsage:\t%s\t<output>\t<inputs...>\n";
        exit(1);
    }

    std::string out_file = argv[1];

    std::ofstream out(out_file);
    if (!out) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }
    for (int i = 2; i < argc; i++) {
        std::ifstream in(argv[i]);
        if (!in) {
            std::cout << "input file " << argv[i] << " open fails. exit.\n";
            exit(1);
        }
        out << in.rdbuf();
        out << EOF_CHAR;
        in.close();
    }
    out.close();
    std::ifstream is(out_file);   // open file
    char c;
    while (is.get(c))                  // loop getting single characters
        std::cout << c;
    is.close();                        // close file

    std::cout << "Done!\n";
}