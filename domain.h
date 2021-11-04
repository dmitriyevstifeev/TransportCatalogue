#pragma once

#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "geo.h"

namespace transcat
  {
    struct Stop {
      std::string name;
      geo::Coordinates coords;
    };

    struct StopHash {
      size_t operator()(const Stop *stop) const {
        if (stop == nullptr) {
          return 0;
        }
        const size_t latitude_h = d_hasher_(stop->coords.lat);
        const size_t longitude_h = d_hasher_(stop->coords.lng);
        const size_t stop_name_h = s_hasher_(stop->name);
        return stop_name_h + latitude_h * 37 + longitude_h * 37 * 37;
      }

      size_t operator()(std::pair<const Stop *, const Stop *> stops) const {
        size_t longitude_1h = 0;
        size_t stop_name_1h = 0;
        size_t latitude_1h = 0;
        size_t latitude_2h = 0;
        size_t longitude_2h = 0;
        size_t stop_name_2h = 0;

        if (stops.first != nullptr) {
          latitude_1h = d_hasher_(stops.first->coords.lat);
          longitude_1h = d_hasher_(stops.first->coords.lng);
          stop_name_1h = s_hasher_(stops.first->name);
        }
        if (stops.second != nullptr) {
          latitude_2h = d_hasher_(stops.second->coords.lat);
          longitude_2h = d_hasher_(stops.second->coords.lng);
          stop_name_2h = s_hasher_(stops.second->name);
        }
        return stop_name_1h + latitude_1h * 37 + longitude_1h * 37 * 37
            + stop_name_2h * 37 * 37 * 37 + latitude_2h * 37 * 37 * 37 * 37 + longitude_2h * 37 * 37 * 37 * 37 * 37;
      }

      using hash_type = std::hash<std::string_view>;
      size_t operator()(const std::string_view stop_name) const { return hash_type{}(stop_name); }

     private:
      std::hash<double> d_hasher_;
      std::hash<std::string> s_hasher_;
    };

    struct Route {
      bool is_roundtrip;
      std::vector<const Stop *> stops;
    };

    struct RouteInfo {
      bool not_found;
      size_t stops_on_route;
      size_t unique_stops;
      double route_length;
      double curvature;
    };

    struct Bus {
      std::string name;
      Route route;
    };

    struct BusesInfo {
      bool not_found;
      std::set<std::string_view, std::less<>> buses;
    };

    struct BusHash {

      size_t operator()(const Bus *bus) const {
        if (bus == nullptr) {
          return 0;
        }
        const size_t bus_hash = s_hasher_(bus->name);
        return bus_hash;
      }

      using hash_type = std::hash<std::string_view>;
      size_t operator()(const std::string_view bus_name) const { return hash_type{}(bus_name); }

     private:
      std::hash<std::string> s_hasher_;
    };

    enum class QueryType {
      NewStop,
      NewRoute
    };

    enum class RequestType {
      Stop,
      Bus,
      Map,
      Route
    };

    struct JsonInfoQuery {
      QueryType type;
      std::string name;
      std::vector<std::string> route;
      bool is_roundtrip;
      geo::Coordinates coordinates;
      std::vector<std::pair<std::string, int>> road_distances;
    };

    struct StopQuery{
      std::string name;
      geo::Coordinates coordinates;
    };

    struct BusQuery{
      std::string name;
      std::vector<std::string> route;
      bool is_roundtrip;
    };

    struct RoadDistanceQuery{
      std::string name;
      std::vector<std::pair<std::string, int>> road_distances;
    };

    using InfoQuery = std::variant<StopQuery, BusQuery, RoadDistanceQuery>;

    struct Request {
      int id;
      RequestType type;
      std::string name;
      std::string from;
      std::string to;
    };

    struct SerializationSettings {
      std::string file;
    };

    using DistancesBetweenStops = std::unordered_map<std::pair<const Stop *, const Stop *>
                                                     , int
                                                     , StopHash
                                                     , std::equal_to<>>;
    using Buses = std::unordered_map<std::string_view, const Bus *, BusHash, std::equal_to<>>;
    using Stops = std::unordered_map<std::string_view, const Stop *, StopHash, std::equal_to<>>;
    using PassingBuses = std::unordered_map<const Stop *, std::set<std::string_view, std::less<>>, StopHash>;

    enum class ItemType {
      WAIT,
      BUS
    };

  }