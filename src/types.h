#pragma once

#include <array>
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
inline constexpr std::size_t k_count = 8;

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

using Index = std::pair<std::unique_ptr<Points>, Metadata>;
