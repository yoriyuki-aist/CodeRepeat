#pragma once

#include <string>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <filesystem>
#include "json/JsonListener.h"

namespace VerboseParser {
    enum State {
        start, repeats, repeat, locations, location, done
    };

    static const char *states[] = {"start", "repeats", "repeat", "locations", "location", "done"};

    struct Location {
        std::filesystem::path path;
        unsigned start_line;
        unsigned end_line;
    };

    struct RepeatData {
        std::string text;
        std::vector<Location> locations{};
    };

    class CloneListener : public JsonListener {
    private:
        State state{start};
        RepeatData current_repeat{};
        Location current_location{};
        std::string last_key{};

    protected:
        std::ostream &out;

        virtual void onRepeat(const RepeatData &repeat) {};

    public:
        explicit CloneListener(std::ostream &out): out{out} {};

        void whitespace(char c) override {
            // NO-OP
        }

        void startDocument() override {
            // NO-OP
        }

        void key(std::string key) override {
            last_key = key;
        }

        void value(std::string value) override {
            if (state == repeat) {
                if (last_key == "text") {
                    current_repeat.text = value;
                }
            } else if (state == location) {
                if (last_key == "path") {
                    current_location.path = value;
                } else if (last_key == "start_line") {
                    current_location.start_line = std::stoul(value);
                } else if (last_key == "end_line") {
                    current_location.end_line = std::stoul(value);
                }
            }
        }

        void endArray() override {
            if (state == locations) {
                state = repeat;
            } else if (state == repeats) {
                state = done;
            } else {
                throw std::runtime_error("Unexpected array end after key " + last_key + " in state " + states[state]);
            }
        }

        void endObject() override {
            if (state == location) {
                this->current_repeat.locations.push_back(std::move(this->current_location));
                this->current_location = Location{};
                state = locations;
            } else if (state == repeat) {
                this->onRepeat(this->current_repeat);
                this->current_repeat = RepeatData{};
                state = repeats;
            } else {
                throw std::runtime_error("Unexpected object end after key " + last_key + " in state " + states[state]);
            }
        }

        void endDocument() override {
            if (state != done) {
                throw std::runtime_error("Unexpected end of document after key " + last_key + " in state " + states[state]);
            }
        }

        void startArray() override {
            if (state == start) {
                state = repeats;
            } else if (state == repeat) {
                state = locations;
            } else {
                throw std::runtime_error("Unexpected array start after key " + last_key + " in state " + states[state]);
            }
        }

        void startObject() override {
            if (state == repeats) {
                state = repeat;
            } else if (state == locations) {
                state = location;
            } else {
                throw std::runtime_error("Unexpected object start after key "+ last_key + " in state " + states[state]);
            }
        }
    };

    class TestCsvGenerator : public CloneListener {
    public:
        explicit TestCsvGenerator(std::ostream &out) : CloneListener(out) {}

    protected:
        void onRepeat(const RepeatData &repeat) override {
            unsigned long size = repeat.locations.size();
            for (int i = 0; i < size-1; i++) {
                for (int j = i+1; j < size; j++) {
                    const auto &pos = repeat.locations[i];
                    out << pos.path.parent_path().filename() << "," << pos.path.filename() << "," << pos.start_line << "," << pos.end_line
                         << ",";
                    const auto &pos2 = repeat.locations[j];
                    out << pos2.path.parent_path().filename() << "," << pos2.path.filename() << "," << pos2.end_line << "\n";
                }
            }
        }
    };
}
