#include "map_renderer.h"
#include "svg.h"

#include <unordered_set>

namespace detail
  {
    inline const double EPSILON = 1e-6;
    bool IsZero(double value) {
      return std::abs(value) < EPSILON;
    }
  }

void SetTextSettings(svg::Text &text,
                     const std::string_view data,
                     const svg::Point &point,
                     const transcat::RenderSettings &render_settings) {
  using namespace std::literals;
  text.SetPosition(point);
  text.SetOffset({render_settings.bus_label_offset[0], render_settings.bus_label_offset[1]});
  text.SetFontSize(render_settings.bus_label_font_size);
  text.SetFontFamily("Verdana"s);
  text.SetFontWeight("bold"s);
  text.SetData(std::string(data));
}

void SetTextLayerSettings(svg::Text &text, const transcat::RenderSettings &render_settings) {
  text.SetFillColor(render_settings.underlayer_color);
  text.SetStrokeColor(render_settings.underlayer_color);
  text.SetStrokeWidth(render_settings.underlayer_width);
  text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
  text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void SetStopTextSettings(svg::Text &text,
                         const std::string_view data,
                         const svg::Point &point,
                         const transcat::RenderSettings &render_settings) {
  using namespace std::literals;
  text.SetPosition(point);
  text.SetOffset({render_settings.stop_label_offset[0], render_settings.stop_label_offset[1]});
  text.SetFontSize(render_settings.stop_label_font_size);
  text.SetFontFamily("Verdana"s);
  text.SetData(std::string(data));
}

void SetStopTextLayerSettings(svg::Text &text, const transcat::RenderSettings &render_settings) {
  text.SetFillColor(render_settings.underlayer_color);
  text.SetStrokeColor(render_settings.underlayer_color);
  text.SetStrokeWidth(render_settings.underlayer_width);
  text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
  text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

void SetPolylineSettings(svg::Polyline &polyline, const transcat::RenderSettings &render_settings) {
  using namespace std::literals;
  polyline.SetFillColor("none"s);
  polyline.SetStrokeWidth(render_settings.line_width);
  polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
  polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
}

transcat::SphereProjector CreateSphereProjector(const std::map<const std::string_view
                                                               , const transcat::Bus *
                                                               , std::less<>> &all_routes,
                                                const transcat::RenderSettings &render_settings) {
  std::unordered_set<geo::Coordinates, geo::CoordinatesHash> coords;
  for (const auto &[key, bus]: all_routes) {
    for (const auto &stop: bus->route.stops) {
      coords.insert(stop->coords);
    }
  }
  const transcat::SphereProjector
      sp(coords.begin(), coords.end(), render_settings.width, render_settings.height, render_settings.padding);
  return sp;
}

namespace transcat
  {
    void MapRenderer::DrawPolylines(svg::Document &doc, const SphereProjector &sp) const {
      const auto number_of_colors = render_settings_.color_palette.size();
      size_t current_color = 0;
      const bool empty_palette = render_settings_.color_palette.empty();
      for (const auto &[key, bus]: all_routes_) {
        svg::Polyline polyline;
        const auto route_size = bus->route.stops.size();
        const auto &route = bus->route.stops;
        if (route_size == 1) {
          polyline.AddPoint({render_settings_.padding, render_settings_.padding});

        } else if (route_size > 1) {
          for (size_t i = 0; i < route_size; ++i) {
            polyline.AddPoint(sp(route[i]->coords));
          }
          if (!bus->route.is_roundtrip) {
            for (size_t i = route_size - 1; i > 0; --i) {
              polyline.AddPoint(sp(route[i - 1]->coords));
            }
          }
        }
        if (!empty_palette) {
          polyline.SetStrokeColor(render_settings_.color_palette[current_color++]);
        }
        SetPolylineSettings(polyline, render_settings_);
        doc.Add(polyline);
        if (current_color > number_of_colors - 1) {
          current_color = 0;
        }
      }
    }
    void MapRenderer::DrawBusText(svg::Document &doc, const SphereProjector &sp) const {
      size_t current_color_for_text = 0;
      const auto number_of_colors = render_settings_.color_palette.size();
      const bool empty_palette = render_settings_.color_palette.empty();
      for (const auto &[key, bus]: all_routes_) {
        const auto route_size = bus->route.stops.size();
        const auto &route = bus->route;
        if (route_size == 0) {
          continue;
        }
        if (route.is_roundtrip) {
          const auto *const stop = route.stops[0];
          svg::Text text;
          svg::Text text_layer;
          SetTextSettings(text, key, sp(stop->coords), render_settings_);
          SetTextSettings(text_layer, key, sp(stop->coords), render_settings_);
          SetTextLayerSettings(text_layer, render_settings_);
          if (!empty_palette) {
            text.SetFillColor(render_settings_.color_palette[current_color_for_text++]);
          }
          if (current_color_for_text > number_of_colors - 1) {
            current_color_for_text = 0;
          }
          doc.Add(text_layer);
          doc.Add(text);
        } else {
          const auto *const first_last_stop = route.stops[0];
          const auto *const last_last_stop = route.stops[route_size - 1];
          svg::Text text;
          svg::Text text_layer;
          SetTextSettings(text, key, sp(first_last_stop->coords), render_settings_);
          SetTextSettings(text_layer, key, sp(first_last_stop->coords), render_settings_);
          SetTextLayerSettings(text_layer, render_settings_);
          if (!empty_palette) {
            text.SetFillColor(render_settings_.color_palette[current_color_for_text]);
          }
          doc.Add(text_layer);
          doc.Add(text);
          if (first_last_stop != last_last_stop) {
            svg::Text text_last_stop;
            svg::Text text_layer_last_stop;
            SetTextSettings(text_last_stop, key, sp(last_last_stop->coords), render_settings_);
            SetTextSettings(text_layer_last_stop, key, sp(last_last_stop->coords), render_settings_);
            SetTextLayerSettings(text_layer_last_stop, render_settings_);
            if (!empty_palette) {
              text_last_stop.SetFillColor(render_settings_.color_palette[current_color_for_text]);
            }
            doc.Add(text_layer_last_stop);
            doc.Add(text_last_stop);
          }
          ++current_color_for_text;
          if (current_color_for_text > number_of_colors - 1) {
            current_color_for_text = 0;
          }
        }
      }
    }
    void MapRenderer::DrawCircles(svg::Document &doc, const SphereProjector &sp) const {
      using namespace std::literals;

      for (const auto &[name, stop]: all_stops_) {
        const auto buses_it = all_passing_buses_.find(stop);
        if (buses_it == all_passing_buses_.cend()) {
          continue;
        }
        if (buses_it->second.empty()) {
          continue;
        }

        svg::Circle circle;
        circle.SetCenter(sp(stop->coords));
        circle.SetRadius(render_settings_.stop_radius);
        circle.SetFillColor("white"s);
        doc.Add(circle);
      }
    }
    void MapRenderer::DrawStopsText(svg::Document &doc, const SphereProjector &sp) const {
      using namespace std::literals;

      for (const auto &[name, stop]: all_stops_) {
        const auto buses_it = all_passing_buses_.find(stop);
        if (buses_it == all_passing_buses_.cend() || buses_it->second.empty()) {
          continue;
        }
        svg::Text text;
        svg::Text text_layer;
        SetStopTextSettings(text, name, sp(stop->coords), render_settings_);
        SetStopTextSettings(text_layer, name, sp(stop->coords), render_settings_);
        SetStopTextLayerSettings(text_layer, render_settings_);
        text.SetFillColor("black"s);
        doc.Add(text_layer);
        doc.Add(text);
      }
    }

    svg::Document MapRenderer::DrawRoutes() const {
      using namespace std::literals;

      svg::Document doc;
      const auto sp = CreateSphereProjector(all_routes_, render_settings_);
      DrawPolylines(doc, sp);
      DrawBusText(doc, sp);
      DrawCircles(doc, sp);
      DrawStopsText(doc, sp);

      return doc;
    }

  }
