//
// Created by Fido on 2023-10-23.
//

template<typename T>
bool nodes_equal(T lhs, T rhs) {
	return &lhs == &rhs;
}

template<std::integral T>
bool nodes_equal(T lhs, T rhs) {
	return lhs == rhs;
}
