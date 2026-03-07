//
// Created by Fido on 2024-12-30.
//

#include <filesystem>

#pragma once

namespace fs = std::filesystem;

fs::path get_running_executable_path();

fs::path get_resource_path(const fs::path& relative_path);


