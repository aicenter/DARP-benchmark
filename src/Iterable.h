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

#include <iterator>
#include <type_traits>
#include <utility>
#include <boost/iterator/indirect_iterator.hpp>

/**
 * Concept defining iterator of specific element
 */
template<class I, typename E>
concept Iterator = std::input_iterator<I>
	&& requires(const I i) {{*i} -> std::convertible_to<E>;};

/*
 * Concept defining iterable of specific element
 */
template<class I, typename E>
concept Iterable = requires(const I i) {
	{i.begin()} -> Iterator<E>;
	{i.end()} -> std::sentinel_for<decltype(i.begin())>;
};

static_assert(Iterable<std::vector<int>,int>);
static_assert(Iterable<std::vector<int*>,int*>);

template<class E, Iterable<E*> I>
class Indirect_iterable {

	using iterator_type = boost::indirect_iterator<typename I::iterator>;

	using const_iterator_type = boost::indirect_iterator<typename I::const_iterator>;

public:
	explicit Indirect_iterable(const I& iterable)
		: iterable(iterable) {
	}

	const_iterator_type begin() const;

	const_iterator_type end() const;

	size_t size() const;
	
private:
	const I& iterable;
};

// deduction guide
template<class I> Indirect_iterable(const I& iterable)
	-> Indirect_iterable<std::remove_pointer_t<typename I::value_type>,I>;



#include "Iterable.tpp"
