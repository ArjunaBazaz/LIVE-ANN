#pragma once

#include "../types.h"

#include <filesystem>

namespace sift {

// Load the same packed file into independently mutable, mutex-protected clusters.
// All loaded points start live, with empty tombstone free lists.
std::unique_ptr<Metadata_Coarse> load_l2_coarse_index(const std::filesystem::path& packed_file);

} // namespace sift
