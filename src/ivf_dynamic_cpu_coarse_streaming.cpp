#include "ivf_dynamic_cpu_coarse_streaming.h"

#include <algorithm>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace ivf_dynamic_cpu_coarse_streaming {
namespace {

// Caller holds metadata.centroids_mutex in shared or exclusive mode.
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

double search(const Point& query, Metadata_Coarse& metadata) {
    std::vector<std::pair<double, std::size_t>> ranked;
    ranked.reserve(metadata.data.size());
    for (std::size_t c = 0; c < metadata.data.size(); c++) {
        ranked.emplace_back(L2_squared(query, metadata.data[c].first), c);
    }
    const auto probes = std::min(k_count, ranked.size());
    std::partial_sort(ranked.begin(), ranked.begin() + probes, ranked.end());

    double best = std::numeric_limits<double>::infinity();
    for (std::size_t rank = 0; rank < probes; rank++) {
        auto& cluster = metadata.data[ranked[rank].second].second;
        std::shared_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);
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
    return best;
}

void insert(const Point& query, Metadata_Coarse& metadata) {
    auto& cluster = metadata.data[closest_centroid(query, metadata)].second;
    std::unique_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);
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

void erase(const Point& query, Metadata_Coarse& metadata) {
    auto& cluster = metadata.data[closest_centroid(query, metadata)].second;
    std::unique_lock<std::shared_mutex> cluster_lock(cluster.centroid_mutex);
    for (std::size_t i = 0; i < cluster.points.size(); ++i) {
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
        try {
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
        } catch (...) {
            std::lock_guard<std::mutex> lock(failure_mutex);
            if (!failure) failure = std::current_exception();
        }
    }
    if (failure) std::rethrow_exception(failure);
    return total_squared_distance;
}

} // namespace ivf_dynamic_cpu_coarse_streaming
