#include <iostream>
#include <algorithm>
#include <filesystem>
#include <set>
#include<assert.h>
#include "../util/ArgParser.h"

namespace fs = std::filesystem;


bool endsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


static const char SPACE_CHAR = ' ';
static const char EOF_CHAR = char(26);


class sink {
   public:
      // pure virtual function
        virtual void putc(char) = 0;
        virtual void flush(void){};
};


class filter: public sink {
    public:
        sink *out = 0;
        void connect(sink *out){
            this->out = out;
        };

        void flush(void){
            this->out->flush();
        };

        sink *operator>>=(sink *out){
            this->connect(out);
            return (sink *)this;
        };
};


class wrap_ostream: public sink {
    private:
        std::ostream *ostream = 0; 
    public:
        wrap_ostream(std::ostream &ostream){
            this->ostream = &ostream;
        };

        void putc(char c){
            *(this->ostream) << c;
        };

        void flush(void){
            (this->ostream)->flush();
        };
};



void output_to(std::istream &in, sink *out){
    char c;
    while(in.get(c)){
        out->putc(c);
    }
    out->flush();
};


class normalize_newlines: public filter {
    public:
        void putc(char c){
            if (c != '\r'){
                this->out->putc(c);
            }
        };
};


enum comment {
    plain, comment_start, multiline_end, quote, line_comment, multiline_comment
};

class remove_cstyle_comments: public filter {
    private:
        comment comment_state = plain;
        bool escape = false;
        char prev_c = 0;

    public:
        remove_cstyle_comments(void){};

        void flush(void){
            switch(comment_state) {
                case multiline_end:
                case comment_start: {
                    this->out->putc(prev_c);
                    break;
                }
            }
        };

        void putc(char c){
            if (!this->escape) {
                switch(c){
                    case '\\': {
                        escape = true;
                        this->out->putc(c);
                        break;
                    }
                    case '\n': {
                        switch (comment_state){
                            case comment_start:{
                                this->out->putc(prev_c);
                                comment_state = plain;
                                break;
                            }
                            case multiline_end:{
                                comment_state = multiline_comment;
                                break;
                            }
                            case line_comment:{
                                this->out->putc(c);
                                comment_state = plain;
                                break;
                            }
                            case multiline_comment:{
                                break;
                            }
                            default:{
                                this->out->putc(c);
                                break;
                            }
                        }
                        break;
                    }
                    case '/': {
                        switch (comment_state) {
                            case plain: {
                                comment_state = comment_start;
                                break;
                            }
                            case comment_start: {
                                comment_state = line_comment;
                                break;
                            }
                            case multiline_end: {
                                comment_state = plain;
                                break;
                            }
                            case line_comment:
                            case multiline_comment: {
                                break;
                            }
                            default: {
                                this->out->putc(c);
                            }
                        }
                        break;
                    }
                    case '*': {
                        switch (comment_state) {
                            case comment_start: {
                                comment_state = multiline_comment;
                                break;
                            }
                            case multiline_end: {
                                break;
                            }
                            case line_comment: {
                                break;
                            }
                            case multiline_comment: {
                                comment_state = multiline_end;
                                break;
                            }
                            default: {
                                this->out->putc(c);
                            }   
                        }
                        break;
                    }
                    case '"': {
                        if (comment_state == plain) {
                            comment_state = quote;
                        } else if (comment_state == quote) {
                            comment_state = plain;
                        }
                        break;
                    }
                    default: {
                        switch (comment_state) {
                            case comment_start: {
                                this->out->putc(prev_c);
                                this->out->putc(c);
                                comment_state = plain;
                                break;
                            }
                            case multiline_end: {
                                comment_state = multiline_comment;
                                break;
                            }
                            case multiline_comment:
                            case line_comment: {
                                break;
                            }
                            default: {
                                this->out->putc(c);
                            }
                        }
                    }
                }   
            } else if (c != '\r') { // line continuation support on windows
                this->out->putc(c);
                this->escape = false;
            }
            this->prev_c = c;
        };
};


class newlines_to_spaces: public filter {
    public:
        void putc(char c){
            if (c == '\r' || c == '\n'){
                c = SPACE_CHAR;
            }
            this->out->putc(c);
        };
};


class normalize_spaces: public filter {
    private:
        bool skip_next_space = false;
    
    public:

        void putc(char c){
            if (std::isblank(c)) {
                if (!this->skip_next_space) {
                    this->out->putc(SPACE_CHAR);
                    this->skip_next_space = true;
                } 
            } else {
                this->skip_next_space = false;
                this->out->putc(c);
            }
        };
};


class remove_trailing_spaces: public filter {
    private:
        int trailing_spaces = 0;

    public:
        void putc(char c){
            if (std::isblank(c)) {
                this->trailing_spaces++;
            } else if (c == '\n' || c == '\r'){
                this->trailing_spaces = 0;
                this->out->putc(c);
            } else {
                for (int j = 0; j < trailing_spaces; j++){
                    this->out->putc(SPACE_CHAR);
                }
                this->trailing_spaces = 0;
                this->out->putc(c);
            }
        };
};


int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "\nUsage:\t"<< argv[0] << "\t<input_directory>\t<output_file>\t<charmap_file>\t[<options...>]\n";
        exit(1);
    }

    ArgParser args(argv + 4, argv + argc);

    // (\h+)       -> ' '
    bool do_normalize_spaces = args.cmdOptionExists("-ns");
    // (\h+\r?\n)  -> ''
    bool do_remove_trailing_spaces = args.cmdOptionExists("-ntr");
    // (\r?\n)     -> '\n'
    bool do_normalize_newlines = args.cmdOptionExists("-nl");
    // (\r?\n)     -> ' '
    bool delcmts = args.cmdOptionExists("--delete-comments");
    bool do_newlines_to_spaces = args.cmdOptionExists("-nl2s");
    bool eof = args.cmdOptionExists("-eof");
    bool debug = args.cmdOptionExists("--debug");
    bool verbose = args.cmdOptionExists("-v");
    bool symlink = args.cmdOptionExists("--symlinks");
    std::optional<std::vector<std::string>> file_extensions = args.getCmdArgs("--extensions");
    std::optional<std::string> linemap_file = args.getCmdArg("--linemap");

    std::string out_file = argv[2];
    std::string charmap_file = argv[3];

    std::ofstream out(out_file, std::ofstream::binary);
    std::ofstream charmap(charmap_file);
    std::optional<std::ofstream> linemap;

    if (linemap_file) {
        linemap.emplace(*linemap_file);

        if (!*linemap) {
            std::cerr << "linemap output file open fails. exit.\n";
            exit(1);
        }
    }

    if (!out) {
        std::cerr << "output file open fails. exit.\n";
        exit(1);
    }

    if (!charmap) {
        std::cout << "charmap file open fails. exit.\n";
        exit(1);
    }

    std::cout << "Looking up files in " << argv[1] << "\n";
    std::set<fs::directory_entry> files;   // directory iteration order is unspecified, so we sort all the paths for consistent behaviour
    std::filesystem::directory_options options = symlink
            ? fs::directory_options::follow_directory_symlink
            : fs::directory_options::none;
    for (const auto &entry : fs::recursive_directory_iterator(argv[1], options)) {
        if (entry.is_regular_file()) {
            const auto &path = entry.path();
            if (!file_extensions || std::any_of(file_extensions->begin(), file_extensions->end(),
                                                [path](auto ext) { return endsWith(path.string(), "." + ext); })) {
                if (verbose) {
                    std::cout << path << std::endl;
                }
                files.insert(entry);
            }
        }
    }

    std::cout << "Processing files\n";
    for (const fs::directory_entry &file : files) {
        std::ifstream in(file.path());
        if (verbose) {
            std::cout << "opening input file " << file << "\n";
        }

        if (!in) {
            std::cerr << "input file " << file << " open fails. exit.\n";
            exit(1);
        }

        charmap << out.tellp() << "\t" << file.path().string() << "\n";
        if (linemap) {
            *linemap << out.tellp() << "\t" << 1 << "\n";
        }

        if (debug) {
            out << "==================" << file << "==================\n";
        }

        wrap_ostream wo = wrap_ostream(out);
        sink *out_sink = &wo;
        remove_trailing_spaces rts = remove_trailing_spaces();
        normalize_spaces ns = normalize_spaces();
        newlines_to_spaces n2s = newlines_to_spaces();
        remove_cstyle_comments rsc = remove_cstyle_comments();
        normalize_newlines nn = normalize_newlines();
        if (do_remove_trailing_spaces){
            out_sink = (rts >>= out_sink);
        }

        if (do_normalize_spaces){
            out_sink = (ns >>= out_sink);
        }

        if (do_newlines_to_spaces){
            out_sink = (n2s >>= out_sink);
        }

        if (delcmts){
            out_sink = (rsc >>= out_sink);
        }

        if (do_normalize_newlines){
            out_sink = (nn >>= out_sink);
        }

        char c;
        unsigned long line_nb = 1;
        while(in.get(c)){
            if (line_nb == 1){
                    *linemap << out.tellp() << "\t" << 1 << "\n";
            }

            if (c=='\n'){
                    *linemap << out.tellp() << "\t" << line_nb << "\n";
                    line_nb++;
            }
            out_sink->putc(c);
        }
        out.flush();

        in.close();
    }

    charmap << out.tellp() << "\t\n";   // blank file name == end
//    fs::resize_file(out_file, out.tellp()); // pubseekoff operations can lead to ghost data
    out.close();
    charmap.close();
    if (linemap) {
        linemap->close();
    }
    std::cout << "\nDone!\n";
}

