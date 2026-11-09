#pragma once

#include "../types.h"

#include <filesystem>

namespace sift {

Index load_l2_index(const std::filesystem::path& packed_file);

} // namespace sift
