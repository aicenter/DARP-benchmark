/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>

#include "common.h"


std::string ExceptionHandeling::process_unexpected_exception(const std::exception_ptr& eptr) {
    if (!eptr) {
        throw std::bad_exception();
    }

    /*char* message;*/
    std::string message;
    try {
        std::rethrow_exception(eptr);
    }
    catch (const std::exception& e) {
        message = e.what();
    }
    catch (const std::string& e) {
        message = e;
    }
    catch (const char* e) {
        message = e;
    }
    catch (...) {
        message = "Unknown error";
    }

    spdlog::error(message);
    return message;
}


void replace_all_inplace(std::string& string, const std::string& replace, const std::string& replacement) {
    size_t start_pos = 0;
    while((start_pos = string.find(replace, start_pos)) != std::string::npos) {
        string.replace(start_pos, replace.length(), replacement);
        start_pos += replacement.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::string replace_all(std::string string, const std::string& replace, const std::string& replacement) {
    replace_all_inplace(string, replace, replacement);
    return string;
}


template <typename T>
std::string ExceptionHandeling::process_nested_exception(const T& e) {
    try {
	    std::rethrow_if_nested(e);
    }
    catch (...) {
	    return fmt::format(" ({})", process_unexpected_exception(std::current_exception()));
        //process_unexpected_exception(std::current_exception());
    }
    return {};
}


