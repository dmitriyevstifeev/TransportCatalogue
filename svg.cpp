#include "svg.h"

namespace svg
  {
    using namespace std::literals;

    std::ostream &operator<<(std::ostream &out, StrokeLineCap stroke_line_cap) {
      switch (stroke_line_cap) {
        case StrokeLineCap::BUTT: {
          out << "butt";
          break;
        }
        case StrokeLineCap::ROUND: {
          out << "round";
          break;
        }
        case StrokeLineCap::SQUARE: {
          out << "square";
          break;
        }
      }
      return out;
    }
    std::ostream &operator<<(std::ostream &out, StrokeLineJoin stroke_line_join) {
      switch (stroke_line_join) {
        case StrokeLineJoin::ARCS: {
          out << "arcs";
          break;
        }
        case StrokeLineJoin::BEVEL: {
          out << "bevel";
          break;
        }
        case StrokeLineJoin::MITER: {
          out << "miter";
          break;
        }
        case StrokeLineJoin::MITER_CLIP: {
          out << "miter-clip";
          break;
        }
        case StrokeLineJoin::ROUND: {
          out << "round";
          break;
        }
      }
      return out;
    }

    void Object::Render(const RenderContext &context) const {
      context.RenderIndent();

      // Делегируем вывод тега своим подклассам
      RenderObject(context);

      context.out << std::endl;
    }
// ---------- Circle ------------------

    Circle &Circle::SetCenter(Point center) {
      center_ = center;
      return *this;
    }

    Circle &Circle::SetRadius(double radius) {
      radius_ = radius;
      return *this;
    }

    void Circle::RenderObject(const RenderContext &context) const {
      auto &out = context.out;
      out << " <circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
      out << "r=\""sv << radius_ << "\""sv;
      RenderAttrs(out);
      out << "/>"sv;
    }

// ---------- Polyline ------------------

    Polyline &Polyline::AddPoint(Point point) {
      points_.push_back(point);
      return *this;
    }

    void Polyline::RenderObject(const RenderContext &context) const {
      auto &out = context.out;
      out << " <polyline points=\""sv;
      bool first_point = true;
      for (const auto &p : points_) {
        if (first_point) {
          first_point = false;
        } else {
          out << " ";
        }
        out << p.x << "," << p.y;
      }
      out << "\""sv;
      RenderAttrs(out);
      out << "/>"sv;
    }

// ------------ Text --------------------

    Text &Text::SetPosition(Point pos) {
      position_ = pos;
      return *this;
    }

    Text &Text::SetOffset(Point offset) {
      offset_ = offset;
      return *this;
    }

    Text &Text::SetFontSize(uint32_t size) {
      font_size_ = size;
      return *this;
    }

    Text &Text::SetFontWeight(std::string font_weight) {
      font_weight_ = std::move(font_weight);
      return *this;
    }

    Text &Text::SetFontFamily(std::string font_family) {
      font_family_ = std::move(font_family);
      return *this;
    }

    Text &Text::SetData(std::string data) {
      data_ = std::move(data);
      return *this;
    }

    void Text::RenderObject(const RenderContext &context) const {
      auto &out = context.out;

      out << " <text";
      RenderAttrs(out);
      out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
      out << " dx=\""sv << offset_.x << "\" "sv << "dy=\""sv << offset_.y << "\""sv;
      out << " font-size=\""sv << font_size_ << "\""sv;
      if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
      }
      if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
      }
      out << ">"sv;
      EscapeTextData(context, data_);
      out << "</text>"sv;
    }
    void Text::EscapeTextData(const RenderContext &context, const std::string &data) const {
      auto &out = context.out;
      for (const auto &c : data) {
        switch (c) {
          case '\"':
            out << "&quot;";
            break;
          case '\'':
            out << "&apos;";
            break;
          case '<':
            out << "&lt;";
            break;
          case '>':
            out << "&gt;";
            break;
          case '&':
            out << "&amp;";
            break;
          default:
            out << c;
            break;
        }
      }
    }
// ------------ Document --------------------

    void Document::AddPtr(std::unique_ptr<Object> &&ptr) {
      objects_.push_back(std::move(ptr));
    }

    void Document::Render(std::ostream &out) const {
      out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
      out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
      for (const auto &obj : objects_) {
        obj->Render(out);
      }
      out << "</svg>"sv;
    }
  }  // namespace svg