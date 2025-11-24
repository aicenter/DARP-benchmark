#pragma once

#include <format>

template<typename T>
std::string format_number(T number);

#include "number_formatter.tpp"

//template<>
//struct fmt::formatter<unsigned> {
//	static auto format(const unsigned value);
//};
//
//template<>
//struct fmt::formatter<unsigned long> {
//	static auto format(const unsigned long value);
//};
//
//template<>
//struct fmt::formatter<size_t> {
//	static auto format(const size_t value);
//};

//static_assert(std::For)