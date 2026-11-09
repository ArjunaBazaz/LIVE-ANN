#pragma once

#include "types.h"

// Used to report the distance between two points
float distance_L2(const Point& point1, const Point& point2);

// The distance between two points with taking the sqrt
// Useful to reduce computation time when it is not needed,
// since sqrt is strictly increasing.
double L2_squared(const Point& point1, const Point& point2);
