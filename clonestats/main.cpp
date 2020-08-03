#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include <ctgmath>
#include "CloneParser.h"
#include "../util/ArgParser.h"

void parse_json(CloneListener &listener, std::ifstream &json_in, std::optional<std::string> out_file) {
    JsonStreamingParser parser;
    parser.setListener(&listener);

    char c;
    while (json_in.get(c)) {
        parser.parse(c);
    }

    if (out_file) {
        std::ofstream out(*out_file);

        if (!out) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }

        listener.printResults(out);
    } else {
        listener.printResults(std::cout);
    }
}

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];
    std::optional<std::string> out_file = args.getCmdArg("-o");
    std::optional<std::string> connectivity = args.getCmdArg("--connectivity");
    bool compute_distance = args.cmdOptionExists("--distance");
    bool compute_count = args.cmdOptionExists("--count");

    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cerr << "json input file open fails. exit.\n";
        exit(1);
    }

    if (compute_distance) {
        DistanceMatrixGenerator listener(connectivity);
        parse_json(listener, json_in, out_file);
    } else if (compute_count) {
        CountMatrixGenerator listener(connectivity);
        parse_json(listener, json_in, out_file);
    } else {
        // Print number of occurrences of repeated subsequences by file extension
        OccurrenceCsvGenerator listener;
        parse_json(listener, json_in, out_file);
    }

    json_in.close();

    return 0;
}

