#include <filesystem>

#include "CloneParser.h"
#include "json/JsonListener.h"


void CloneListener::startDocument() {
    // NO-OP
}

void CloneListener::whitespace(char c) {
    // NO-OP
}

void CloneListener::key(std::string key) {
    last_key = key;
}

void CloneListener::startObject() {
    if (state == root && last_key == "file_starts") {
        state = file_starts;
    } else if (state == repeats) {
        state = repeat;
    } else if (state == start) {
        state = root;
    } else {
        throw std::runtime_error("Unexpected object start after key " + last_key + " in state " + states[state]);
    }
}

void CloneListener::endObject() {
    if (state == file_starts) {
        state = root;
    } else if (state == repeat) {
        state = repeats;
        unsigned long size = current_repeat.text.size();
        
        for (const auto &entry : current_repeat.occurrences) {
            auto &occ = occurrences[entry.first][size];
            occ.total += entry.second;
            occ.unique++;
        }
        
        current_repeat.occurrences.clear();
        current_repeat.text.clear();
    } else if (state == root) {
        state = done;
    } else {
        throw std::runtime_error("Unexpected object end after key " + last_key + " in state " + states[state]);
    }
}

void CloneListener::value(std::string value) {
    if (state == file_starts) {
        std::string ext = std::filesystem::path(last_key).extension();
        extensions[std::stoul(value)] = ext;
    } else if (state == positions) {
        std::string &source_ext = (--extensions.upper_bound(std::stoul(value)))->second;
        current_repeat.occurrences[source_ext]++;
    } else if (state == repeat && last_key == "text") {
        current_repeat.text = value;
    } else if (state != root) {
        throw std::runtime_error("Unexpected value after key " + last_key + " in state " + states[state]);
    }
}

void CloneListener::startArray() {
    if (state == root && last_key == "repeats") {
        state = repeats;
    } else if (state == repeat && last_key == "positions") {
        state = positions;
    } else {
        throw std::runtime_error("Unexpected array start after key " + last_key + " in state " + states[state]);
    }
}

void CloneListener::endArray() {
    if (state == repeats) {
        state = root;
    } else if (state == positions) {
        state = repeat;
    } else {
        throw std::runtime_error("Unexpected array end after key " + last_key + " in state " + states[state]);
    }
}

void CloneListener::endDocument() {
    // NO-OP
}
