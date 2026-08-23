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

#include <chrono>
#include <type_traits>
#include <functional>


template<class R, typename T>
struct benchmark_result {
	R return_value;
	T computational_time;

	benchmark_result(R&& return_value_par, const T& computational_time_par)
		: return_value(std::move(return_value_par)),
		computational_time(computational_time_par) {
	}

	/**
	 * @brief Returns the computational time in [T] calling T.count()
	 * @return computational time
	*/
	unsigned long count() {
		return static_cast<long>(computational_time.count());
	}
};

template<
	typename T = std::chrono::milliseconds,
	typename F,
	typename ...A,
	typename R = std::invoke_result_t<F, A...>,
	std::enable_if_t<std::is_void_v<R>, int> = 0
>
static T benchmark(F&& func, A&&... args)
{
	const auto start = std::chrono::high_resolution_clock::now();
	std::invoke(std::forward<F>(func), std::forward<A>(args)...);
	return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now() - start);
}

template<
	typename T = std::chrono::milliseconds,
	typename F,
	typename ...A,
	typename R = std::invoke_result_t<F, A...>,
	std::enable_if_t<!std::is_void_v<R>, int> = 0
>
static benchmark_result<R, T> benchmark(F&& func, A&&... args)
{
	const auto start = std::chrono::high_resolution_clock::now();

	return benchmark_result{
		std::invoke(std::forward<F>(func), std::forward<A>(args)...),
		std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now() - start)
	};
}
