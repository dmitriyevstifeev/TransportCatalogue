#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
  Rgb() = default;
  Rgb(uint8_t red, uint8_t green, uint8_t blue)
      : red(red)
      , green(green)
      , blue(blue) {
  }

  uint8_t red = 0;
  uint8_t green = 0;
  uint8_t blue = 0;
};

struct Rgba
    : public Rgb {
 public:
  Rgba() = default;
  Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
      : Rgb(red, green, blue)
      , opacity(opacity) {}


  double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"};

using namespace std::literals;

struct OstreamColorPrinter {
  std::ostream &out;
  void operator()(std::monostate) const {
    out << "none";
  }
  void operator()(const std::string& s) const {
    out << s;
  }
  void operator()(const Rgb &rgb) const {
    out << "rgb("sv << unsigned(rgb.red) << ","sv << unsigned(rgb.green) << ","sv << unsigned(rgb.blue) << ")"sv;
  }
  void operator()(const Rgba &rgba) const {
    out << "rgba("sv << unsigned(rgba.red) << ","sv << unsigned(rgba.green) << ","sv << unsigned(rgba.blue) << ","sv << rgba.opacity << ")"sv;
  }
};

enum class StrokeLineCap {
  BUTT,
  ROUND,
  SQUARE,
};

enum class StrokeLineJoin {
  ARCS,
  BEVEL,
  MITER,
  MITER_CLIP,
  ROUND,
};

std::ostream &operator<<(std::ostream &out, StrokeLineCap stroke_line_cap);
std::ostream &operator<<(std::ostream &out, StrokeLineJoin stroke_line_join);

struct Point {
  Point() = default;
  Point(double x, double y)
      : x(x)
      , y(y) {
  }
  double x = 0;
  double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
  RenderContext(std::ostream &out)
      : out(out) {
  }

  RenderContext(std::ostream &out, int indent_step, int indent = 0)
      : out(out)
      , indent_step(indent_step)
      , indent(indent) {
  }

  RenderContext Indented() const {
    return {out, indent_step, indent + indent_step};
  }

  void RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
      out.put(' ');
    }
  }

  std::ostream &out;
  int indent_step = 0;
  int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
 public:
  void Render(const RenderContext &context) const;
  virtual ~Object() = default;

 private:
  virtual void RenderObject(const RenderContext &context) const = 0;
};

template<typename Owner>
class PathProps {
 public:
  Owner &SetFillColor(Color color) {
    fill_color_ = std::move(color);
    return AsOwner();
  }
  Owner &SetStrokeColor(Color color) {
    stroke_color_ = std::move(color);
    return AsOwner();
  }
  Owner &SetStrokeWidth(double width) {
    stroke_width_ = width;
    return AsOwner();
  }
  Owner &SetStrokeLineCap(StrokeLineCap stroke_line_cap) {
    stroke_line_cap_ = stroke_line_cap;
    return AsOwner();
  }
  Owner &SetStrokeLineJoin(StrokeLineJoin stroke_line_join) {
    stroke_line_join_ = stroke_line_join;
    return AsOwner();
  }
 protected:
  ~PathProps() = default;
  void RenderAttrs(std::ostream &out) const {
    using namespace std::literals;
    if (fill_color_.has_value()) {
      out << " fill=\""sv;
      std::visit(OstreamColorPrinter{out}, *fill_color_);
      out << "\""sv;
    }
    if (stroke_color_.has_value()) {
      out << " stroke=\""sv;
      std::visit(OstreamColorPrinter{out}, *stroke_color_);
      out << "\""sv;
    }
    if (stroke_width_.has_value()) {
      out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
    }
    if (stroke_line_cap_.has_value()) {
      out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
    }
    if (stroke_line_join_.has_value()) {
      out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
    }
  }
 private:
  Owner &AsOwner() {
    // static_cast безопасно преобразует *this к Owner&,
    // если класс Owner — наследник PathProps
    return static_cast<Owner &>(*this);
  }
  std::optional<Color> fill_color_;
  std::optional<Color> stroke_color_;
  std::optional<double> stroke_width_;
  std::optional<StrokeLineCap> stroke_line_cap_;
  std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final
    : public Object, public PathProps<Circle> {
 public:
  Circle &SetCenter(Point center);
  Circle &SetRadius(double radius);

 private:
  void RenderObject(const RenderContext &context) const override;

  Point center_;
  double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final
    : public Object, public PathProps<Polyline> {
 public:
  // Добавляет очередную вершину к ломаной линии
  Polyline &AddPoint(Point point);

 private:
  void RenderObject(const RenderContext &context) const override;
  std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final
    : public Object, public PathProps<Text> {
 public:
  // Задаёт координаты опорной точки (атрибуты x и y)
  Text &SetPosition(Point pos);

  // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
  Text &SetOffset(Point offset);

  // Задаёт размеры шрифта (атрибут font-size)
  Text &SetFontSize(uint32_t size);

  // Задаёт название шрифта (атрибут font-family)
  Text &SetFontFamily(std::string font_family);

  // Задаёт толщину шрифта (атрибут font-weight)
  Text &SetFontWeight(std::string font_weight);

  // Задаёт текстовое содержимое объекта (отображается внутри тега text)
  Text &SetData(std::string data);

 private:
  void EscapeTextData(const RenderContext &context, const std::string &data) const;
  void RenderObject(const RenderContext &context) const override;

  Point position_ = {0.0, 0.0};
  Point offset_ = {0.0, 0.0};
  uint32_t font_size_ = 1;
  std::string font_weight_ = "";
  std::string font_family_ = "";
  std::string data_ = "";
};

class ObjectContainer {
 public:
  template<typename Obj>
  void Add(Obj obj) {
    AddPtr(std::make_unique<Obj>(std::move(obj)));
  }
  virtual void AddPtr(std::unique_ptr<Object> &&ptr) = 0;

 protected:
  virtual ~ObjectContainer() = default;
};

class Document
    : public ObjectContainer {
 public:
  // Добавляет в svg-документ объект-наследник svg::Object
  void AddPtr(std::unique_ptr<Object> &&ptr) override;

  // Выводит в ostream svg-представление документа
  void Render(std::ostream &out) const;

  // Прочие методы и данные, необходимые для реализации класса Document
 private:
  std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg