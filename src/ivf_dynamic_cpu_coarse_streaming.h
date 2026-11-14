#pragma once

#include "distance.h"

#include <vector>

namespace ivf_dynamic_cpu_coarse_streaming {

// Returns the nearest squared distance, holding one cluster shared lock at a time.
// Returns infinity if no live point exists in the selected clusters.
double search(const Point& query, Metadata_Coarse& metadata);

void insert(const Point& query, Metadata_Coarse& metadata);

// Erases the first exact live match in the nearest cluster
// Does nothing when point is not present
void erase(const Point& query, Metadata_Coarse& metadata);

// Returns the sum of nearest squared distances for search operations only.
double process_requests(const std::vector<Operation>& requests, Metadata_Coarse& metadata);

} // namespace ivf_dynamic_cpu_coarse_streaming
