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
#pragma once
#include <concepts>
#include <unordered_set>


template<class P, class T>
concept Pointer =
	std::is_same_v<P, T*>
	|| std::is_same_v<P, T* const>
;

template<class I, class T>
concept Pointer_iterator =
	std::forward_iterator<I>
	&& (
		Pointer<std::remove_reference_t<decltype(*std::declval<I>())>,T>
		|| Pointer<std::remove_reference_t<decltype(*std::declval<I>())>,const T>
	)
;

/**
 * Concept for container of pointers
 */
template<class I, typename T>
concept Pointer_iterable = 
	requires(I i){{i.size()} -> std::convertible_to<std::size_t>;}
	&& requires(I i, T t){{i.begin()} -> Pointer_iterator<T>;}
;

//static_assert(Pointer_iterable<std::unordered_set<const int*>, int>);
static_assert(Pointer_iterable<std::vector<const int*>, int>);

template<class T, Pointer_iterable<T> I>
class Pointer_to_reference_iterator {
private:
	const I& iterable;
	
	class Iterator_begin  {
	public:

		explicit Iterator_begin(const I& iterable)
			: real_begin_iterator(iterable.begin()) {
		}

		bool operator!=(const decltype(std::declval<const I&>().end())& rhs) {
			return !(real_begin_iterator == rhs);
		}

		const T& operator*() {
			return **real_begin_iterator;
		}

		Iterator_begin operator++() {
			++real_begin_iterator;
			return *this;
		}

	private:
		decltype(std::declval<const I&>().begin()) real_begin_iterator;
	};

	Iterator_begin begin_iterator;

public:

	explicit Pointer_to_reference_iterator(const I& iterable)
		: iterable(iterable), begin_iterator(iterable) {
	}
	
	Iterator_begin begin() const {
		return begin_iterator;
	}

	auto end() const {
		return iterable.end();
	}

	std::size_t size() const {
		return iterable.size();
	}
};


class ExceptionHandeling {
public:
	static std::string process_unexpected_exception(const std::exception_ptr& eptr = std::current_exception());
private:
	template<typename T>
	static std::string process_nested_exception(const T& e);
};

void replace_all_inplace(
	std::string& string,
	const std::string& replace,
	const std::string& replacement
);

std::string replace_all(
	std::string string,
	const std::string& replace,
	const std::string& replacement
);

/**
 * \brief Template for determining the type at compile time as a part of an error message
 */
template <typename...> struct Get_type;

template<typename T>
bool nodes_equal(T lhs, T rhs);

template<std::integral T>
bool nodes_equal(T lhs, T rhs);

#include "common.tpp"






