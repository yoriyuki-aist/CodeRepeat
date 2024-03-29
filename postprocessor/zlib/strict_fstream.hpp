/*
 * zstr
 * Copyright (c) 2015 Matei David, Ontario Institute for Cancer Research
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __STRICT_FSTREAM_HPP
#define __STRICT_FSTREAM_HPP

#include <cassert>
#include <fstream>
#include <cstring>
#include <string>

/**
 * This namespace defines wrappers for std::ifstream, std::ofstream, and
 * std::fstream objects. The wrappers perform the following steps:
 * - check the open modes make sense
 * - check that the call to open() is successful
 * - (for input streams) check that the opened file is peek-able
 * - turn on the badbit in the exception mask
 */
namespace strict_fstream
{

/// Overload of error-reporting function, to enable use with VS.
/// Ref: http://stackoverflow.com/a/901316/717706
static std::string strerror()
{
    std::string buff(80, '\0');
#ifdef _WIN32
    if (strerror_s(&buff[0], buff.size(), errno) != 0)
    {
        buff = "Unknown error";
    }
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
// XSI-compliant strerror_r()
    if (strerror_r(errno, &buff[0], buff.size()) != 0)
    {
        buff = "Unknown error";
    }
#else
// GNU-specific strerror_r()
    // Looks like a bug
    strerror_r(errno, &buff[0], buff.size());
    // auto p = strerror_r(errno, &buff[0], buff.size());
    // std::string tmp(p, std::strlen(p));
    // std::swap(buff, tmp);
#endif
    buff.resize(buff.find('\0'));
    return buff;
}

/// Exception class thrown by failed operations.
class Exception
    : public std::exception
{
public:
    Exception(const std::string& msg) : _msg(msg) {}
    const char * what() const noexcept { return _msg.c_str(); }
private:
    std::string _msg;
}; // class Exception

namespace detail
{

struct static_method_holder
{
    static std::string mode_to_string(std::ios_base::openmode mode)
    {
        static const int n_modes = 6;
        static const std::ios_base::openmode mode_val_v[n_modes] =
            {
                std::ios_base::in,
                std::ios_base::out,
                std::ios_base::app,
                std::ios_base::ate,
                std::ios_base::trunc,
                std::ios_base::binary
            };

        static const char * mode_name_v[n_modes] =
            {
                "in",
                "out",
                "app",
                "ate",
                "trunc",
                "binary"
            };
        std::string res;
        for (int i = 0; i < n_modes; ++i)
        {
            if (mode & mode_val_v[i])
            {
                res += (! res.empty()? "|" : "");
                res += mode_name_v[i];
            }
        }
        if (res.empty()) res = "none";
        return res;
    }
    static void check_mode(const std::string& filename, std::ios_base::openmode mode)
    {
        if ((mode & std::ios_base::trunc) && ! (mode & std::ios_base::out))
        {
            throw Exception(std::string("strict_fstream: open('") + filename + "'): mode error: trunc and not out");
        }
        else if ((mode & std::ios_base::app) && ! (mode & std::ios_base::out))
        {
            throw Exception(std::string("strict_fstream: open('") + filename + "'): mode error: app and not out");
        }
        else if ((mode & std::ios_base::trunc) && (mode & std::ios_base::app))
        {
            throw Exception(std::string("strict_fstream: open('") + filename + "'): mode error: trunc and app");
        }
     }
    static void check_open(std::ios * s_p, const std::string& filename, std::ios_base::openmode mode)
    {
        if (s_p->fail())
        {
            throw Exception(std::string("strict_fstream: open('")
                            + filename + "'," + mode_to_string(mode) + "): open failed: "
                            + strerror());
        }
    }
    static void check_peek(std::istream * is_p, const std::string& filename, std::ios_base::openmode mode)
    {
        bool peek_failed = true;
        try
        {
            is_p->peek();
            peek_failed = is_p->fail();
        }
        catch (std::ios_base::failure e) {}
        if (peek_failed)
        {
            throw Exception(std::string("strict_fstream: open('")
                            + filename + "'," + mode_to_string(mode) + "): peek failed: "
                            + strerror());
        }
        is_p->clear();
    }
}; // struct static_method_holder

} // namespace detail

class ifstream
    : public std::ifstream
{
public:
    ifstream() = default;
    ifstream(const std::string& filename, std::ios_base::openmode mode = std::ios_base::in)
    {
        open(filename, mode);
    }
    void open(const std::string& filename, std::ios_base::openmode mode = std::ios_base::in)
    {
        mode |= std::ios_base::in;
        exceptions(std::ios_base::badbit);
        detail::static_method_holder::check_mode(filename, mode);
        std::ifstream::open(filename, mode);
        detail::static_method_holder::check_open(this, filename, mode);
        detail::static_method_holder::check_peek(this, filename, mode);
    }
}; // class ifstream

class ofstream
    : public std::ofstream
{
public:
    ofstream() = default;
    ofstream(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out)
    {
        open(filename, mode);
    }
    void open(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out)
    {
        mode |= std::ios_base::out;
        exceptions(std::ios_base::badbit);
        detail::static_method_holder::check_mode(filename, mode);
        std::ofstream::open(filename, mode);
        detail::static_method_holder::check_open(this, filename, mode);
    }
}; // class ofstream

class fstream
    : public std::fstream
{
public:
    fstream() = default;
    fstream(const std::string& filename, std::ios_base::openmode mode = std::ios_base::in)
    {
        open(filename, mode);
    }
    void open(const std::string& filename, std::ios_base::openmode mode = std::ios_base::in)
    {
        if (! (mode & std::ios_base::out)) mode |= std::ios_base::in;
        exceptions(std::ios_base::badbit);
        detail::static_method_holder::check_mode(filename, mode);
        std::fstream::open(filename, mode);
        detail::static_method_holder::check_open(this, filename, mode);
        detail::static_method_holder::check_peek(this, filename, mode);
    }
}; // class fstream

} // namespace strict_fstream

#endif
