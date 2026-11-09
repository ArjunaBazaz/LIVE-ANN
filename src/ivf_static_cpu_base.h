#pragma once

#include "distance.h"

#include <vector>

namespace ivf_static_cpu_base {

Point find_closest_point(const Point& query, const Points& points, const Metadata& metadata);

std::vector<Point> process_requests(const std::vector<Point>& queries, const Points& points, const Metadata& metadata);

} // namespace ivf_static_cpu_base
