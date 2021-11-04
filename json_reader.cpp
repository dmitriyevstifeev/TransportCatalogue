#include "json_builder.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_router.h"
#include "serialization.h"

#include <memory>
#include <sstream>
#include <utility>

namespace detail
  {
    double AboveZero(const double d) {
      if (d < 0.0) {
        return 0.0;
      } else {
        return d;
      }
    }

    int AboveZero(const int i) {
      if (i < 0) {
        return 0;
      } else {
        return i;
      }
    }
  }

namespace transcat
  {
    namespace queries
      {
        void AddRouteDistance(const JsonInfoQuery &json_info_query, std::vector<InfoQuery> &queries_to_add) {
          if (!json_info_query.road_distances.empty()) {
            RoadDistanceQuery road_distance_query;
            road_distance_query.name = json_info_query.name;
            road_distance_query.road_distances = json_info_query.road_distances;
            queries_to_add.push_back(std::move(road_distance_query));
          }
        }

        void AddStop(const JsonInfoQuery &json_info_query, std::vector<InfoQuery> &queries_to_add) {
          StopQuery stop_query;
          stop_query.name = json_info_query.name;
          stop_query.coordinates = json_info_query.coordinates;
          queries_to_add.push_back(std::move(stop_query));
        }

        void AddRoute(const JsonInfoQuery &json_info_query, std::vector<InfoQuery> &queries_to_add) {
          BusQuery bus_query;
          bus_query.name = json_info_query.name;
          bus_query.is_roundtrip = json_info_query.is_roundtrip;
          bus_query.route = json_info_query.route;
          queries_to_add.push_back(std::move(bus_query));
        }

        void AddQuery(const JsonInfoQuery &json_info_query, std::vector<InfoQuery> &queries_to_add) {
          AddRouteDistance(json_info_query, queries_to_add);
          if (json_info_query.type == QueryType::NewStop) {
            AddStop(json_info_query, queries_to_add);
          } else {
            AddRoute(json_info_query, queries_to_add);
          }
        }

        void ReadBaseRequests(json::Node &reqs, std::vector<InfoQuery> &queries_to_add) {
          for (auto &req: reqs.AsArray()) {
            JsonInfoQuery json_info_query;
            for (auto&[name, value]: req.AsDict()) {
              if (name == "type") {
                const auto &query_type = value.AsString();
                if (query_type == "Bus") {
                  json_info_query.type = QueryType::NewRoute;
                } else {
                  json_info_query.type = QueryType::NewStop;
                }
              } else if (name == "name") {
                json_info_query.name = value.AsString();
              } else if (name == "stops") {
                for (auto &stop: value.AsArray()) {
                  json_info_query.route.push_back(stop.AsString());
                }
              } else if (name == "is_roundtrip") {
                json_info_query.is_roundtrip = value.AsBool();
              } else if (name == "latitude") {
                if (value.IsPureDouble()) {
                  json_info_query.coordinates.lat = value.AsDouble();
                } else {
                  json_info_query.coordinates.lat = value.AsInt();
                }
              } else if (name == "longitude") {
                if (value.IsPureDouble()) {
                  json_info_query.coordinates.lng = value.AsDouble();
                } else {
                  json_info_query.coordinates.lng = value.AsInt();
                }
              } else if (name == "road_distances") {
                for (const auto&[stop, distance]: value.AsDict()) {
                  json_info_query.road_distances.push_back({stop, distance.AsInt()});
                }
              }

            }
            AddQuery(json_info_query, queries_to_add);
          }
        }
        void ReadStatRequests(json::Node &reqs, std::vector<Request> &requests) {
          for (auto &req: reqs.AsArray()) {
            Request request;
            for (const auto&[name, value]: req.AsDict()) {
              if (name == "type") {
                const auto &query_type = value.AsString();
                if (query_type == "Bus") {
                  request.type = RequestType::Bus;
                } else if (query_type == "Stop") {
                  request.type = RequestType::Stop;
                } else if (query_type == "Map") {
                  request.type = RequestType::Map;
                } else if (query_type == "Route") {
                  request.type = RequestType::Route;
                }
              } else if (name == "name") {
                request.name = value.AsString();
              } else if (name == "id") {
                request.id = value.AsInt();
              } else if (name == "from") {
                request.from = value.AsString();
              } else if (name == "to") {
                request.to = value.AsString();
              }
            }
            requests.push_back(std::move(request));
          }
        }
        void ReadRenderSettings(json::Node &reqs, transcat::RenderSettings &render_settings) {
          for (auto&[name, value]: reqs.AsDict()) {
            if (name == "width") {
              render_settings.width = ::detail::AboveZero(value.AsDouble());
            } else if (name == "height") {
              render_settings.height = ::detail::AboveZero(value.AsDouble());
            } else if (name == "padding") {
              render_settings.padding = ::detail::AboveZero(value.AsDouble());
            } else if (name == "line_width") {
              render_settings.line_width = ::detail::AboveZero(value.AsDouble());
            } else if (name == "stop_radius") {
              render_settings.stop_radius = ::detail::AboveZero(value.AsDouble());
            } else if (name == "bus_label_font_size") {
              render_settings.bus_label_font_size = ::detail::AboveZero(value.AsInt());
            } else if (name == "bus_label_offset") {
              int i = 0;
              for (const auto &offset: value.AsArray()) {
                render_settings.bus_label_offset[i++] = offset.AsDouble();
              }
            } else if (name == "stop_label_font_size") {
              render_settings.stop_label_font_size = ::detail::AboveZero(value.AsInt());
            } else if (name == "stop_label_offset") {
              int i = 0;
              for (const auto &offset: value.AsArray()) {
                render_settings.stop_label_offset[i++] = offset.AsDouble();
              }
            } else if (name == "underlayer_color") {
              if (value.IsString()) {
                render_settings.underlayer_color = value.AsString();
              } else if (value.IsArray() && value.AsArray().size() == 3) {
                auto &jColor = value.AsArray();
                svg::Rgb color(jColor[0].AsInt(), jColor[1].AsInt(), jColor[2].AsInt());
                render_settings.underlayer_color = std::move(color);
              } else if (value.IsArray() && value.AsArray().size() == 4) {
                const auto &jColor = value.AsArray();
                svg::Rgba color(jColor[0].AsInt(), jColor[1].AsInt(), jColor[2].AsInt(), jColor[3].AsDouble());
                render_settings.underlayer_color = std::move(color);
              }
            } else if (name == "underlayer_width") {
              render_settings.underlayer_width = ::detail::AboveZero(value.AsDouble());
            } else if (name == "color_palette") {
              for (auto &color: value.AsArray()) {
                if (color.IsString()) {
                  render_settings.color_palette.push_back(color.AsString());
                } else if (color.IsArray() && color.AsArray().size() == 3) {
                  const auto &jColor = color.AsArray();
                  svg::Rgb color_to_add(jColor[0].AsInt(), jColor[1].AsInt(), jColor[2].AsInt());
                  render_settings.color_palette.push_back(std::move(color_to_add));
                } else if (color.IsArray() && color.AsArray().size() == 4) {
                  const auto &jColor = color.AsArray();
                  svg::Rgba
                      color_to_add(jColor[0].AsInt(), jColor[1].AsInt(), jColor[2].AsInt(), jColor[3].AsDouble());
                  render_settings.color_palette.push_back(std::move(color_to_add));
                }
              }
            }

          }
        }
        void ReadRoutingSettings(json::Node &reqs, transcat::RoutingSettings &routing_settings) {
          for (auto&[name, value]: reqs.AsDict()) {
            if (name == "bus_wait_time") {
              routing_settings.bus_wait_time_minutes = value.AsInt();
            } else if (name == "bus_velocity") {
              routing_settings.bus_velocity_kilometres_per_hour = value.AsDouble();
            }
          }
        }
        void ReadSerializationSettings(json::Node &reqs, transcat::SerializationSettings &serialization_settings) {
          for (auto&[name, value]: reqs.AsDict()) {
            if (name == "file") {
              serialization_settings.file = value.AsString();
            }
          }
        }

        void QueryManager::ReadJsonRequests(std::istream &input) {
          const auto Doc = json::Load(input);
          auto root = Doc.GetRoot();
          for (auto &[req_type, reqs]: root.AsDict()) {
            if (req_type == "base_requests") {
              ReadBaseRequests(reqs, queries_to_add_);
            } else if (req_type == "stat_requests") {
              ReadStatRequests(reqs, requests_);
            } else if (req_type == "render_settings") {
              ReadRenderSettings(reqs, render_settings_);
            } else if (req_type == "routing_settings") {
              ReadRoutingSettings(reqs, routing_settings_);
            } else if (req_type == "serialization_settings") {
              ReadSerializationSettings(reqs, serialization_settings_);
            }
            AddQueriesToTC();
          }
        }

        void QueryManager::AddQueriesToTC() {
          for (const auto &query: queries_to_add_) {
            if (std::holds_alternative<StopQuery>(query)) {
              StopQuery stop_query = std::get<StopQuery>(query);
              Stop new_stop;
              new_stop.name = stop_query.name;
              new_stop.coords = stop_query.coordinates;
              tc_.AddNewStop(new_stop);
            }
          }
          for (const auto &query: queries_to_add_) {
            if (std::holds_alternative<RoadDistanceQuery>(query)) {
              RoadDistanceQuery road_distance_query = std::get<RoadDistanceQuery>(query);
              for (const auto &[to_stop, dist]: road_distance_query.road_distances) {
                const auto *const first_stop = tc_.FindStop(road_distance_query.name);
                const auto *const second_stop = tc_.FindStop(to_stop);
                tc_.InsertStopsDistance(first_stop, second_stop, dist);
              }
            }
          }
          for (const auto &query: queries_to_add_) {
            if (std::holds_alternative<BusQuery>(query)) {
              BusQuery bus_query = std::get<BusQuery>(query);
              auto *const bus = tc_.FindCreateBus(bus_query.name);
              bus->route.is_roundtrip = bus_query.is_roundtrip;
              for (const auto &stop: bus_query.route) {
                const auto *const found_stop = tc_.FindStop(stop);
                bus->route.stops.push_back(found_stop);
              }
              tc_.AddPassingBus(bus);
            }
          }
          queries_to_add_.clear();
        }

        json::Document QueryManager::GetJSONAnswers() {
          using namespace std::literals;
          auto jBuilder = json::Builder{};
          jBuilder.StartArray();
          for (const auto &request: requests_) {
            jBuilder.StartDict();
            jBuilder.Key("request_id"s).Value(static_cast<int>(request.id));
            switch (request.type) {
              case RequestType::Bus: {
                const auto &bus_name = request.name;
                const auto route_info = tc_.ComputeRouteInfo(bus_name);
                if (route_info.not_found) {
                  jBuilder.Key("error_message"s).Value("not found"s);
                } else {
                  jBuilder.Key("curvature"s).Value(route_info.curvature);
                  jBuilder.Key("route_length"s).Value(route_info.route_length);
                  jBuilder.Key("unique_stop_count"s).Value(static_cast<int>(route_info.unique_stops));
                  jBuilder.Key("stop_count"s).Value(static_cast<int>(route_info.stops_on_route));
                }
                break;
              }
              case RequestType::Stop: {
                const auto &stop_name = request.name;
                const auto buses_info = tc_.ComputeBusInfo(stop_name);
                if (buses_info.not_found) {
                  jBuilder.Key("error_message"s).Value("not found"s);
                } else {
                  if (buses_info.buses.empty()) {
                    jBuilder.Key("buses"s).StartArray().EndArray();
                  } else {
                    jBuilder.Key("buses"s).StartArray();
                    for (const auto &bus: buses_info.buses) {
                      jBuilder.Value(std::string(bus));
                    }
                    jBuilder.EndArray();
                  }
                }
                break;
              }
              case RequestType::Map: {
                transcat::MapRenderer map_renderer
                    (GetAllOrderedRoutes(tc_), GetAllOrderedStops(tc_), GetAllPassingBuses(tc_), render_settings_);
                const auto svg_doc = map_renderer.DrawRoutes();
                std::stringstream stringstream;
                svg_doc.Render(stringstream);
                jBuilder.Key("map"s).Value(stringstream.str());
                break;
              }
              case RequestType::Route: {
              if (!tr_->IsInitialized()) {
                  tr_->Initialize(routing_settings_);
                }
                const auto route_info = tr_->BuildRoute(request.from, request.to);
                if (route_info.not_found) {
                  jBuilder.Key("error_message"s).Value("not found"s);
                } else {
                  jBuilder.Key("total_time"s).Value(route_info.total_time);
                  jBuilder.Key("items"s).StartArray();
                  for (const auto &item: route_info.items) {
                    jBuilder.StartDict();
                    jBuilder.Key("time"s).Value(static_cast<double>(item.point_time));
                    if (item.item_type == transcat::ItemType::WAIT) {
                      jBuilder.Key("type"s).Value("Wait"s);
                      jBuilder.Key("stop_name"s).Value(std::string(item.name));
                    } else {
                      jBuilder.Key("type"s).Value("Bus"s);
                      jBuilder.Key("bus"s).Value(std::string(item.name));
                      jBuilder.Key("span_count"s).Value(static_cast<int>(item.span_count));
                    }
                    jBuilder.EndDict();
                  }
                  jBuilder.EndArray();
                }
                break;
              }
            }
            jBuilder.EndDict();

          }
          jBuilder.EndArray();
          requests_.clear();
          json::Document doc{jBuilder.Build()};
          return doc;
        }

        void QueryManager::SetTransportRouter(std::shared_ptr<transcat::TransportRouter> transport_router) {
          tr_ = std::move(transport_router);
        }

        const std::shared_ptr<transcat::TransportRouter>& QueryManager::GetTranstoptRouter() const {
          return tr_;
        }

        void QueryManager::Serialize() {
          transcat::TransportRouter transport_router(tc_);
          tr_ = std::make_shared<transcat::TransportRouter>(transport_router);
          SerializeTransportCatalogue(serialization_settings_.file, tc_, render_settings_, routing_settings_, tr_);
        }

        void QueryManager::Deserialize() {
          DeserializeTransportCatalogue(serialization_settings_.file,
                                        queries_to_add_,
                                        render_settings_,
                                        routing_settings_,
                                        this,
                                        tc_);

        }
      }
  }



