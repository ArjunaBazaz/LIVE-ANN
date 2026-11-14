#include "loader.h"

#include <fstream>
#include <stdexcept>

namespace sift {

constexpr std::size_t points_bytes = point_count * dimension * sizeof(float);
constexpr std::size_t centroids_bytes = centroid_count * dimension * sizeof(float);
constexpr std::size_t segments_bytes = centroid_count * 2 * sizeof(std::uint32_t);
constexpr std::size_t packed_bytes = points_bytes + centroids_bytes + segments_bytes;

// Reads the input file for the clusters.
std::ifstream open_input(const std::filesystem::path& path, std::size_t expected_bytes) {
    if (std::filesystem::file_size(path) != expected_bytes) {
        throw std::runtime_error("Unexpected file size: " + path.string());
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open: " + path.string());
    return input;
}

// Reads the bytes of a file.
void read_exact(std::istream& input, void* data, std::size_t bytes) {
    input.read(static_cast<char*>(data), static_cast<std::streamsize>(bytes));
    if (!input) throw std::runtime_error("Failed reading packed index/input.");
}

std::unique_ptr<Metadata_Coarse> load_l2_coarse_index(const std::filesystem::path& packed_file) {
    auto input = open_input(packed_file, packed_bytes);
    Metadata packed_metadata;
    input.seekg(static_cast<std::streamoff>(points_bytes));
    read_exact(input, packed_metadata.centroids.data(), centroids_bytes);
    std::size_t end = 0;
    for (auto& segment : packed_metadata.segments) {
        read_exact(input, &segment.first, sizeof(segment.first));
        read_exact(input, &segment.second, sizeof(segment.second));
        end += segment.second;
    }

    auto metadata = std::make_unique<Metadata_Coarse>(centroid_count);
    input.seekg(0);
    for (std::size_t c = 0; c < centroid_count; c++) {
        auto& entry = metadata->data[c];
        entry.first = packed_metadata.centroids[c];
        auto& cluster = entry.second;
        const auto length = packed_metadata.segments[c].second;
        cluster.points.resize(length);
        cluster.tombstones.assign(length, false);
        if (length != 0) read_exact(input, cluster.points.data(), length * sizeof(Point));
    }
    return metadata;
}

} // namespace sift
