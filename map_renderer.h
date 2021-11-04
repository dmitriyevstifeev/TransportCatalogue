#pragma once

#include <algorithm>
#include <cmath>
#include <map>

#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace detail
  {
    bool IsZero(double value);
  }

namespace transcat {
    struct RenderSettings {
      double width   = 0.0;
      double height = 0.0;
      double padding = 0.0;
      double line_width = 0.0;
      double stop_radius = 0.0;
      size_t bus_label_font_size = 0;
      double bus_label_offset[2];
      size_t stop_label_font_size = 0;
      double stop_label_offset[2];
      svg::Color underlayer_color;
      double underlayer_width;
      std::vector<svg::Color> color_palette;
    };

    class SphereProjector {
     public:
      template<typename PointInputIt>
      SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                      double max_height, double padding)
          : padding_(padding) {
        if (points_begin == points_end) {
          return;
        }

        const auto[left_it, right_it]
        = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
          return lhs.lng < rhs.lng;
        });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto[bottom_it, top_it]
        = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
          return lhs.lat < rhs.lat;
        });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!detail::IsZero(max_lon - min_lon_)) {
          width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!detail::IsZero(max_lat_ - min_lat)) {
          height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
          zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
          zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
          zoom_coeff_ = *height_zoom;
        }
      }

      svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
      }

     private:
      double padding_;
      double min_lon_ = 0;
      double max_lat_ = 0;
      double zoom_coeff_ = 0;
    };

    class MapRenderer {
     public:
      explicit MapRenderer(std::map<const std::string_view, const transcat::Bus *, std::less<>> all_routes,
                           std::map<const std::string_view, const transcat::Stop *, std::less<>> all_stops,
                           const transcat::PassingBuses &all_passing_buses,
                           const RenderSettings &render_settings)
          : all_routes_(std::move(all_routes))
          , all_stops_(std::move(all_stops))
          , all_passing_buses_(all_passing_buses)
          , render_settings_(render_settings) {
      }

      svg::Document DrawRoutes() const;
     private:
      void DrawPolylines(svg::Document &doc, const SphereProjector &sp) const;
      void DrawBusText(svg::Document &doc, const SphereProjector &sp) const;
      void DrawCircles(svg::Document &doc, const SphereProjector &sp) const;
      void DrawStopsText(svg::Document &doc, const SphereProjector &sp) const;

      const std::map<const std::string_view, const transcat::Bus *, std::less<>> all_routes_;
      const std::map<const std::string_view, const transcat::Stop *, std::less<>> all_stops_;
      const transcat::PassingBuses &all_passing_buses_;
      const RenderSettings &render_settings_;
    };
}