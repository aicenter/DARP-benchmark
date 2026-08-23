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

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include "common.h"

template<class E>
class Linked_hash_set {
	using container_type = boost::multi_index_container<
		E,
		boost::multi_index::indexed_by<
			boost::multi_index::sequenced<>,
			boost::multi_index::hashed_unique<
				boost::multi_index::identity<E>,
				std::hash<E>,
				std::equal_to<>
			>
		>
	>;

public:
    Linked_hash_set() = default;

	Linked_hash_set(std::initializer_list<E> elements): container(elements){}

	void insert(E&& element) {
		container.template get<1>().insert(element);
	}

	void insert(const E& element) {
		container.template get<1>().insert(element);
	}

	void erase(E element) {
		container.template get<1>().erase(element);
	}

	[[nodiscard]] bool contains(E element) const {
		return container.template get<1>().contains(element);
	}

    [[nodiscard]] size_t size() const {
		return container.size();
	}

    [[nodiscard]] bool empty() const {
		return container.empty();
	}

	void clear() {
		container.clear();
	}

	[[nodiscard]] typename container_type::iterator begin() const {
		return container.begin();
	}

    [[nodiscard]] typename container_type::iterator end() const {
		return container.end();
	}

	bool operator==(const Linked_hash_set<E>& other) const{
        return container.template get<1>() == other.container.template get<1>();
    }

    size_t hash() const {
		std::hash<E> hash_f;
        size_t final_hash{ 0 };
        for (const E& element : container) {
            final_hash ^= hash_f(element);
        }
        return final_hash;
	}
private:
	container_type container;
};
//static_assert(std::is_same_v<std::iter_value_t<Linked_hash_set<const int*>>, const int>);
static_assert(Pointer_iterable<Linked_hash_set<const int*>, int>);