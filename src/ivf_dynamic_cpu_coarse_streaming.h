#pragma once

#include "distance.h"

#include <vector>

namespace ivf_dynamic_cpu_coarse_streaming {

// Holds the centroid-table shared lock, then one cluster shared lock at a time.
// Throws if no live point exists in the selected clusters.
Point search(const Point& query, Metadata_Coarse& metadata);

void insert(const Point& query, Metadata_Coarse& metadata);

// Erases the first exact live match in the nearest cluster
// Does nothing when point is not present
void erase(const Point& query, Metadata_Coarse& metadata);

std::vector<Point> process_requests(const std::vector<Operation>& requests, Metadata_Coarse& metadata);

} // namespace ivf_dynamic_cpu_coarse_streaming
