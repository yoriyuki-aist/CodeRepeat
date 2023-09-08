#include <iostream>
#include <fstream>
#include <sdsl/suffix_arrays.hpp>

using namespace std;
using namespace sdsl;

int main(int argc,char** argv) {
    // Load or generate your input text

    std::ifstream inputFile(argv[5]);
    string input_text,line;
    while (std::getline(inputFile, line)) {
        input_text += line + "\n";
    }

    // Construct the compressed suffix array
    csa_wt<> csa;
    construct_im(csa, input_text, 1);

    // Define a minimum length for repeated substrings
    std::string arg = argv[2];
    size_t min_repeated_length = std::stoi(arg);  // Set to your desired minimum length
    //std::cout << min_repeated_length;

    std::ofstream outputFile(argv[4]);

    // Traverse the compressed suffix array to find repeated strings
    //std::cout << "csa.size(): "<< csa.size() <<"\n" ;
    for (size_t i = 1; i < csa.size(); ++i) {
        size_t sa_pos1 = csa[i - 1];  // Position of the suffix in the original text
        size_t sa_pos2 = csa[i];

        // Calculate the length of the common prefix
        size_t common_prefix_length = 0;
        while (sa_pos1 + common_prefix_length < input_text.size() &&
               sa_pos2 + common_prefix_length < input_text.size() &&
               input_text[sa_pos1 + common_prefix_length] == input_text[sa_pos2 + common_prefix_length]) {
            ++common_prefix_length;
        }

        // If the common prefix is long enough, output the repeated substring
        if (common_prefix_length >= min_repeated_length) {
            outputFile << "Repeat size: " << common_prefix_length << "\n"; 
            outputFile << "Number of occurrences: 2\n"; 
            outputFile << "Repeat subtext: " << input_text.substr(sa_pos1, common_prefix_length) << " \n";
            outputFile << "Suffix array interval of this repeat: " << "["<<i<<", "<< i+1 <<"]\n"; 
            outputFile << "Text positions of this repeat:  " << sa_pos1 << " " << sa_pos2 <<"\n\n"; 
        }
    }

    return 0;
}