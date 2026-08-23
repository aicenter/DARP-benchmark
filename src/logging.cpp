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
#include <filesystem>
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "logging.h"



void set_up_logger(const fs::path& directory_path) {
	const auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
	console_sink->set_level(spdlog::level::info);

	const std::string log_filepath = fmt::format("{}/log.txt", directory_path.string());
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(log_filepath, true);

	std::initializer_list<spdlog::sink_ptr> sink_list{console_sink, file_sink};
	const auto logger = std::make_shared<spdlog::logger>("DARP Benchmark logger", sink_list);
	logger->set_level(spdlog::level::debug);

	spdlog::set_default_logger(logger);
	spdlog::flush_every(std::chrono::seconds(3));

	spdlog::info("setting up file logging to {}", log_filepath);
}
