#include <filesystem>

#include "CloneParser.h"


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
        statistics.similarity_matrix = SimpleMatrix<unsigned long>(files.size());
        statistics.count_matrix = SimpleMatrix<unsigned long>(files.size());
    } else if (state == repeat) {
        state = repeats;
        unsigned long size = current_repeat.text.size();

        if (current_repeat.length >= 0 && size != current_repeat.length) {
            throw std::runtime_error(std::string("Invalid length from parsed JSON: ") +
            " should be " + std::to_string(current_repeat.length) +
            " chars, got " + std::to_string(current_repeat.text.length()));
        }

        unsigned count = 0;
        std::unordered_set<extension> encountered_exts;
        std::unordered_map<unsigned, unsigned> repeats_per_file;

        for (const FileData &source_file : current_repeat.occurrences) {
            auto &occ = statistics.occurrences[source_file.ext][size];
            count++;
            occ.total++;
            repeats_per_file[source_file.id]++;

            // Only works if the entries in the JSON are unique themselves (no 2 entries with the same text)
            if (encountered_exts.insert(source_file.ext).second) {
                occ.unique++;
            }

            // similarity[x, y] += (size of the repeat) * (combined number of occurrences in x and y)
            for (unsigned file_id : current_repeat.file_ids) {
                statistics.similarity_matrix.at(source_file.id, file_id) += size;
            }
            for (unsigned file_id : current_repeat.file_ids) {
                statistics.count_matrix.at(source_file.id, file_id) += 1;
            }
        }

        statistics.repeats.push_back({current_repeat.text, count});
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
        unsigned id = files.size();
        std::string ext = std::filesystem::path(last_key).extension();
        unsigned long file_start = std::stoul(value);
        files[file_start] = {id, last_key, ext};
    } else if (state == positions) {
        FileData &source = (--files.upper_bound(std::stoul(value)))->second;
        current_repeat.occurrences.push_back(source);
        current_repeat.file_ids.insert(source.id);
    } else if (state == repeat && last_key == "text") {
        current_repeat.text = value;
    } else if (state == repeat && last_key == "length") {
        current_repeat.length = std::stol(value);
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

