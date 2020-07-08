#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "json/JsonListener.h"

using extension = std::string;

struct OccurrenceCounter {
    unsigned unique;
    unsigned total;
};

struct RepeatDigest {
    std::string text;
    unsigned long occurrences;
};

struct Statistics {
    // extension -> repeat size -> number of occurrences
    std::unordered_map<std::string, std::map<unsigned long, OccurrenceCounter>> occurrences;
    // subtext -> number of occurrences
    std::unordered_map<std::string, unsigned long> repeats;
};

enum State {
    start, root, file_starts, repeats, repeat, positions, done
};

static const char *states[] = {"start", "root", "file starts", "repeats", "repeat", "positions", "done"};

struct RepeatData {
    std::string text;
    long length{-1};
    std::unordered_map<extension, unsigned long> occurrences{};
};

class CloneListener : public JsonListener {
private:
    State state{start};
    RepeatData current_repeat{};
    std::string last_key{};
    // position in the concatenated file -> extension of the source file
    // will be empty if the "file_starts" object is not parsed before the "repeats"!
    std::map<unsigned long, std::string> &extensions;
    Statistics &statistics;

public:
    CloneListener(std::map<unsigned long, std::string> &extensions, Statistics &statistics) :
            extensions(extensions), statistics(statistics) {};

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
