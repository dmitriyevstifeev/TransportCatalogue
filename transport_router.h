#pragma once

#include "domain.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transcat
  {
    using Minutes = double;

    struct Item {
      Minutes point_time;
      ItemType item_type;
      std::string_view name;
      size_t span_count;
    };

    struct RoutingSettings {
      int bus_wait_time_minutes = 6;
      double bus_velocity_kilometres_per_hour = 40;
    };

    struct GrathRouteInfo {
      Minutes total_time;
      std::vector<Item> items;
      bool not_found = false;
    };

    class TransportRouter {
     public:
      explicit TransportRouter(const transcat::TransportCatalogue &tc);
      void Initialize(const RoutingSettings &routing_settings);
      bool IsInitialized() const;
      GrathRouteInfo BuildRoute(const std::string &from, const std::string &to) const;
      graph::DirectedWeightedGraph<Minutes> &GetGraph();
      std::shared_ptr<graph::Router<Minutes>> GetRouter() const;
      void SetRouter(const graph::Router<Minutes> &router) {
        router_ = std::make_shared<graph::Router<Minutes>>(router);
      }
      void SetRoutingSettings(const RoutingSettings &routing_settings) {
        routing_settings_ = routing_settings;
      }
      const std::unordered_map<std::string_view, size_t> &GetReversedDataForGraph() const {
        return reverse_data_for_graph_;
      }
      void SetReverseDataForGraph(const std::unordered_map<std::string_view, size_t> &reverse_data_for_graph) {
        for (const auto pair: reverse_data_for_graph) {
          reverse_data_for_graph_.insert({pair.first, pair.second});
        }
      }
     private:
      void CreateGraph();

      const transcat::TransportCatalogue &transport_catalogue_;
      RoutingSettings routing_settings_;
      std::unordered_map<std::string_view, size_t> reverse_data_for_graph_;
      graph::DirectedWeightedGraph<Minutes> graph_;
      std::shared_ptr<graph::Router<Minutes>> router_;
    };
  }