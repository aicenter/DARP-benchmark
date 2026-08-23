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

#include <sstream>

template<typename T>
std::string format_number(T number) {
    std::string str = std::to_string(number);
    std::stringstream ss;
    for (int i = static_cast<int>(str.length() - 1); i >= 0;) {
        ss << str[i];//inserting 1st digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << str[i];//inserting 2nd digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << str[i];//inserting 3rd digit
        --i;
        if (i == -1)break;//checking if we are out of left bound
        ss << " ";//after 3 digits insertion, finally inserting a dot "."
    }
    str = ss.str();
    reverse(str.begin(), str.end()); //reversing the final string
    return str;
}

//auto fmt::formatter<unsigned>::format(const unsigned value) {
//    return format_number(value);
//}
//
//
//auto fmt::formatter<unsigned long>::format(const unsigned long value) {
//    return format_number(value);
//}
//
//auto fmt::formatter<unsigned long long>::format(const size_t value) {
//    return format_number(value);
//}
