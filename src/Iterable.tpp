

template <class E, Iterable<E*> I>
typename Indirect_iterable<E, I>::const_iterator_type Indirect_iterable<E, I>::begin() const {
	return const_iterator_type(iterable.begin());
}

template <class E, Iterable<E*> I>
typename Indirect_iterable<E, I>::const_iterator_type Indirect_iterable<E, I>::end() const {
	return const_iterator_type(iterable.end());
}

template <class E, Iterable<E*> I>
size_t Indirect_iterable<E, I>::size() const {
	return iterable.size();
}

