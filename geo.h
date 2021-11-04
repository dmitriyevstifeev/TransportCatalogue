#pragma once

#include <cmath>
#include <functional>

namespace geo
  {
    struct Coordinates {
      double lat = 0.0;
      double lng = 0.0;
    };

    inline const double EPSILON = 1e-6;
    inline bool operator==(const Coordinates &lhs, const Coordinates &rhs) {
      return (std::abs(std::abs(lhs.lat) - std::abs(rhs.lat)) < EPSILON
          && std::abs(std::abs(lhs.lng) - std::abs(rhs.lng)) < EPSILON);
    }

    struct CoordinatesHash {
      size_t operator()(const Coordinates &coordinates) const {
        const size_t latitude = d_hasher_(coordinates.lat);
        const size_t longitude = d_hasher_(coordinates.lng);
        return latitude + longitude * 37;
      }

     private:
      std::hash<double> d_hasher_;
    };

    inline double ComputeDistance(Coordinates from, Coordinates to) {
      using namespace std;
      static const double dr = 3.1415926535 / 180.;
      return acos(sin(from.lat * dr) * sin(to.lat * dr)
                      + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
          * 6371000;
    }

  }
