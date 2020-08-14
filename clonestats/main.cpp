#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include <ctgmath>
#include "CloneParser.h"
#include "../util/ArgParser.h"

struct ProcessingOptions {
    std::optional<std::string> out_file;
    bool verbose_input;
    std::optional<std::string> connectivity;
    bool compute_distance;
    bool compute_count;
    bool bigcloneeval;
};

void parse_json(CloneListener &listener, std::ifstream &json_in, ProcessingOptions opts) {
    JsonStreamingParser parser;
    parser.setListener(&listener);

    std::ofstream out;

    if (opts.out_file) {
        out = std::ofstream(*opts.out_file);

        if (!out) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }

        listener.output(&out);
    }

    if (opts.verbose_input) parser.parse('[');

    char c;
    while (json_in.get(c)) {
        if (opts.verbose_input && c == '\n') parser.parse(',');
        parser.parse(c);
    }

    if (opts.verbose_input) parser.parse(']');

    listener.end();
}

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];

    ProcessingOptions opts {
        args.getCmdArg("-o"),
        args.cmdOptionExists("--verbose-input"),
        args.getCmdArg("--connectivity"),
        args.cmdOptionExists("--distance"),
        args.cmdOptionExists("--count"),
        args.cmdOptionExists("--bigcloneeval")
    };

    std::ifstream json_in(json_file);

    if (!json_in) {
        std::cerr << "json input file open fails. exit.\n";
        exit(1);
    }

    if (opts.bigcloneeval) {
        TestCsvGenerator listener;
        parse_json(listener, json_in, opts);
    } else if (opts.compute_distance) {
        DistanceMatrixGenerator listener(opts.connectivity);
        parse_json(listener, json_in, opts);
    } else if (opts.compute_count) {
        CountMatrixGenerator listener(opts.connectivity);
        parse_json(listener, json_in, opts);
    } else {
        // Print number of occurrences of repeated subsequences by file extension
        OccurrenceCsvGenerator listener;
        parse_json(listener, json_in, opts);
    }

    json_in.close();

    return 0;
}

