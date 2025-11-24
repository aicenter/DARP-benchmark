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


