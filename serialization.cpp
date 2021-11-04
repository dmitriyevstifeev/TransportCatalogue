#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <utility>

#include "serialization.h"
#include "transport_router.h"

#include <graph.pb.h>
#include <map_renderer.pb.h>
#include <svg.pb.h>
#include <transport_catalogue.pb.h>
#include <transport_router.pb.h>

transport_catalogue_serialize::StopsList SerializeStops(const transcat::TransportCatalogue &transport_catalogue,
                                                        std::unordered_map<const transcat::Stop *, int> &stop_id_list) {
  transport_catalogue_serialize::StopsList stops_list;
  int id = 0;
  for (const auto &stop: transport_catalogue.GetAllStops()) {
    transport_catalogue_serialize::Stop serialized_stop;
    serialized_stop.set_name(stop.second->name);
    serialized_stop.set_coordinates_lat(stop.second->coords.lat);
    serialized_stop.set_coordinates_lng(stop.second->coords.lng);
    auto *new_stop = stops_list.add_stops();
    *new_stop = std::move(serialized_stop);
    stop_id_list.insert({stop.second, id++});
  }
  return stops_list;
}

transport_catalogue_serialize::BusesList SerializeBuses(const transcat::TransportCatalogue &transport_catalogue,
                                                        const std::unordered_map<const transcat::Stop *
                                                                                 , int> &stop_id_list,
                                                        std::unordered_map<const std::string_view
                                                                           , int
                                                                           , BusHash
                                                                           , std::equal_to<>> &bus_id_list) {
  transport_catalogue_serialize::BusesList buses_list;
  int bus_id = 1;
  for (const auto &bus: transport_catalogue.GetAllRoutes()) {
    transport_catalogue_serialize::Bus serialized_bus;
    bus_id_list.insert({bus.second->name, bus_id++});
    serialized_bus.set_name(bus.second->name);
    serialized_bus.set_is_roundtrip(bus.second->route.is_roundtrip);
    for (const auto *const stop: bus.second->route.stops) {
      transport_catalogue_serialize::Stop serialized_stop;
      const auto &stop_it = stop_id_list.find(stop);
      serialized_bus.add_stops_id(stop_it->second);
    }
    auto *new_bus = buses_list.add_buses();
    *new_bus = std::move(serialized_bus);
  }
  return buses_list;
}

transport_catalogue_serialize::DistancesList SerializeDistanceBetweenStops(const transcat::TransportCatalogue &transport_catalogue,
                                                                           const std::unordered_map<const transcat::Stop *
                                                                                                    , int> &stop_id_list) {
  transport_catalogue_serialize::DistancesList distances_list;
  for (const auto &stops_pair: transport_catalogue.GetDistanceBetweenStops()) {
    transport_catalogue_serialize::DistanceBetweenStops serialized_distance_between_stops;
    const auto &stop_it_first = stop_id_list.find(stops_pair.first.first);
    serialized_distance_between_stops.set_first_stop_id(stop_it_first->second);
    const auto &stop_it_second = stop_id_list.find(stops_pair.first.second);
    serialized_distance_between_stops.set_second_stop_id(stop_it_second->second);
    serialized_distance_between_stops.set_distance(stops_pair.second);

    auto *new_distance = distances_list.add_distance();
    *new_distance = std::move(serialized_distance_between_stops);
  }
  return distances_list;
}

transport_catalogue_serialize::Color SerializeColor(const svg::Color &color) {
  transport_catalogue_serialize::Color serialized_color;
  if (std::holds_alternative<std::string>(color)) {
    serialized_color.set_string_value(std::get<std::string>(color));
  } else if (std::holds_alternative<svg::Rgb>(color)) {
    transport_catalogue_serialize::Rgb serialized_rgb;
    svg::Rgb rgb = std::get<svg::Rgb>(color);
    serialized_rgb.set_red(rgb.red);
    serialized_rgb.set_green(rgb.green);
    serialized_rgb.set_blue(rgb.blue);
    *serialized_color.mutable_rgb_value() = std::move(serialized_rgb);
  } else if (std::holds_alternative<svg::Rgba>(color)) {
    transport_catalogue_serialize::Rgba serialized_rgba;
    svg::Rgba rgba = std::get<svg::Rgba>(color);
    serialized_rgba.set_red(rgba.red);
    serialized_rgba.set_green(rgba.green);
    serialized_rgba.set_blue(rgba.blue);
    serialized_rgba.set_opacity(rgba.opacity);
    *serialized_color.mutable_rgba_value() = std::move(serialized_rgba);
  }
  return serialized_color;
}

transport_catalogue_serialize::Render_settings SerializeRenderSettings(const transcat::RenderSettings &render_settings) {
  transport_catalogue_serialize::Render_settings serialized_render_settings;
  serialized_render_settings.set_width(render_settings.width);
  serialized_render_settings.set_height(render_settings.height);
  serialized_render_settings.set_padding(render_settings.padding);
  serialized_render_settings.set_line_width(render_settings.line_width);
  serialized_render_settings.set_stop_radius(render_settings.stop_radius);
  serialized_render_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
  serialized_render_settings.add_bus_label_offset(render_settings.bus_label_offset[0]);
  serialized_render_settings.add_bus_label_offset(render_settings.bus_label_offset[1]);
  serialized_render_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
  serialized_render_settings.add_stop_label_offset(render_settings.stop_label_offset[0]);
  serialized_render_settings.add_stop_label_offset(render_settings.stop_label_offset[1]);
  *serialized_render_settings.mutable_underlayer_color() = SerializeColor(render_settings.underlayer_color);
  serialized_render_settings.set_underlayer_width(render_settings.underlayer_width);
  for (const auto &color: render_settings.color_palette) {
    auto *new_color = serialized_render_settings.add_color_palette();
    *new_color = SerializeColor(color);
  }

  return serialized_render_settings;
}

transport_catalogue_serialize::RoutingSettings SerializeRoutingSettings(const transcat::RoutingSettings &routing_settings) {
  transport_catalogue_serialize::RoutingSettings serialized_routing_settings;
  serialized_routing_settings.set_bus_wait_time(routing_settings.bus_wait_time_minutes);
  serialized_routing_settings.set_bus_velocity(routing_settings.bus_velocity_kilometres_per_hour);

  return serialized_routing_settings;
}

transport_catalogue_serialize::Edge SerializeEdge(const graph::Edge<Minutes> &edge,
                                                  const transcat::TransportCatalogue &transport_catalogue,
                                                  const std::unordered_map<const transcat::Stop *, int> &stop_id_list,
                                                  const std::unordered_map<const std::string_view
                                                                           , int
                                                                           , BusHash
                                                                           , std::equal_to<>> &bus_id_list) {
  transport_catalogue_serialize::Edge serialized_edge;
  serialized_edge.set_from(edge.from);
  serialized_edge.set_to(edge.to);
  serialized_edge.set_weight(edge.weight);
  if (!edge.bus_name.empty()) {
    const auto bus_it = bus_id_list.find(edge.bus_name);
    serialized_edge.set_bus_id(bus_it->second);
  } else {
    serialized_edge.set_bus_id(0);
  }
  const auto stop_it = stop_id_list.find(transport_catalogue.FindStop(edge.stop_name));
  serialized_edge.set_stop_id(stop_it->second);
  serialized_edge.set_span_count(edge.span_count);

  return serialized_edge;
}

transport_catalogue_serialize::Graph SerializeGraph(const transcat::TransportCatalogue &transport_catalogue,
                                                    const std::shared_ptr<transcat::TransportRouter>& transport_router,
                                                    const std::unordered_map<const transcat::Stop *, int> &stop_id_list,
                                                    const std::unordered_map<const std::string_view
                                                                             , int
                                                                             , BusHash
                                                                             , std::equal_to<>> &bus_id_list) {

  transport_catalogue_serialize::Graph serialized_graph;
  const auto &graph = transport_router->GetGraph();
  const size_t EdgeCount = graph.GetEdgeCount();
  for (size_t i = 0; i < EdgeCount; ++i) {
    auto *new_edge = serialized_graph.add_edges();
    *new_edge = SerializeEdge(graph.GetEdge(i), transport_catalogue, stop_id_list, bus_id_list);
  }
  const auto &incidence_lists = graph.GetIncidenceLists();
  for (const auto &list: incidence_lists) {
    auto *new_serialized_incidence_list = serialized_graph.add_incidence_lists();
    transport_catalogue_serialize::IncidenceList serialized_incidence_list;
    for (const auto &edge_id: list) {
      serialized_incidence_list.add_edge_id(edge_id);
    }
    *new_serialized_incidence_list = std::move(serialized_incidence_list);
  }
  return serialized_graph;
}

void SerializeRoutesInternalData(const std::shared_ptr<transcat::TransportRouter>& transport_router,
                                 transport_catalogue_serialize::TransportRouter &serialized_transport_router) {
  const auto router = transport_router->GetRouter();
  for (const auto &routes_internal_data: router->GetRoutesInternalData()) {
    transport_catalogue_serialize::RoutesInternalData serialized_routes_internal_data;
    for (const auto &optional_rid: routes_internal_data) {
      transport_catalogue_serialize::optionalRID serialized_optional_rid;
      if (optional_rid.has_value()) {
        transport_catalogue_serialize::RID serialized_rid;
        serialized_rid.set_weight(optional_rid.value().weight);
        if (optional_rid.value().prev_edge.has_value()) {
          serialized_rid.set_prev_edge_id(optional_rid.value().prev_edge.value());
        } else {
          serialized_rid.set_prev_edge_id(-1);
        }
        serialized_optional_rid.set_no_value(false);
        *serialized_optional_rid.mutable_rid_value() = std::move(serialized_rid);
      } else {
        serialized_optional_rid.set_no_value(true);
      }
      auto *new_optional_rid = serialized_routes_internal_data.add_optional_rid();
      *new_optional_rid = std::move(serialized_optional_rid);
    }
    auto *new_routes_internal_data = serialized_transport_router.add_routes_internal_data();
    *new_routes_internal_data = std::move(serialized_routes_internal_data);
  }
}

void SerializeReverseDataForStops(const std::unordered_map<const transcat::Stop *
                                                           , int> &stop_id_list,
                                  const std::shared_ptr<transcat::TransportRouter>& transport_router,
                                  const transcat::TransportCatalogue &transport_catalogue,
                                  transport_catalogue_serialize::TransportRouter &serialized_transport_router) {
  for (const auto &i: transport_router->GetReversedDataForGraph()) {
    transport_catalogue_serialize::ReverseDataForGraph serialized_reverse_data_for_graph;
    const auto *const stop = transport_catalogue.FindStop(i.first);
    const auto stop_id_it = stop_id_list.find(stop);
    const int stop_id = stop_id_it->second;
    serialized_reverse_data_for_graph.set_stop_id(stop_id);
    serialized_reverse_data_for_graph.set_reversed_stop_id(i.second);
    auto *new_reverse_data_for_graph = serialized_transport_router.add_reversed_data_for_graph();
    *new_reverse_data_for_graph = std::move(serialized_reverse_data_for_graph);
  }
}

transport_catalogue_serialize::TransportRouter SerializeTransportRouter(const transcat::TransportCatalogue &transport_catalogue,
                                                                        const std::shared_ptr<transcat::TransportRouter>& transport_router,
                                                                        const transcat::RoutingSettings &routing_settings,
                                                                        const std::unordered_map<const transcat::Stop *
                                                                                                , int> &stop_id_list,
                                                                        const std::unordered_map<const std::string_view
                                                                                                , int
                                                                                                , BusHash
                                                                                                , std::equal_to<>> &bus_id_list) {
  transport_router->Initialize(routing_settings);
  transport_catalogue_serialize::TransportRouter serialized_transport_router;
  SerializeReverseDataForStops(stop_id_list, transport_router, transport_catalogue, serialized_transport_router);
  *serialized_transport_router.mutable_graph() =
      SerializeGraph(transport_catalogue, transport_router, stop_id_list, bus_id_list);

  SerializeRoutesInternalData(transport_router, serialized_transport_router);
  return serialized_transport_router;
}

void SerializeTransportCatalogue(const std::string &file_name,
                                 const transcat::TransportCatalogue &transport_catalogue,
                                 const transcat::RenderSettings &render_settings,
                                 const transcat::RoutingSettings &routing_settings,
                                 const std::shared_ptr<transcat::TransportRouter>& transport_router) {
  if (file_name.empty()) {
    return;
  }
  std::ofstream out(file_name, std::ios::binary);
  std::unordered_map<const transcat::Stop *, int> stop_id_list;
  std::unordered_map<const std::string_view, int, BusHash, std::equal_to<>> bus_id_list;
  transport_catalogue_serialize::TransportCatalogue serialized_transport_catalogue;
  *serialized_transport_catalogue.mutable_stops_list() = SerializeStops(transport_catalogue, stop_id_list);
  *serialized_transport_catalogue.mutable_buses_list() = SerializeBuses(transport_catalogue, stop_id_list, bus_id_list);
  *serialized_transport_catalogue.mutable_distances_list() =
      SerializeDistanceBetweenStops(transport_catalogue, stop_id_list);
  *serialized_transport_catalogue.mutable_render_settings() = SerializeRenderSettings(render_settings);
  *serialized_transport_catalogue.mutable_routing_settings() = SerializeRoutingSettings(routing_settings);
  *serialized_transport_catalogue.mutable_transport_router() =
      SerializeTransportRouter(transport_catalogue, transport_router, routing_settings, stop_id_list, bus_id_list);
  serialized_transport_catalogue.SerializeToOstream(&out);
}

svg::Color DeserializeColor(const transport_catalogue_serialize::Color &serialized_color) {
  svg::Color color;
  if (serialized_color.has_rgb_value()) {
    const auto &serialized_rgb = serialized_color.rgb_value();
    svg::Rgb rgb;
    rgb.red = serialized_rgb.red();
    rgb.green = serialized_rgb.green();
    rgb.blue = serialized_rgb.blue();
    color = rgb;
  } else if (serialized_color.has_rgba_value()) {
    const auto &serialized_rgba = serialized_color.rgba_value();
    svg::Rgba rgba;
    rgba.red = serialized_rgba.red();
    rgba.green = serialized_rgba.green();
    rgba.blue = serialized_rgba.blue();
    rgba.opacity = serialized_rgba.opacity();
    color = rgba;
  } else {
    color = serialized_color.string_value();
  }
  return color;
}

transcat::RenderSettings DeserializeRenderSettings(const transport_catalogue_serialize::Render_settings &serialized_render_settings) {
  transcat::RenderSettings render_settings;
  render_settings.width = serialized_render_settings.width();
  render_settings.height = serialized_render_settings.height();
  render_settings.padding = serialized_render_settings.padding();
  render_settings.line_width = serialized_render_settings.line_width();
  render_settings.stop_radius = serialized_render_settings.stop_radius();
  render_settings.bus_label_font_size = serialized_render_settings.bus_label_font_size();
  render_settings.bus_label_offset[0] = serialized_render_settings.bus_label_offset(0);
  render_settings.bus_label_offset[1] = serialized_render_settings.bus_label_offset(1);
  render_settings.stop_label_font_size = serialized_render_settings.stop_label_font_size();
  render_settings.stop_label_offset[0] = serialized_render_settings.stop_label_offset(0);
  render_settings.stop_label_offset[1] = serialized_render_settings.stop_label_offset(1);
  render_settings.underlayer_color = DeserializeColor(serialized_render_settings.underlayer_color());
  render_settings.underlayer_width = serialized_render_settings.underlayer_width();
  const int size = serialized_render_settings.color_palette_size();
  for (int i = 0; i < size; ++i) {
    render_settings.color_palette.push_back(DeserializeColor(serialized_render_settings.color_palette(i)));
  }
  return render_settings;
}

transcat::RoutingSettings DeserializeRoutingSettings(const transport_catalogue_serialize::RoutingSettings &serialized_routing_settings) {
  transcat::RoutingSettings routing_settings;
  routing_settings.bus_wait_time_minutes = serialized_routing_settings.bus_wait_time();
  routing_settings.bus_velocity_kilometres_per_hour = serialized_routing_settings.bus_velocity();
  return routing_settings;
}

graph::Edge<Minutes> DeserializeEdge(const transcat::TransportCatalogue &transport_catalogue,
                                     const transport_catalogue_serialize::Edge &serialized_edge,
                                     const transport_catalogue_serialize::StopsList &stops_list,
                                     const transport_catalogue_serialize::BusesList &buses_list) {
  graph::Edge<Minutes> edge;
  edge.from = serialized_edge.from();
  edge.to = serialized_edge.to();
  edge.weight = serialized_edge.weight();
  if (serialized_edge.bus_id() != 0) {
    const auto &bus_name = buses_list.buses(serialized_edge.bus_id() - 1);
    const auto *bus = transport_catalogue.FindBus(bus_name.name());
    if (bus != nullptr) {
      edge.bus_name = bus->name;
    }
  }

  const auto &stop_name = stops_list.stops(serialized_edge.stop_id());
  const auto *stop = transport_catalogue.FindStop(stop_name.name());
  if (stop != nullptr) {
    edge.stop_name = stop->name;
  }
  edge.span_count = serialized_edge.span_count();
  return edge;
}

void DeserializeGraph(const transcat::TransportCatalogue &transport_catalogue,
                      graph::DirectedWeightedGraph<Minutes> &graph,
                      const transport_catalogue_serialize::Graph &serialized_graph,
                      const transport_catalogue_serialize::StopsList &stops_list,
                      const transport_catalogue_serialize::BusesList &buses_list
) {

  std::vector<graph::Edge<Minutes>> edges_list;
  const int size = serialized_graph.edges_size();
  for (int i = 0; i < size; ++i) {
    graph::Edge<Minutes> edge = DeserializeEdge(transport_catalogue, serialized_graph.edges(i), stops_list, buses_list);
    edges_list.push_back(edge);
  }
  const int incidence_list_size = serialized_graph.incidence_lists_size();
  std::vector<std::vector<graph::EdgeId>> incidence_lists;
  for (int j = 0; j < incidence_list_size; ++j) {
    std::vector<graph::EdgeId> list;
    const auto &serialized_list = serialized_graph.incidence_lists(j);
    const int serialized_list_size = serialized_list.edge_id_size();
    for (int k = 0; k < serialized_list_size; ++k) {
      list.push_back(serialized_list.edge_id(k));
    }
    incidence_lists.push_back(std::move(list));
  }
  graph.SetEdges(edges_list);
  graph.SetIncidenceLists(incidence_lists);
}

std::vector<std::vector<std::optional
                            <graph::RouteInternalData<Minutes>>>>
DeserializeRouteInternalData(const transport_catalogue_serialize::TransportRouter &serialised_transport_router) {
  std::vector<std::vector<std::optional<graph::RouteInternalData<Minutes>>>> routes_internal_data;
  const transport_catalogue_serialize::optionalRID empty_rid;
  const int size = serialised_transport_router.routes_internal_data_size();
  for (int i = 0; i < size; ++i) {
    std::vector<std::optional<graph::RouteInternalData<Minutes>>> optional_rid;
    const auto &serialized_optional_rid = serialised_transport_router.routes_internal_data(i);
    const int size_optional_rid = serialized_optional_rid.optional_rid_size();
    for (int j = 0; j < size_optional_rid; ++j) {
      std::optional<graph::RouteInternalData<Minutes>> opt_rid;
      if (serialized_optional_rid.optional_rid(j).no_value()) {
        opt_rid = std::nullopt;
        optional_rid.push_back(opt_rid);
        continue;
      }
      const auto &serialized_opt_rid = serialized_optional_rid.optional_rid(j);
      graph::RouteInternalData<Minutes> rid;
      const auto &serialized_rid = serialized_opt_rid.rid_value();
      rid.weight = serialized_rid.weight();
      if (serialized_rid.prev_edge_id() != -1) {
        rid.prev_edge = serialized_rid.prev_edge_id();
      } else {
        rid.prev_edge = std::nullopt;
      }
      opt_rid = rid;
      optional_rid.push_back(opt_rid);
    }

    routes_internal_data.push_back(optional_rid);
  }
  return routes_internal_data;
}

std::unordered_map<std::string_view
                   , size_t> DeserializeReversedDataForGraph(const transport_catalogue_serialize::TransportRouter &serialised_transport_router,
                                                             const transport_catalogue_serialize::StopsList &stops_list,
                                                             const transcat::TransportCatalogue &transport_catalogue) {
  std::unordered_map<std::string_view, size_t> reversed_data_for_graph;
  const int size = serialised_transport_router.reversed_data_for_graph_size();
  for (int i = 0; i < size; ++i) {
    const auto &serialized_reversed_data = serialised_transport_router.reversed_data_for_graph(i);
    const auto &serialized_stop = stops_list.stops(serialized_reversed_data.stop_id());
    const auto *const stop = transport_catalogue.FindStop(serialized_stop.name());
    reversed_data_for_graph.insert({stop->name, serialized_reversed_data.reversed_stop_id()});
  }

  return reversed_data_for_graph;
}

void DeserializeTransportRouter(const transport_catalogue_serialize::TransportRouter &serialised_transport_router,
                                const transcat::TransportCatalogue &transport_catalogue,
                                const std::shared_ptr<transcat::TransportRouter>& transport_router,
                                const transport_catalogue_serialize::StopsList &stops_list,
                                const transport_catalogue_serialize::BusesList &buses_list
) {
  DeserializeGraph(transport_catalogue,
                   transport_router->GetGraph(),
                   serialised_transport_router.graph(),
                   stops_list,
                   buses_list);
  transport_router->SetReverseDataForGraph(DeserializeReversedDataForGraph(serialised_transport_router,
                                                                           stops_list,
                                                                           transport_catalogue));
  const auto router =
      graph::Router<Minutes>(transport_router->GetGraph(), DeserializeRouteInternalData(serialised_transport_router));
  transport_router->SetRouter(router);
}

void DeserializeTransportCatalogue(const std::string &file_name,
                                   std::vector<InfoQuery> &queries_to_add,
                                   transcat::RenderSettings &render_settings,
                                   transcat::RoutingSettings &routing_settings,
                                   transcat::queries::QueryManager *queryManager,
                                   const transcat::TransportCatalogue &tc
) {
  if (file_name.empty()) {
    return;
  }
  std::ifstream file(file_name, std::ios::binary);
  transport_catalogue_serialize::TransportCatalogue transport_catalogue;
  if (file) {
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    if (transport_catalogue.ParseFromIstream(&buffer)) {
      const auto &stops_list = transport_catalogue.stops_list();
      const int stop_size = stops_list.stops_size();
      std::unordered_map<std::string_view, size_t> reverse_data_for_graph;
      for (int i = 0; i < stop_size; ++i) {
        const transport_catalogue_serialize::Stop &stop = stops_list.stops(i);
        StopQuery stop_query;
        stop_query.name = stop.name();
        stop_query.coordinates = {stop.coordinates_lat(), stop.coordinates_lng()};
        reverse_data_for_graph.insert({stop_query.name, i});
        queries_to_add.push_back(std::move(stop_query));
      }
      const auto &distances_list = transport_catalogue.distances_list();
      const int distances_size = distances_list.distance_size();
      for (int i = 0; i < distances_size; ++i) {
        const transport_catalogue_serialize::DistanceBetweenStops &distance = distances_list.distance(i);
        RoadDistanceQuery road_distance_query;
        road_distance_query.name = stops_list.stops(distance.first_stop_id()).name();
        const auto &second_stop = stops_list.stops(distance.second_stop_id()).name();
        road_distance_query.road_distances.push_back({second_stop, distance.distance()});
        queries_to_add.push_back(std::move(road_distance_query));
      }
      const auto &buses_list = transport_catalogue.buses_list();
      const int buses_size = buses_list.buses_size();
      for (int i = 0; i < buses_size; ++i) {
        const transport_catalogue_serialize::Bus &bus = buses_list.buses(i);
        BusQuery bus_query;
        bus_query.name = bus.name();
        bus_query.is_roundtrip = bus.is_roundtrip();
        const int stops_size = bus.stops_id_size();
        for (int j = 0; j < stops_size; ++j) {
          bus_query.route.push_back(stops_list.stops(bus.stops_id(j)).name());
        }
        queries_to_add.push_back(std::move(bus_query));
      }

      render_settings = DeserializeRenderSettings(transport_catalogue.render_settings());
      routing_settings = DeserializeRoutingSettings(transport_catalogue.routing_settings());

      queryManager->AddQueriesToTC();
      transcat::TransportRouter transport_router(tc);
      const auto tr = std::make_shared<transcat::TransportRouter>(transport_router);
      tr->SetRoutingSettings(routing_settings);
      queryManager->SetTransportRouter(tr);

      DeserializeTransportRouter(transport_catalogue.transport_router(),
                                 tc,
                                 queryManager->GetTranstoptRouter(),
                                 stops_list,
                                 buses_list);

    }
  }

}