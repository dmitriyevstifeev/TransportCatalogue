#include "transport_catalogue.h"
#include "geo.h"

#include <string>
#include <unordered_set>

namespace transcat
  {
    void TransportCatalogue::AddPassingBus(const Bus *const bus) {
      if (bus == nullptr) {
        return;
      }
      for (const auto *stop : bus->route.stops) {
        stop_passing_buses_[stop].insert(bus->name);
      }
    }

    const Stop *TransportCatalogue::AddNewStop(const Stop &stop) {
      const auto &new_stop = stops_list_.emplace_back(stop);
      stops_dict_.insert({new_stop.name, &new_stop});

      return &new_stop;
    }

    Bus *TransportCatalogue::AddNewBus(const Bus &bus) {
      auto &new_bus = buses_list_.emplace_back(bus);
      buses_dict_.insert({new_bus.name, &new_bus});

      return &new_bus;
    }

    std::pair<DistancesBetweenStops::iterator , bool> TransportCatalogue::InsertStopsDistance(const Stop *stop_from, const Stop *stop_to, int dist) {
      return distance_between_stops_.insert({{stop_from, stop_to}, dist});
    }

    double ComputeGeoLength(const Stop *prev_stop, const Stop *next_stop) {
      if (prev_stop == nullptr || next_stop == nullptr){
        return 0.0;
      }
      return ComputeDistance(prev_stop->coords, next_stop->coords);
    }

    size_t ComputeStopsCountOnRoute(const Route *const route) {
      if (route == nullptr) {
        return 0;
      }
      const auto stops_count = route->stops.size();
      if (stops_count == 0) {
        return 0;
      }

      if (route->is_roundtrip) {
        return stops_count;
      } else {
        return stops_count + stops_count - 1;
      }
    }

    size_t ComputeUniqueStopsCountOnRoute(const Route *const route) {
      if (route == nullptr){
        return 0;
      }
      const auto result = std::unordered_set<const Stop *, StopHash, std::equal_to<>>(route->stops.begin(),
                                                                                      route->stops.end());
      return result.size();
    }

    double ComputeGeoRouteLength(const Route *route) {
      double result = 0.0;
      if (route == nullptr) {
        return result;
      }
      const auto route_size = route->stops.size();
      if (route_size < 2) {
        return result;
      } else {
        for (size_t i = 1; i < route_size; ++i) {
          result += ComputeGeoLength(route->stops[i - 1], route->stops[i]);
        }
        if (!route->is_roundtrip) {
          for (size_t i = route_size - 1; i > 0; --i) {
            result += ComputeGeoLength(route->stops[i], route->stops[i - 1]);
          }
        }

        return result;
      }
    }

    double ComputeFactRouteLength(const Route *const route, const DistancesBetweenStops &distance_between_stops) {
      double result = 0.0;
      if (route == nullptr) {
        return result;
      }
      const auto route_size = route->stops.size();
      if (route_size < 2) {
        return result;
      } else {
        for (size_t i = 1; i < route_size; ++i) {
          result += detail::ComputeFactGeoLength(route->stops[i - 1], route->stops[i], distance_between_stops);
        }
        if (!route->is_roundtrip) {
          for (size_t i = route_size - 1; i > 0; --i) {
            result += detail::ComputeFactGeoLength(route->stops[i], route->stops[i - 1], distance_between_stops);
          }
        }
        return result;
      }
    }

    RouteInfo TransportCatalogue::ComputeRouteInfo(const std::string_view &bus_name) const {
      RouteInfo route_info;
      const Bus *const bus = FindBus(bus_name);
      if (bus == nullptr) {
        route_info.not_found = true;
        return route_info;
      } else {
        route_info.not_found = false;
        const auto *const route = &bus->route;
        route_info.stops_on_route = ComputeStopsCountOnRoute(route);
        route_info.unique_stops = ComputeUniqueStopsCountOnRoute(route);
        route_info.route_length = ComputeFactRouteLength(route, distance_between_stops_);
        const auto geo_length = ComputeGeoRouteLength(route);
        route_info.curvature = (geo_length > 0) ? route_info.route_length / geo_length : 0;
      }

      return route_info;
    }

    BusesInfo TransportCatalogue::ComputeBusInfo(const std::string_view &stop_name) const {
      BusesInfo buses_info;
      const Stop *const stop = FindStop(stop_name);
      if (stop == nullptr) {
        buses_info.not_found = true;
        return buses_info;
      } else {
        buses_info.not_found = false;
        const auto stop_pas_buses_it = stop_passing_buses_.find(stop);
        if (stop_pas_buses_it != stop_passing_buses_.cend()) {
          buses_info.buses = stop_pas_buses_it->second;
        }
      }

      return buses_info;
    }

    const Stop *TransportCatalogue::FindStop(const std::string_view &stop_name) const {
      const auto stop_it = stops_dict_.find(stop_name);
      if (stop_it == stops_dict_.end()) {
        return nullptr;
      } else {
        return stop_it->second;
      }
    }

    const Bus *TransportCatalogue::FindBus(const std::string_view &bus_name) const {
      const auto bus_it = buses_dict_.find(bus_name);
      if (bus_it == buses_dict_.end()) {
        return nullptr;
      } else {
        return bus_it->second;
      }
    }

    Bus *TransportCatalogue::FindCreateBus(const std::string_view &bus_name) {
      const auto bus_it = buses_dict_.find(bus_name);
      if (bus_it == buses_dict_.end()) {
        Bus bus;
        bus.name = std::string(bus_name);
        return AddNewBus(bus);
      } else {
        return const_cast<Bus *>(bus_it->second);
      }
    }

    const Buses &TransportCatalogue::GetAllRoutes() const {
      return buses_dict_;
    }

    const Stops &TransportCatalogue::GetAllStops() const {
      return stops_dict_;
    }

    const PassingBuses &TransportCatalogue::GetAllPassingBuses() const {
      return stop_passing_buses_;
    }

    const DistancesBetweenStops &TransportCatalogue::GetDistanceBetweenStops() const {
      return distance_between_stops_;
    }

    namespace detail
      {


        double ComputeFactGeoLength(const Stop* const prev_stop,
                                    const Stop* const next_stop,
                                    const DistancesBetweenStops &distance_between_stops) {
          double fact_distanse = 0.0;
          const auto dist_it = distance_between_stops.find({prev_stop, next_stop});
          if (dist_it == distance_between_stops.end()) {
            const auto dist_reverse_it_ = distance_between_stops.find({next_stop, prev_stop});
            if (dist_reverse_it_ != distance_between_stops.end()) {
              fact_distanse = dist_reverse_it_->second;
            } else {
              fact_distanse = -1.0;
            }
          } else {
            fact_distanse = dist_it->second;
          }
          return fact_distanse;
        }


      }
  }
