#pragma once

#include "domain.h"

#include <deque>
#include <string_view>

namespace transcat
  {
    class TransportCatalogue {
     public:
      explicit TransportCatalogue() = default;

      void AddPassingBus(const Bus *const bus);
      const Stop *AddNewStop(const Stop &stop);
      Bus *AddNewBus(const Bus &bus);
      std::pair<DistancesBetweenStops::iterator , bool> InsertStopsDistance(const Stop *stop_from, const Stop *stop_to, int dist);

      RouteInfo ComputeRouteInfo(const std::string_view &bus_name) const;
      BusesInfo ComputeBusInfo(const std::string_view &stop_name) const;
      const Stop *FindStop(const std::string_view &stop_name) const;
      const Bus *FindBus(const std::string_view &bus_name) const;
      Bus *FindCreateBus(const std::string_view &bus_name);
      const Buses &GetAllRoutes() const;
      const Stops &GetAllStops() const;
      const PassingBuses &GetAllPassingBuses() const;
      const DistancesBetweenStops &GetDistanceBetweenStops() const;
     private:
      std::deque<Stop> stops_list_;
      std::deque<Bus> buses_list_;
      Stops stops_dict_;
      Buses buses_dict_;
      PassingBuses stop_passing_buses_;
      DistancesBetweenStops distance_between_stops_;

    };

    namespace detail
      {
        double ComputeFactGeoLength(const Stop* const prev_stop,
                                    const Stop* const next_stop,
                                    const DistancesBetweenStops &distance_between_stops);
       }
  }