#include "ivf_dynamic_cpu_coarse_streaming.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace ivf_dynamic_cpu_coarse_streaming {
namespace {

// Find the closest centroid out of all of the centroids in the metadata.
// Useful for the insert and delete operations.
std::size_t closest_centroid(const Point& query, const Metadata_Coarse& metadata) {
    std::size_t closest = 0;
    double best = L2_squared(query, metadata.data[0].first);
    for (std::size_t c = 1; c < metadata.data.size(); c++) {
        const double distance = L2_squared(query, metadata.data[c].first);
        if (distance < best) {
            closest = c;
            best = distance;
        }
    }
    return closest;
}
} // namespace

// Scans through the top k closest clusters to find the likely closest point.
double search(const Point& query, Metadata_Coarse& metadata) {

    // Find the top k closest centroids.
    std::vector<std::pair<double, std::size_t>> ranked;
    ranked.reserve(metadata.data.size());
    for (std::size_t c = 0; c < metadata.data.size(); c++) {
        ranked.emplace_back(L2_squared(query, metadata.data[c].first), c);
    }
    const auto probes = std::min(k_count, ranked.size());
    std::partial_sort(ranked.begin(), ranked.begin() + probes, ranked.end());

    // Iterates through each cluster to find the closest point
    double best = std::numeric_limits<double>::infinity();
    for (std::size_t rank = 0; rank < probes; rank++) {
        auto& cluster = metadata.data[ranked[rank].second].second;

        // Grabs a read lock when scanning through each cluster
        std::shared_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);

        // Measures the distance for non-tombstone points, and compared to the smallest distance
        for (std::size_t i = 0; i < cluster.points.size(); i++) {
            if (cluster.tombstones[i]) {
                continue;
            }
            const double distance = L2_squared(query, cluster.points[i]);
            if (distance < best) {
                best = distance;
            }
        }
    }

    // Returns the closests distance (squared distance)
    return best;
}

// Insert a point into the database.
void insert(const Point& query, Metadata_Coarse& metadata) {

    // Finds the closest centroid to the point that is being queried
    auto& cluster = metadata.data[closest_centroid(query, metadata)].second;

    // Grabs a unique lock on the cluster, ensures that nothing reads from the cluster is a vector resizing needs to happen.
    // TODO: This only needs a non shared lock with an atomic if memory resizing is needed, can this be exploited, for example using lock ungrading.
    std::unique_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);

    // If there is a tombstone in the array, replace it with the entry
    // If there is no tombstone, append to the end of the list
    if (!cluster.tombstone_indexes.empty()) {
        const int index = cluster.tombstone_indexes.back();
        cluster.points[index] = query;
        cluster.tombstones[index] = false;
        cluster.tombstone_indexes.pop_back();
        cluster.tombstone_count--;
    } else {
        cluster.points.push_back(query);
        cluster.tombstones.push_back(false);
    }
}

// Removes a point from the database.
// When not present, no-op
void erase(const Point& query, Metadata_Coarse& metadata) {

    // Find the closest centroid to the point that is being queried
    auto& cluster = metadata.data[closest_centroid(query, metadata)].second;

    // Grabs a unique lock on the cluster, ensures that nothing reads from a tombstoned entry.
    // TODO: Can this be switched to an atomic? Tombstone planting doesn't change memory at all. Would need to handle inserts
    std::unique_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);

    // Scans through the list to find the point that matched the query point, and insert a tombstone when found.
    for (std::size_t i = 0; i < cluster.points.size(); i++) {
        if (!cluster.tombstones[i] && cluster.points[i] == query) {
            cluster.tombstone_indexes.push_back(static_cast<int>(i));
            cluster.tombstones[i] = true;
            cluster.tombstone_count++;
            return;
        }
    }
}

// Processes an entire vector of requests.
// Dynamic OpenMP scheduling executes requests concurrently, not in input order.
double process_requests(const std::vector<Operation>& requests, Metadata_Coarse& metadata) {
    double total_squared_distance = 0;
    std::exception_ptr failure;
    std::mutex failure_mutex;

    #pragma omp parallel for schedule(dynamic) default(none) shared(requests, metadata, failure, failure_mutex) reduction(+:total_squared_distance)
    for(std::size_t i = 0; i < requests.size(); i++) {
        switch (requests[i].type) {
            case RequestType::Query:
                total_squared_distance += search(requests[i].point, metadata);
                break;
            case RequestType::Insert:
                insert(requests[i].point, metadata);
                break;
            case RequestType::Delete:
                erase(requests[i].point, metadata);
                break;
        }
    }
    return total_squared_distance;
}

} // namespace ivf_dynamic_cpu_coarse_streaming
