#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>

int main(int argc, char **argv) {
    if (argc != 2 && argc != 4) {
        std::cout << "\nUsage:\t%s\t<BWT_input>\t[<BWT_output>\t<BWT_pos_output>]\n";
        exit(1);
    }

    std::string bwt_file = argv[1];
    std::string pos_output_file, output_file;
    if (argc > 2) {
        output_file = argv[2];
        pos_output_file = argv[3];
    } else {
        output_file = bwt_file + "raw";
        pos_output_file = bwt_file + "pos";
    }

    FILE *fp_bwt, *fp_pos_output, *fp_output;

    /* Open all relevant files */
    std::ifstream bwt_in(bwt_file);
    std::ofstream bwt_pos_out(pos_output_file);
    std::ofstream bwt_out(output_file);

    if (!bwt_in) {
        std::cout << "bwt file open fails. exit.\n";
        exit(1);
    }
    if (!bwt_pos_out) {
        std::cout << "bwt pos file open fails. exit.\n";
        exit(1);
    }
    if(!bwt_out) {
        std::cout << "output file open fails. exit.\n";
        exit(1);
    }
    std::string header;
    bwt_in >> header;
    if (header != "BWT") {
        std::cout << "invalid bwt file, does not begin with \"BWT\"";
    }
    std::string tmp;
    bwt_in >> tmp;
    bwt_pos_out << tmp;
    std::getline(bwt_in, tmp); // discard rest of the line
    bool newline;
    while (std::getline(bwt_in, tmp)) {
        tmp.erase(std::remove(tmp.begin(), tmp.end(), '\r'), tmp.end());
        if (newline) {
            bwt_out << "\n";
        }
        bwt_out << tmp;
        newline = true;
    }
    std::cout << "Done!\n";
}