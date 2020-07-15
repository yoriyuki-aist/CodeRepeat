#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "json/JsonListener.h"

using extension = std::string;
using fileid = unsigned;

struct OccurrenceCounter {
    unsigned unique;
    unsigned total;
};

struct FileData {
    unsigned id;
    std::string ext;
};

struct RepeatDigest {
    std::string text;
    unsigned long occurrences;
};

struct Statistics {
    // extension -> repeat size -> number of occurrences
    std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> occurrences;
    // subtext -> number of occurrences
    std::vector<RepeatDigest> repeats;
    // file id -> file id -> similarity
    std::unique_ptr<double[]> similarity_matrix{nullptr};
};

enum State {
    start, root, file_starts, repeats, repeat, positions, done
};

static const char *states[] = {"start", "root", "file starts", "repeats", "repeat", "positions", "done"};

struct RepeatData {
    std::string text;
    long length{-1};
    std::vector<FileData> occurrences{};
};

class CloneListener : public JsonListener {
private:
    State state{start};
    RepeatData current_repeat{};
    std::string last_key{};
    // position in the concatenated file -> extension+id of the source file
    // will be empty if the "file_starts" object is not parsed before the "repeats"!
    std::map<unsigned long, FileData> &files;
    Statistics &statistics;

public:
    CloneListener(std::map<unsigned long, FileData> &files, Statistics &statistics) :
            files(files), statistics(statistics) {};

    void whitespace(char c) override;

    void startDocument() override;

    void key(std::string key) override;

    void value(std::string value) override;

    void endArray() override;

    void endObject() override;

    void endDocument() override;

    void startArray() override;

    void startObject() override;
};
