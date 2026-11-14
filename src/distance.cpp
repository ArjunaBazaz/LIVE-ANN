#include "distance.h"

#include <cmath>

// Calculates the squared distance between two points. Used for ANN calculations.
// L2_squared is separated from L2 since sqrt is strictly increasing, so sqrt is not needed for relative distance.
double L2_squared(const Point& point1, const Point& point2) {
    double result = 0;
    for (std::size_t i = 0; i < point1.size(); ++i) {
        const double difference = static_cast<double>(point1[i]) - point2[i];
        result += difference * difference;
    }
    return result;
}

// Distance between two points.
float distance_L2(const Point& point1, const Point& point2) {
    return static_cast<float>(std::sqrt(L2_squared(point1, point2)));
}
