#include "graph.h"
#include "transport_router.h"

#include <execution>
#include <memory>
#include <mutex>

namespace transcat
  {
    TransportRouter::TransportRouter(const transcat::TransportCatalogue &tc)
        : transport_catalogue_(tc)
        , graph_(tc.GetAllStops().size()) {

    }

    void TransportRouter::Initialize(const RoutingSettings &routing_settings) {
      routing_settings_ = routing_settings;
      CreateGraph();
    }

    bool TransportRouter::IsInitialized() const {
      return router_ != nullptr;
    }

    GrathRouteInfo TransportRouter::BuildRoute(const std::string &from, const std::string &to) const {
      GrathRouteInfo route_info{};

      const auto from_id = reverse_data_for_graph_.find(from);
      const auto to_id = reverse_data_for_graph_.find(to);
      if (from_id == reverse_data_for_graph_.end() || to_id == reverse_data_for_graph_.end()) {
        route_info.not_found = true;
        return route_info;
      }

      const auto router_result = router_->BuildRoute(from_id->second, to_id->second);
      if (router_result.has_value()) {
        route_info.total_time = router_result->weight;
        const auto &edges = router_result->edges;
        const auto size = edges.size();
        for (size_t i = 0; i < size; ++i) {
          const auto edge = graph_.GetEdge(edges[i]);
          route_info.items.push_back({
                                         static_cast<double>(routing_settings_.bus_wait_time_minutes), ItemType::WAIT
                                         , edge.stop_name, 0
                                     });
          route_info.items.push_back({
                                         edge.weight - routing_settings_.bus_wait_time_minutes, ItemType::BUS
                                         , edge.bus_name, edge.span_count
                                     });
        }
      } else {
        route_info.not_found = true;
      }
      return route_info;
    }

    graph::DirectedWeightedGraph<Minutes> &TransportRouter::GetGraph() {
      return graph_;
    }

    std::shared_ptr<graph::Router<Minutes>> TransportRouter::GetRouter() const {
      return router_;
    }

    void TransportRouter::CreateGraph() {
      constexpr double kMetreInMinuteCoefficient = 1000 * 1.0 / 60;

      {
        size_t id = 0;
        for (const auto &[name, stop]: transport_catalogue_.GetAllStops()) {
          reverse_data_for_graph_.insert({stop->name, id});
          std::string_view sv;
          graph_.AddEdge({id, id, 0.0, sv, name, 0});
          ++id;
        }
      }
      const auto &all_routes = transport_catalogue_.GetAllRoutes();
      const auto &distance_between_stops = transport_catalogue_.GetDistanceBetweenStops();
      const int wait_time = routing_settings_.bus_wait_time_minutes;
      const double speed = routing_settings_.bus_velocity_kilometres_per_hour * kMetreInMinuteCoefficient;
      std::mutex mx;
      std::for_each(std::execution::par, all_routes.begin(), all_routes.end(), [&](const auto &name_bus) {
        const auto *const bus = name_bus.second;
        const auto route_size = bus->route.stops.size();
        const auto &stops = bus->route.stops;
        for (size_t i = 0; i < route_size - 1; ++i) {
          size_t span_count = 0;
          for (size_t j = i + 1; j < route_size; ++j) {
            double distance = 0;
            for (size_t k = i + 1; k <= j; ++k) {
              distance += transcat::detail::ComputeFactGeoLength(stops[k - 1], stops[k], distance_between_stops);
            }
            const double time = distance / speed;
            const auto stop_it_from = reverse_data_for_graph_.find(stops[i]->name);
            const graph::VertexId from = stop_it_from->second;
            const auto stop_it_to = reverse_data_for_graph_.find(stops[j]->name);
            const graph::VertexId to = stop_it_to->second;
            std::lock_guard<std::mutex> guard(mx);
            graph_.AddEdge({from, to, wait_time + time, bus->name, stops[i]->name, ++span_count});
          }
        }
        if (!bus->route.is_roundtrip) {
          for (size_t i = route_size - 1; i > 0; --i) {
            const auto stop_it_from = reverse_data_for_graph_.find(stops[i]->name);
            const graph::VertexId from = stop_it_from->second;
            size_t span_count = 0;
            for (int j = i - 1; j >= 0; --j) {
              double distance = 0;
              for (int k = i; k > j; --k) {
                distance += transcat::detail::ComputeFactGeoLength(stops[k], stops[k - 1], distance_between_stops);
              }
              const double time = distance / speed;
              const auto stop_it_to = reverse_data_for_graph_.find(stops[j]->name);
              const graph::VertexId to = stop_it_to->second;
              std::lock_guard<std::mutex> guard(mx);
              graph_.AddEdge({from, to, wait_time + time, bus->name, stops[i]->name, ++span_count});
            }
          }
        }
      });
      graph::Router<Minutes> router(graph_);
      router_ = std::make_unique<graph::Router<Minutes>>(router);
    }

  }