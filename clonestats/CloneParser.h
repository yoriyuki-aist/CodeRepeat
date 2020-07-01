#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include "json/JsonListener.h"

struct Occurrences {
    unsigned unique;
    unsigned total;
};

enum State {
    start, root, file_starts, repeats, repeat, positions, done
};

using extension = std::string;

struct RepeatData {
    std::string text;
    std::unordered_map<extension, unsigned long> occurrences{};
};

static const char * states[] = { "start", "root", "file starts", "repeats", "repeat", "positions", "done" };

class CloneListener : public JsonListener {
private:
    State state{start};
    RepeatData current_repeat{};
    std::string last_key{};
    // position in the concatenated file -> extension of the source file
    // will be empty if the "file_starts" object is not parsed before the "repeats"!
    std::map<unsigned long, std::string> &extensions;
    // extension -> repeat size -> number of occurrences
    std::unordered_map<extension, std::map<unsigned long, Occurrences>> &occurrences;

public:
    CloneListener(std::map<unsigned long, std::string> &extensions,
                  std::unordered_map<std::string, std::map<unsigned long, Occurrences>> &occurrences) :
            extensions(extensions), occurrences(occurrences) {};

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
