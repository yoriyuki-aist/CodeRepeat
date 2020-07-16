#pragma once

#include <fstream>

#include "stringescape.h"

unsigned int utf8ToCodepoint(const char *&s, const char *e) {
    const unsigned int REPLACEMENT_CHARACTER = 0xFFFD;

    unsigned int firstByte = static_cast<unsigned char>(*s);

    if (firstByte < 0x80)
        return firstByte;

    if (firstByte < 0xE0) {
        if (e - s < 2)
            return REPLACEMENT_CHARACTER;

        unsigned int calculated =
                ((firstByte & 0x1F) << 6) | (static_cast<unsigned int>(s[1]) & 0x3F);
        s += 1;
        // oversized encoded characters are invalid
        return calculated < 0x80 ? REPLACEMENT_CHARACTER : calculated;
    }

    if (firstByte < 0xF0) {
        if (e - s < 3)
            return REPLACEMENT_CHARACTER;

        unsigned int calculated = ((firstByte & 0x0F) << 12) |
                                  ((static_cast<unsigned int>(s[1]) & 0x3F) << 6) |
                                  (static_cast<unsigned int>(s[2]) & 0x3F);
        s += 2;
        // surrogates aren't valid codepoints itself
        // shouldn't be UTF-8 encoded
        if (calculated >= 0xD800 && calculated <= 0xDFFF)
            return REPLACEMENT_CHARACTER;
        // oversized encoded characters are invalid
        return calculated < 0x800 ? REPLACEMENT_CHARACTER : calculated;
    }

    if (firstByte < 0xF8) {
        if (e - s < 4)
            return REPLACEMENT_CHARACTER;

        unsigned int calculated = ((firstByte & 0x07) << 18) |
                                  ((static_cast<unsigned int>(s[1]) & 0x3F) << 12) |
                                  ((static_cast<unsigned int>(s[2]) & 0x3F) << 6) |
                                  (static_cast<unsigned int>(s[3]) & 0x3F);
        s += 3;
        // oversized encoded characters are invalid
        return calculated < 0x10000 ? REPLACEMENT_CHARACTER : calculated;
    }

    return REPLACEMENT_CHARACTER;
}

static const char hex2[] = "000102030405060708090a0b0c0d0e0f"
                           "101112131415161718191a1b1c1d1e1f"
                           "202122232425262728292a2b2c2d2e2f"
                           "303132333435363738393a3b3c3d3e3f"
                           "404142434445464748494a4b4c4d4e4f"
                           "505152535455565758595a5b5c5d5e5f"
                           "606162636465666768696a6b6c6d6e6f"
                           "707172737475767778797a7b7c7d7e7f"
                           "808182838485868788898a8b8c8d8e8f"
                           "909192939495969798999a9b9c9d9e9f"
                           "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
                           "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
                           "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
                           "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
                           "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
                           "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

static void appendRaw(std::ostream &out, unsigned ch) {
    out << static_cast<char>(ch);
}

static void appendHex(std::ostream &out, unsigned ch) {
    const unsigned int hi = (ch >> 8u) & 0xff;
    const unsigned int lo = ch & 0xff;
    out << "\\u";
    out << hex2[2 * hi];
    out << hex2[2 * hi + 1];
    out << hex2[2 * lo];
    out << hex2[2 * lo + 1];
}

void write_escaped_string(std::ostream &out, const std::string &str) {
    out << '"';
    char const *value = str.c_str();
    char const *end = value + str.size();

    for (const char *c = value; c != end; ++c) {
        switch (*c) {
            case '\"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default: {
#ifdef EMIT_UTF_8_JSON
                unsigned codepoint = static_cast<unsigned char>(*c);
                if (std::iscntrl(codepoint)) {
                    appendHex(out, codepoint);
                } else {
                    appendRaw(out, codepoint);
                }
#else
                unsigned codepoint = utf8ToCodepoint(c, end); // modifies `c`
                if (std::iscntrl((unsigned char) codepoint)) {
                    appendHex(out, codepoint);
                } else if (codepoint < 0x80) {
                    appendRaw(out, codepoint);
                } else if (codepoint < 0x10000) {
                    // Basic Multilingual Plane
                    appendHex(out, codepoint);
                } else {
                    // Extended Unicode. Encode 20 bits as a surrogate pair.
                    codepoint -= 0x10000;
                    appendHex(out, 0xd800 + ((codepoint >> 10) & 0x3ff));
                    appendHex(out, 0xdc00 + (codepoint & 0x3ff));
                }
#endif
                break;
            }
        }
    }
    out << '"';
}