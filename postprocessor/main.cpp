#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>

namespace fs = std::filesystem;

bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

struct RepeatEntry {
    int size{};
    int occurrences{};
    std::string subtext;
    std::vector<int> positions;
};

// custom extractor for objects of type RepeatEntry
std::istream& operator>>(std::istream& is, RepeatEntry& repeat)
{
    std::istream::sentry s(is);
    std::string line;
    if (s) {
        std::getline(is, line, ':');
        if (line != "Repeat size") {
            throw std::runtime_error("Expected repeat size in first Repeat line");
        }
        is >> repeat.size;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');
        if (line != "Number of occurrences") {
            throw std::runtime_error("Expected number of occurrences in second Repeat line");
        }
        is >> repeat.occurrences;
        std::getline(is, line); // discard rest of line
        std::getline(is, line, ':');
        if (line != "Repeat subtext") {
            throw std::runtime_error("Expected repeat subtext in third Repeat line");
        }
        is.get();
        std::getline(is, repeat.subtext);
        std::getline(is, line, ':');
        if (line != "Suffix array interval of this repeat") {
            throw std::runtime_error("Expected suffix array interval in fourth Repeat line");
        }
        std::getline(is, line); // discard suffix array interval
        std::getline(is, line, ':');
        if (line != "Text positions of this repeat") {
            throw std::runtime_error("Expected text positions in fifth Repeat line");
        }
        int pos;
        while (is >> pos) {
            repeat.positions.push_back(pos);
        }
        is.clear();
    }
    return is;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "\nUsage:\t%s\t<bwt_output>\t<charmap_file>\t[<options...>]\n";
        exit(1);
    }

    std::string bwt_file = argv[1];
    std::string charmap_file = argv[2];
    std::ifstream charmap_in(charmap_file);

    if (!charmap_in) {
        std::cout << "charmap file open fails. exit.\n";
        exit(1);
    }

    std::map<int, std::string> charmap;
    std::string line;
    int char_idx;

    while (charmap_in >> char_idx) {
        if (charmap_in.rdbuf()->sbumpc() != '\t') {
            std::cerr << "Unexpected character at position " << charmap_in.tellg() << " in " << charmap_file;
            break;
        }
        std::getline(charmap_in, line);
        charmap[char_idx] = line;
    }

    charmap_in.close();

    std::ifstream bwt(bwt_file);

    if (!bwt) {
        std::cout << "bwt output file open fails. exit.\n";
        exit(1);
    }

    std::vector<RepeatEntry> repeats;
    RepeatEntry repeat;
    try {
        while (bwt >> repeat) {
            repeats.push_back(repeat);
        }
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to read repeat entry at position " << bwt.tellg() << " in " << bwt_file << ": " << e.what();
    }

    bwt.close();
}
