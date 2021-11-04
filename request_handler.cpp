#include "request_handler.h"
namespace transcat
  {
    std::map<const std::string_view
             , const transcat::Bus *
             , std::less<>> GetAllOrderedRoutes(const transcat::TransportCatalogue &tc) {
      const auto &unordered_routes = tc.GetAllRoutes();
      return {unordered_routes.begin(), unordered_routes.end()};
    }

    std::map<const std::string_view
             , const transcat::Stop *
             , std::less<>> GetAllOrderedStops(const transcat::TransportCatalogue &tc) {
      const auto &unordered_stops = tc.GetAllStops();
      return {unordered_stops.begin(), unordered_stops.end()};
    }

    const transcat::PassingBuses &GetAllPassingBuses(const transcat::TransportCatalogue &tc) {
      return tc.GetAllPassingBuses();
    }
  }