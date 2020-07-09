/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch and https://github.com/squix78/json-streaming-parser
*/

#include <string>
#include <iostream>
#include "JsonStreamingParser.h"

JsonStreamingParser::JsonStreamingParser() {
    reset();
}

JsonStreamingParser::JsonStreamingParser(bool emitWhitespaces): doEmitWhitespace(emitWhitespaces) {
    reset();
}

void JsonStreamingParser::reset() {
    state = STATE_START_DOCUMENT;
    buffer.resize(BUFFER_MIN_LENGTH);
    bufferPos = 0;
    unicodeEscapeBufferPos = 0;
    unicodeBufferPos = 0;
    unicodeHighSurrogate = -1;
    characterCounter = 0;
    line = 1;
    charInLine = 1;
}

void JsonStreamingParser::setListener(JsonListener* listener) {
  myListener = listener;
}

void JsonStreamingParser::parse(char c) {
    consumeChar(c);

    if (c == '\n') {
        line++;
        charInLine = 0;
    }

    charInLine++;
    characterCounter++;
  }

void JsonStreamingParser::consumeChar(char c) {
    //System.out.print(c);
    // valid whitespace characters in JSON (from RFC4627 for JSON) include:
    // space, horizontal tab, line feed or new line, and carriage return.
    // thanks:
    // http://stackoverflow.com/questions/16042274/definition-of-whitespace-in-json
    switch (state) {
        case STATE_IN_ARRAY:
        case STATE_IN_OBJECT:
        case STATE_END_KEY:
        case STATE_AFTER_KEY:
        case STATE_AFTER_VALUE:
        case STATE_START_DOCUMENT:
        case STATE_DONE:
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (doEmitWhitespace) {
                    myListener->whitespace(c);
                }
                return;
            }
    }
    switch (state) {
        case STATE_IN_STRING:
            if (c == '"') {
                endString();
            } else if (c == '\\') {
                state = STATE_START_ESCAPE;
            } else if (std::iscntrl(c)) {
                throw ParsingError(std::string("Unescaped control character encountered: ") + c + "(" + std::to_string((int)c) + ") at position" + std::to_string(characterCounter));
            } else {
                buffer[bufferPos] = c;
                increaseBufferPointer();
            }
            break;
        case STATE_IN_ARRAY:
            if (c == ']') {
                endArray();
            } else {
                startValue(c);
            }
            break;
        case STATE_IN_OBJECT:
            if (c == '}') {
                endObject();
            } else if (c == '"') {
                startKey();
            } else {
                throw ParsingError(std::string("Start of string expected for object key. Instead got: ") + c + " at position" + std::to_string(characterCounter));
            }
            break;
        case STATE_END_KEY:
            if (c != ':') {
                throw ParsingError(std::string("Expected ':' after key. Instead got ") + c + " at position" + std::to_string(characterCounter));
            }
            state = STATE_AFTER_KEY;
            break;
        case STATE_AFTER_KEY:
            startValue(c);
            break;
        case STATE_START_ESCAPE:
            processEscapeCharacters(c);
            break;
        case STATE_UNICODE:
            processUnicodeCharacter(c);
            break;
        case STATE_UNICODE_SURROGATE:
            unicodeEscapeBuffer[unicodeEscapeBufferPos] = c;
            unicodeEscapeBufferPos++;
            if (unicodeEscapeBufferPos == 2) {
                endUnicodeSurrogateInterstitial();
            }
            break;
        case STATE_AFTER_VALUE: {
            // not safe for size == 0!!!
            int within = stack[stackPos - 1];
            if (within == STACK_OBJECT) {
                if (c == '}') {
                    endObject();
                } else if (c == ',') {
                    state = STATE_IN_OBJECT;
                } else {
                    throw ParsingError(std::string("Expected ',' or '}' while parsing object. Got: ") + c + ". " + std::to_string(characterCounter));
                }
            } else if (within == STACK_ARRAY) {
                if (c == ']') {
                    endArray();
                } else if (c == ',') {
                    state = STATE_IN_ARRAY;
                } else {
                    throw ParsingError(std::string("Expected ',' or ']' while parsing array. Got: ") + c + ". " + std::to_string(characterCounter));

                }
            } else {
                throw ParsingError(std::string("Finished a literal, but unclear what state to move to. Last state: ") + std::to_string(characterCounter));
            }
        }break;
        case STATE_IN_NUMBER:
            if (c >= '0' && c <= '9') {
                buffer[bufferPos] = c;
                increaseBufferPointer();
            } else if (c == '.') {
                if (doesCharArrayContain(buffer, bufferPos, '.')) {
                    throw ParsingError(std::string("Cannot have multiple decimal points in a number. ") + std::to_string(characterCounter));
                } else if (doesCharArrayContain(buffer, bufferPos, 'e')) {
                    throw ParsingError(std::string("Cannot have a decimal point in an exponent.") + std::to_string(characterCounter));
                }
                buffer[bufferPos] = c;
                increaseBufferPointer();
            } else if (c == 'e' || c == 'E') {
                if (doesCharArrayContain(buffer, bufferPos, 'e')) {
                    throw ParsingError(std::string("Cannot have multiple exponents in a number. ") + std::to_string(characterCounter));
                }
                buffer[bufferPos] = c;
                increaseBufferPointer();
            } else if (c == '+' || c == '-') {
                char last = buffer[bufferPos - 1];
                if (!(last == 'e' || last == 'E')) {
                    throw ParsingError(std::string("Can only have '+' or '-' after the 'e' or 'E' in a number.") + std::to_string(characterCounter));
                }
                buffer[bufferPos] = c;
                increaseBufferPointer();
            } else {
                endNumber();
                // we have consumed one beyond the end of the number
                parse(c);
            }
            break;
        case STATE_IN_TRUE:
            buffer[bufferPos] = c;
            increaseBufferPointer();
            if (bufferPos == 4) {
                endTrue();
            }
            break;
        case STATE_IN_FALSE:
            buffer[bufferPos] = c;
            increaseBufferPointer();
            if (bufferPos == 5) {
                endFalse();
            }
            break;
        case STATE_IN_NULL:
            buffer[bufferPos] = c;
            increaseBufferPointer();
            if (bufferPos == 4) {
                endNull();
            }
            break;
        case STATE_START_DOCUMENT:
            myListener->startDocument();
            if (c == '[') {
                startArray();
            } else if (c == '{') {
                startObject();
            } else {
                throw ParsingError(/*$this->_line_number,
         $this->_char_number,*/
                        "Document must start with object or array.");
            }
            break;
        case STATE_DONE:
            throw ParsingError(/*$this->_line_number, $this->_char_number,*/
                    "Expected end of document.");
        default:
            throw std::logic_error(/*$this->_line_number, $this->_char_number,*/
                    std::string("Internal error. Reached an unknown state: ") + std::to_string(state));
    }
}

void JsonStreamingParser::increaseBufferPointer() {
    bufferPos++;
    if (bufferPos >= buffer.size()) {
        buffer.resize(buffer.size() + BUFFER_MIN_LENGTH);
    }
}

void JsonStreamingParser::endString() {
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped == STACK_KEY) {
      myListener->key(buffer.substr(0, bufferPos));
      state = STATE_END_KEY;
    } else if (popped == STACK_STRING) {
      myListener->value(buffer.substr(0, bufferPos));
      state = STATE_AFTER_VALUE;
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       "Unexpected end of string.");
    }
    bufferPos = 0;
  }
void JsonStreamingParser::startValue(char c) {
    if (c == '[') {
      startArray();
    } else if (c == '{') {
      startObject();
    } else if (c == '"') {
      startString();
    } else if (isDigit(c)) {
      startNumber(c);
    } else if (c == 't') {
      state = STATE_IN_TRUE;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else if (c == 'f') {
      state = STATE_IN_FALSE;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else if (c == 'n') {
      state = STATE_IN_NULL;
      buffer[bufferPos] = c;
      increaseBufferPointer();
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Unexpected character for value: ") + c);
    }
  }

bool JsonStreamingParser::isDigit(char c) {
    // Only concerned with the first character in a number.
    return (c >= '0' && c <= '9') || c == '-';
  }

void JsonStreamingParser::endArray() {
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped != STACK_ARRAY) {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       "Unexpected end of array encountered.");
    }
    myListener->endArray();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
      endDocument();
    }
  }

void JsonStreamingParser::startKey() {
    stack[stackPos] = STACK_KEY;
    stackPos++;
    state = STATE_IN_STRING;
  }

void JsonStreamingParser::endObject() {
    int popped = stack[stackPos - 1];
    stackPos--;
    if (popped != STACK_OBJECT) {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       "Unexpected end of object encountered.");
    }
    myListener->endObject();
    state = STATE_AFTER_VALUE;
    if (stackPos == 0) {
      endDocument();
    }
  }

void JsonStreamingParser::processEscapeCharacters(char c) {
    if (c == '"') {
      buffer[bufferPos] = '"';
      increaseBufferPointer();
    } else if (c == '\\') {
      buffer[bufferPos] = '\\';
      increaseBufferPointer();
    } else if (c == '/') {
      buffer[bufferPos] = '/';
      increaseBufferPointer();
    } else if (c == 'b') {
      buffer[bufferPos] = 0x08;
      increaseBufferPointer();
    } else if (c == 'f') {
      buffer[bufferPos] = '\f';
      increaseBufferPointer();
    } else if (c == 'n') {
      buffer[bufferPos] = '\n';
      increaseBufferPointer();
    } else if (c == 'r') {
      buffer[bufferPos] = '\r';
      increaseBufferPointer();
    } else if (c == 't') {
      buffer[bufferPos] = '\t';
      increaseBufferPointer();
    } else if (c == 'u') {
      state = STATE_UNICODE;
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Expected escaped character after backslash. Got: ") + c);
    }
    if (state != STATE_UNICODE) {
      state = STATE_IN_STRING;
    }
  }

void JsonStreamingParser::processUnicodeCharacter(char c) {
    if (!isHexCharacter(c)) {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Expected hex character for escaped Unicode character. Unicode parsed: ")
       + unicodeBuffer + " and got: " + c);
    }

    unicodeBuffer[unicodeBufferPos] = c;
    unicodeBufferPos++;

    if (unicodeBufferPos == 4) {
      int codepoint = getHexArrayAsDecimal(unicodeBuffer, unicodeBufferPos);

      if (codepoint >= 0xD800 && codepoint < 0xDC00) {
        unicodeHighSurrogate = codepoint;
        unicodeBufferPos = 0;
        state = STATE_UNICODE_SURROGATE;
      } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
        if (unicodeHighSurrogate == -1) {
           throw ParsingError(/*$this->_line_number, $this->_char_number,*/
           "Missing high surrogate for Unicode low surrogate.");
        }

        int combinedCodePoint = ((unicodeHighSurrogate - 0xD800) * 0x400) + (codepoint - 0xDC00) + 0x10000;
        endUnicodeCharacter(combinedCodePoint);
      } else {
          if (unicodeHighSurrogate != -1) {
              throw ParsingError(/*$this->_line_number, $this->_char_number,*/
                                     "Invalid low surrogate following Unicode high surrogate.");
          }

          endUnicodeCharacter(codepoint);
      }
    }
  }
bool JsonStreamingParser::isHexCharacter(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

int JsonStreamingParser::getHexArrayAsDecimal(const char hexArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = hexArray[i];
      int value = 0;
      if (current >= 'a' && current <= 'f') {
        value = current - 'a' + 10;
      } else if (current >= 'A' && current <= 'F') {
        value = current - 'A' + 10;
      } else if (current >= '0' && current <= '9') {
        value = current - '0';
      }
      result = result * 16 + value;
    }
    return result;
  }

bool JsonStreamingParser::doesCharArrayContain(const std::string &myArray, int length, char c) {
    for (int i = 0; i < length; i++) {
      if (myArray[i] == c) {
        return true;
      }
    }
    return false;
  }

void JsonStreamingParser::endUnicodeSurrogateInterstitial() {
    char unicodeEscape = unicodeEscapeBuffer[unicodeEscapeBufferPos - 1];
    if (unicodeEscape != 'u') {
       std::cerr << (/*$this->_line_number, $this->_char_number,*/
       std::string("Expected '\\u' following a Unicode high surrogate. Got: ") +
       unicodeEscape);
    }
    unicodeBufferPos = 0;
    unicodeEscapeBufferPos = 0;
    state = STATE_UNICODE;
  }

void JsonStreamingParser::endNumber() {
    std::string value = buffer.substr(0, bufferPos);
    //float result = 0.0;
    //if (doesCharArrayContain(buffer, bufferPos, '.')) {
    //  result = value.toFloat();
    //} else {
      // needed special treatment in php, maybe not in Java and c
    //  result = value.toFloat();
    //}
    myListener->value(value);
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

int JsonStreamingParser::convertDecimalBufferToInt(const char myArray[], int length) {
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = myArray[length - i - 1];
      result += (current - '0') * 10;
    }
    return result;
  }

void JsonStreamingParser::endDocument() {
    myListener->endDocument();
    state = STATE_DONE;
  }

void JsonStreamingParser::endTrue() {
    std::string value = buffer.substr(0, bufferPos);
    if (value == "true") {
      myListener->value("true");
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Expected 'true'. Got: ") + value);
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::endFalse() {
    std::string value = buffer.substr(0, bufferPos);
    if (value == "false") {
      myListener->value("false");
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Expected 'false'. Got: ") + value);
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::endNull() {
    std::string value = buffer.substr(0, bufferPos);
    if (value == "null") {
      myListener->value("null");
    } else {
       throw ParsingError(/*$this->_line_number, $this->_char_number,*/
       std::string("Expected 'null'. Got: ") + value);
    }
    bufferPos = 0;
    state = STATE_AFTER_VALUE;
  }

void JsonStreamingParser::startArray() {
    myListener->startArray();
    state = STATE_IN_ARRAY;
    stack[stackPos] = STACK_ARRAY;
    stackPos++;
  }

void JsonStreamingParser::startObject() {
    myListener->startObject();
    state = STATE_IN_OBJECT;
    stack[stackPos] = STACK_OBJECT;
    stackPos++;
  }

void JsonStreamingParser::startString() {
    stack[stackPos] = STACK_STRING;
    stackPos++;
    state = STATE_IN_STRING;
  }

void JsonStreamingParser::startNumber(char c) {
    state = STATE_IN_NUMBER;
    buffer[bufferPos] = c;
    increaseBufferPointer();
  }

void JsonStreamingParser::endUnicodeCharacter(int codepoint) {
    convertCodepointToCharacter(codepoint);
    unicodeBufferPos = 0;
    unicodeHighSurrogate = -1;
    state = STATE_IN_STRING;
  }

void JsonStreamingParser::convertCodepointToCharacter(int cp) {
    if (cp <= 0x7f) {
        buffer[bufferPos] = static_cast<char>(cp);
        increaseBufferPointer();
    } else if (cp <= 0x7FF) {
        buffer[bufferPos] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & cp));
        increaseBufferPointer();
    } else if (cp <= 0xFFFF) {
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & cp));
    } else if (cp <= 0x10FFFF) {
        buffer[bufferPos] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        increaseBufferPointer();
        buffer[bufferPos] = static_cast<char>(0x80 | (0x3f & cp));
        increaseBufferPointer();
    }
  }
