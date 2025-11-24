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
