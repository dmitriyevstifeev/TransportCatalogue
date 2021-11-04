#pragma once

#include "transport_catalogue.h"
#include "domain.h"

#include <map>
#include <unordered_map>
#include <string_view>

namespace transcat
  {
    std::map<const std::string_view
             , const transcat::Bus *
             , std::less<>> GetAllOrderedRoutes(const transcat::TransportCatalogue &tc);
    std::map<const std::string_view
             , const transcat::Stop *
             , std::less<>> GetAllOrderedStops(const transcat::TransportCatalogue &tc);
    const transcat::PassingBuses &GetAllPassingBuses(const transcat::TransportCatalogue &tc);
  }