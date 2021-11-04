#pragma once

#include "json.h"

#include <string>
#include <memory>
#include <vector>

namespace json
  {
    class BuildConstructor;
    class BuildContextFirst;
    class BuildContextSecond;
    class KeyContext;
    class ValueKeyContext;
    class ValueArrayContext;
    class DictContext;
    class ArrayContext;
    class Builder;

    class BuildConstructor {
     public:
      explicit BuildConstructor(Builder &builder);

      virtual ~BuildConstructor() = default;
     protected:
      Builder &builder_;
    };

    class BuildContextFirst
        : public BuildConstructor {
     public:
      explicit BuildContextFirst(Builder &builder);

      virtual DictContext &StartDict() = 0;
      virtual ArrayContext &StartArray() = 0;
    };

    class BuildContextSecond
        : public BuildConstructor {
     public:
      explicit BuildContextSecond(Builder &builder);

      virtual KeyContext &Key(const std::string &key) = 0;
      virtual Builder &EndDict() = 0;
    };

    class KeyContext
        : public BuildContextFirst {
     public:
      explicit KeyContext(Builder &builder);
      virtual ValueKeyContext &Value(const Node::Value &value) = 0;
    };

    class ValueKeyContext
        : public BuildContextSecond {
     public:
      explicit ValueKeyContext(Builder &builder);
    };

    class ValueArrayContext
        : public BuildContextFirst {
     public:
      explicit ValueArrayContext(Builder &builder);

      virtual ValueArrayContext &Value(const Node::Value &value) = 0;
      virtual Builder &EndArray() = 0;
    };

    class DictContext
        : public BuildContextSecond {
     public:
      explicit DictContext(Builder &builder);
    };

    class ArrayContext
        : public ValueArrayContext {
     public:
      explicit ArrayContext(Builder &builder);
    };

    class Builder final
        : virtual public KeyContext
            , virtual public ValueKeyContext
            , virtual public DictContext
            , virtual public ArrayContext {
     public:
      Builder();

      KeyContext &Key(const std::string &key) override;
      Builder &Value(const Node::Value &value);
      DictContext &StartDict() override;
      ArrayContext &StartArray() override;
      Builder &EndDict() override;
      Builder &EndArray() override;

      Node Build() const;

     private:
      bool UnableAdd() const;
      bool NotNullNode() const;
      bool UnableUseKey() const;
      bool UnableUseValue() const;
      bool UnableUseStartDict() const;
      bool UnableUseEndDict() const;
      bool UnableUseStartArray() const;
      bool UnableUseEndArray() const;
      bool UnableUseBuild() const;

      void AddNode(Node node);

      Node root_ = nullptr;
      std::vector<std::unique_ptr<Node>> nodes_stack_;
    };

  }
