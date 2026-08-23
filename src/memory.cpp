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
#if defined(_MSC_VER)
	#include <Windows.h>
	#include <psapi.h>
#elif defined(__GNUC__)
	#include <sys/resource.h>
#endif

#include "memory.h"
#include <cmath>

unsigned long long get_max_memory_usage() {
	unsigned long long max_mem;

	#if defined(_MSC_VER)
		PROCESS_MEMORY_COUNTERS pmc;
		K32GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		max_mem = pmc.PeakWorkingSetSize;
		max_mem = static_cast<long long>(std::round(static_cast<double>(max_mem) / 1024));
	#elif defined(__GNUC__)
		struct rusage usage;
		getrusage(RUSAGE_SELF, &usage);
		max_mem = usage.ru_maxrss;
	#endif

	return max_mem;
}

