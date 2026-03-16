#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

#include "progress_bar.h"
#include "aliases.h"
#include "travel_time_provider/CSV_reader.h"
#include "travel_time_provider/HDF_reader.h"

namespace {

std::unique_ptr<Distance_matrix_reader> make_reader_for_path(const std::string& path) {
	const auto pos = path.find_last_of('.');
	const std::string ext = (pos == std::string::npos) ? std::string{} : path.substr(pos);

	if (ext == ".h5" || ext == ".hdf5" || ext == ".hdf") {
		return std::make_unique<HDF_reader>();
	}

	// Fallback to CSV reader for other extensions
	return std::make_unique<CSV_reader>();
}

int check_triangle_inequality(const std::string& matrix_path) {
	auto reader = make_reader_for_path(matrix_path);

	auto data = reader->read_matrix(matrix_path);
	const unsigned size = data.second;

	if (size == 0u) {
		std::cerr << "Distance matrix has zero size: " << matrix_path << '\n';
		return 1;
	}

	travel_time_type* const dm = data.first.get();

	constexpr unsigned int step = 100;

	indicators::ProgressBar bar{
		indicators::option::BarWidth{70},
		indicators::option::PostfixText{"Checking triangle inequality"},
		indicators::option::MaxProgress{size / step},
        indicators::option::ShowPercentage{true},
		indicators::option::ShowElapsedTime{true},
		indicators::option::ShowRemainingTime{true}
	};

	travel_time_type* base = dm;
	constexpr unsigned block_size = 64;

	struct Violation {
		unsigned i{};
		unsigned j{};
		unsigned k{};
		travel_time_type dik{};
		travel_time_type dij{};
		travel_time_type djk{};
	};

	std::atomic<bool> violation_found{false};
	Violation first_violation{};

	std::mutex progress_mutex;

	const unsigned hw_threads = std::max(1u, std::thread::hardware_concurrency());
	const unsigned num_threads = std::min(hw_threads, size);
	const unsigned rows_per_thread = (size + num_threads - 1) / num_threads;

	auto worker = [&](unsigned i_begin, unsigned i_end) {
		for (unsigned i = i_begin; i < i_end && !violation_found.load(std::memory_order_relaxed); ++i) {
			const auto* row_i = base + static_cast<std::size_t>(i) * size;

			for (unsigned jb = 0; jb < size && !violation_found.load(std::memory_order_relaxed); jb += block_size) {
				const unsigned j_end = std::min(size, jb + block_size);

				for (unsigned kb = 0; kb < size && !violation_found.load(std::memory_order_relaxed); kb += block_size) {
					const unsigned k_end = std::min(size, kb + block_size);

					for (unsigned j = jb; j < j_end && !violation_found.load(std::memory_order_relaxed); ++j) {
						const auto* row_j = base + static_cast<std::size_t>(j) * size;
						const travel_time_type dij = row_i[j];

						for (unsigned k = kb; k < k_end; ++k) {
							const travel_time_type djk = row_j[k];
							const travel_time_type dik = row_i[k];

							const std::uint32_t sum =
								static_cast<std::uint32_t>(dij) + static_cast<std::uint32_t>(djk);

							if (dik > sum) {
								bool expected = false;
								if (violation_found.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
									first_violation = Violation{i, j, k, dik, dij, djk};
								}
								return;
							}
						}
					}
				}
			}

			if ((i % step) == 0) {
				std::lock_guard<std::mutex> lock(progress_mutex);
				bar.tick();
			}
		}
	};

	std::vector<std::thread> threads;
	threads.reserve(num_threads);

	for (unsigned t = 0; t < num_threads; ++t) {
		const unsigned i_begin = t * rows_per_thread;
		if (i_begin >= size) {
			break;
		}
		const unsigned i_end = std::min(size, i_begin + rows_per_thread);
		threads.emplace_back(worker, i_begin, i_end);
	}

	for (auto& th : threads) {
		th.join();
	}

	if (violation_found.load(std::memory_order_relaxed)) {
		const auto v = first_violation;
		std::cerr
			<< "Triangle inequality violated at nodes (i=" << v.i
			<< ", j=" << v.j << ", k=" << v.k << ")\n"
			<< "d(i,k) = " << v.dik
			<< ", d(i,j) = " << v.dij
			<< ", d(j,k) = " << v.djk << '\n'
			<< "Matrix file: " << matrix_path << '\n';
		return 2;
	}

	std::cout << "Triangle inequality holds for all triples in " << matrix_path << '\n';
	return 0;
}

} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: check_triangle_inequality <distance-matrix-file>\n";
		return 1;
	}

	const std::string matrix_path = argv[1];

	try {
		return check_triangle_inequality(matrix_path);
	}
	catch (const std::exception& e) {
		std::cerr << "Error while checking triangle inequality: " << e.what() << '\n';
		return 1;
	}
	catch (...) {
		std::cerr << "Unknown error while checking triangle inequality.\n";
		return 1;
	}
}

