//
// Created by Fido on 2024-12-30.
//

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "future-config/resources.h"

fs::path get_running_executable_path() {
	char buffer[1024];
#ifdef _WIN32
	GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
#elif __linux__
	ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
	if (count == -1) throw std::runtime_error("Failed to get executable path");
	buffer[count] = '\0';
#else
	throw std::runtime_error("Unsupported platform");
#endif
	return std::filesystem::path(buffer).parent_path();
}

fs::path get_resource_path(const fs::path& relative_path) {
	return get_running_executable_path() / "data" / relative_path;
}
