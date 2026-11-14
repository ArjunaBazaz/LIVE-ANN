#pragma once

#include <array>
#include <vector>
#include <shared_mutex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

// Dimensionality of the vectors in the database
inline constexpr std::size_t dimension = 128;

// Number of total points
inline constexpr std::size_t point_count = 1000000;

// Number of total centroids
inline constexpr std::size_t centroid_count = 256;

// The number of clusters to examine for the closest point
inline constexpr std::size_t k_count = 4;

// Datatype for the vector points
using Point = std::array<float, dimension>;

// Array of points representing a cluster of set of clusters
struct Points {
    std::array<Point, point_count> points;
};

// Metadata of the clusters
// Centroids stores the centroid points for each cluster
// segments stores <offset, length> of each cluster in the packed Points array
struct Metadata {
    std::array<Point, centroid_count> centroids;
    std::array<std::pair<std::uint32_t, std::uint32_t>, centroid_count> segments;
};

// Structure for Centroid with coarsed grained locking
struct Centroid_Coarse {
    std::vector<Point> points;
    std::vector<bool> tombstones;
    std::vector<int> tombstone_indexes;
    std::shared_mutex centroid_mutex;
    int tombstone_count = 0;
};

struct Metadata_Coarse {
    std::vector<std::pair<Point, Centroid_Coarse>> data;
    explicit Metadata_Coarse(std::size_t count = centroid_count) : data(count) {}
};

enum class RequestType : std::uint8_t {
    Query,
    Insert,
    Delete
};

struct Operation {
    RequestType type;
    Point point;
};

// Owns a persistent device index. Construct/upload before starting a benchmark.
// heap_bytes is a minimum device-heap budget; 0 selects 75% of currently free
// device memory. Configure the largest needed budget before the first index.
// CUDA's device heap cannot grow after kernels using it have been loaded.
class Metadata_GPU {
public:
    explicit Metadata_GPU(const Metadata_Coarse& source, std::size_t heap_bytes = 0);
    ~Metadata_GPU();
    Metadata_GPU(Metadata_GPU&&) noexcept;
    Metadata_GPU& operator=(Metadata_GPU&&) noexcept;
    Metadata_GPU(const Metadata_GPU&) = delete;
    Metadata_GPU& operator=(const Metadata_GPU&) = delete;

    std::size_t heap_bytes() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend double process_requests(const std::vector<Operation>&, Metadata_GPU&);
};

using Index = std::pair<std::unique_ptr<Points>, Metadata>;
