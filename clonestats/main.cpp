#include <iostream>
#include <algorithm>
#include <json/JsonStreamingParser.h>
#include <memory>
#include "CloneParser.h"
#include "VerboseCloneParser.h"
#include "../util/ArgParser.h"

struct ProcessingOptions {
    std::optional<std::string> out_file;
    bool verbose_input{};
    std::optional<std::string> connectivity;
    bool compute_distance{};
    bool compute_count{};
    bool bigcloneeval{};
};

void parse_json(JsonListener &listener, std::ifstream &json_in, const ProcessingOptions &opts) {
    JsonStreamingParser parser;
    parser.setListener(&listener);

    if (opts.verbose_input) parser.parse('[');

    char c;
    while (json_in.get(c)) {
        if (opts.verbose_input && c == '\n') parser.parse(',');
        parser.parse(c);
    }

    if (opts.verbose_input) parser.parse(']');
}

namespace fs = std::filesystem;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "\nUsage:\t" << argv[0] << "\t<clones_json>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv, argv + argc);

    std::string json_file = argv[1];

    ProcessingOptions opts{
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

    std::ofstream fout;
    std::ostream *out = &std::cout;

    if (opts.out_file) {
        fout = std::ofstream(*opts.out_file);
        out = &fout;

        if (!fout) {
            std::cerr << "output file open fails. exit.\n";
            exit(1);
        }

    }

    if (opts.verbose_input) {
        if (opts.bigcloneeval) {
            VerboseParser::TestCsvGenerator listener(*out);
            parse_json(listener, json_in, opts);
        } else {
            throw std::runtime_error("Verbose format is only supported for BigCloneEval mode");
        }
    } else {
        std::unique_ptr<CloneListener> listener;
        if (opts.bigcloneeval) {
            listener = std::make_unique<TestCsvGenerator>(*out);
        } else if (opts.compute_distance) {
            listener = std::make_unique<DistanceMatrixGenerator>(*out, opts.connectivity);
        } else if (opts.compute_count) {
            listener = std::make_unique<CountMatrixGenerator>(*out, opts.connectivity);
        } else {
            // Print number of occurrences of repeated subsequences by file extension
            listener = std::make_unique<OccurrenceCsvGenerator>(*out);
        }
        parse_json(*listener, json_in, opts);
        listener->end();
    }

    json_in.close();

    return 0;
}

