#include "json_builder.h"

#include <memory>

namespace json
  {
    using namespace std::literals;

    BuildConstructor::BuildConstructor(Builder &builder)
        : builder_(builder) {}

    BuildContextFirst::BuildContextFirst(Builder &builder)
        : BuildConstructor(builder) {}

    DictContext &BuildContextFirst::StartDict() {
      return builder_.StartDict();
    }

    ArrayContext &BuildContextFirst::StartArray() {
      return builder_.StartArray();
    }

    BuildContextSecond::BuildContextSecond(Builder &builder)
        : BuildConstructor(builder) {}

    KeyContext &BuildContextSecond::Key(const std::string &key) {
      return builder_.Key(key);
    }

    Builder &BuildContextSecond::EndDict() {
      return builder_.EndDict();
    }

    KeyContext::KeyContext(Builder &builder)
        : BuildContextFirst(builder) {}

    ValueKeyContext &KeyContext::Value(const Node::Value &value) {
      return builder_.Value(value);
    }

    ValueKeyContext::ValueKeyContext(Builder &builder)
        : BuildContextSecond(builder) {}

    ValueArrayContext::ValueArrayContext(Builder &builder)
        : BuildContextFirst(builder) {}

    ValueArrayContext &ValueArrayContext::Value(const Node::Value &value) {
      return builder_.Value(value);
    }

    Builder &ValueArrayContext::EndArray() {
      return builder_.EndArray();
    }

    DictContext::DictContext(Builder &builder)
        : BuildContextSecond(builder) {}

    ArrayContext::ArrayContext(Builder &builder)
        : ValueArrayContext(builder) {}

    Builder::Builder()
        : KeyContext(*this)
        , ValueKeyContext(*this)
        , DictContext(*this)
        , ArrayContext(*this) {}

    KeyContext &Builder::Key(const std::string &key) {
      if (UnableUseKey()) {
        throw std::logic_error("Key error"s);
      } else {
        nodes_stack_.push_back(std::make_unique<Node>(key));
      }
      return *this;
    }

    Builder &Builder::Value(const Node::Value &value) {
      if (UnableUseValue()) {
        throw std::logic_error("Value error"s);
      } else {
        visit([this](auto &val) {
          nodes_stack_.push_back(std::make_unique<Node>(val));
        }, value);
        AddNode(std::move(*nodes_stack_.back()));
      }

      return *this;
    }

    DictContext &Builder::StartDict() {
      if (UnableUseStartDict()) {
        throw std::logic_error("Starting dict error"s);
      } else {
        nodes_stack_.push_back(std::make_unique<Node>(Dict{}));
      }

      return *this;
    }

    Builder &Builder::EndDict() {
      if (UnableUseEndDict()) {
        throw std::logic_error("Ending dict error"s);
      } else {
        AddNode(std::move(*nodes_stack_.back()));
      }
      return *this;
    }

    ArrayContext &Builder::StartArray() {
      if (UnableUseStartArray()) {
        throw std::logic_error("Starting array error"s);
      } else {
        nodes_stack_.push_back(std::make_unique<Node>(Array{}));
      }
      return *this;
    }

    Builder &Builder::EndArray() {
      if (UnableUseEndArray()) {
        throw std::logic_error("Ending array error"s);
      } else {
        AddNode(std::move(*nodes_stack_.back()));
      }
      return *this;
    }

    Node Builder::Build() const {
      if (UnableUseBuild()) {
        throw std::logic_error("Build error"s);
      } else {
        return root_;
      }
    }

    bool Builder::UnableAdd() const {
      return !(nodes_stack_.empty()
          || nodes_stack_.back()->IsArray()
          || nodes_stack_.back()->IsString());
    }

    bool Builder::NotNullNode() const {
      return !root_.IsNull();
    }

    bool Builder::UnableUseKey() const {
      return NotNullNode()
          || nodes_stack_.empty()
          || !nodes_stack_.back()->IsDict();
    }

    bool Builder::UnableUseValue() const {
      return NotNullNode()
          || UnableAdd();
    }

    bool Builder::UnableUseStartDict() const {
      return UnableUseValue();
    }

    bool Builder::UnableUseEndDict() const {
      return NotNullNode()
          || nodes_stack_.empty()
          || !nodes_stack_.back()->IsDict();
    }

    bool Builder::UnableUseStartArray() const {
      return UnableUseValue();
    }

    bool Builder::UnableUseEndArray() const {
      return NotNullNode()
          || nodes_stack_.empty()
          || !nodes_stack_.back()->IsArray();
    }

    bool Builder::UnableUseBuild() const {
      return !NotNullNode();
    }

    void Builder::AddNode(Node node) {
      nodes_stack_.pop_back();
      if (nodes_stack_.empty()) {
        root_ = std::move(node);
      } else if (nodes_stack_.back()->IsArray()) {
        nodes_stack_.back()->AsArray().push_back(std::move(node));
      } else {
        const Node key(nodes_stack_.back()->AsString());
        nodes_stack_.pop_back();
        nodes_stack_.back()->AsDict().emplace(key.AsString(), std::move(node));
      }
    }
  }



