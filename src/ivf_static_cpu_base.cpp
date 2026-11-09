#include "ivf_static_cpu_base.h"

#include <algorithm>
#include <limits>
#include <numeric>

namespace ivf_static_cpu_base {

// Performs a single query. Takes in the address of points and metadata, along with the point to query, and searches for
// the closest point to the point queried.
Point find_closest_point(const Point& query, const Points& points, const Metadata& metadata) {
    std::array<std::size_t, centroid_count> centroid_index;
    std::iota(centroid_index.begin(), centroid_index.end(), 0);
    std::array<double, centroid_count> distances;
    for (std::size_t c = 0; c < centroid_count; c++) {
        distances[c] = L2_squared(query, metadata.centroids[c]);
    }

    std::partial_sort(centroid_index.begin(), centroid_index.begin() + k_count, centroid_index.end(), [&](std::size_t a, std::size_t b) {
        return distances[a] < distances[b] || (distances[a] == distances[b] && a < b);
    });

    double best_distance = std::numeric_limits<double>::infinity();
    std::size_t best_index = points.points.size();
    for (std::size_t rank = 0; rank < k_count; rank++) {
        const auto [offset, length] = metadata.segments[centroid_index[rank]];
        const std::size_t end = static_cast<std::size_t>(offset) + length;
        for (std::size_t i = offset; i < end; i++) {
            const double distance = L2_squared(query, points.points[i]);
            if (distance < best_distance || (distance == best_distance && i < best_index)) {
                best_distance = distance;
                best_index = i;
            }
        }
    }
    return points.points[best_index];
}

// Processes an entire vector of requests.
// Uses single dynamic OpenMP parallelism across points.
std::vector<Point> process_requests(const std::vector<Point>& queries, const Points& points, const Metadata& metadata) {
    std::vector<Point> toReturn(queries.size());

    #pragma omp parallel for schedule(dynamic) default(none) shared(queries, points, metadata, toReturn)
    for(std::size_t i = 0; i < queries.size(); i++) {
        toReturn[i] = find_closest_point(queries[i], points, metadata);
    }
    return toReturn;
}

} // namespace ivf_static_cpu_base
