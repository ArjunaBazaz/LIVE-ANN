#include "distance.h"

#include <cmath>

double L2_squared(const Point& point1, const Point& point2) {
    double result = 0;
    for (std::size_t i = 0; i < point1.size(); ++i) {
        const double difference = static_cast<double>(point1[i]) - point2[i];
        result += difference * difference;
    }
    return result;
}

float distance_L2(const Point& point1, const Point& point2) {
    return static_cast<float>(std::sqrt(L2_squared(point1, point2)));
}
