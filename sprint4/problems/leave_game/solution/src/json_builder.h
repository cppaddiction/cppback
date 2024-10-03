#pragma once
#include "json.h"

namespace json {

class Builder;
class DictItemContext;
class ArrayItemContext;
class KeyItemContext;

class BaseItemContext {
    public:
        BaseItemContext(Builder& builder);
        Node Build();
        KeyItemContext Key(std::string key);
        BaseItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        BaseItemContext EndDict();
        BaseItemContext EndArray();

    private:
        Builder& builder_;
};

class DictItemContext : public BaseItemContext {
    public:
        DictItemContext(BaseItemContext base) : BaseItemContext(base) {}
        Node Build() = delete;
        BaseItemContext Value(Node::Value value) = delete;
        BaseItemContext EndArray() = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
};

class ArrayItemContext : public BaseItemContext {
    public:
        ArrayItemContext(BaseItemContext base) : BaseItemContext(base) {}
        ArrayItemContext Value(Node::Value value) { return BaseItemContext::Value(std::move(value)); }
        Node Build() = delete;
        KeyItemContext Key(std::string key) = delete;
        BaseItemContext EndDict() = delete;
};

class KeyItemContext : public BaseItemContext {
    public:
        KeyItemContext(BaseItemContext base) : BaseItemContext(base) {}
        DictItemContext Value(Node::Value value) { return BaseItemContext::Value(std::move(value)); }
        Node Build() = delete;
        KeyItemContext Key(std::string key) = delete;
        BaseItemContext EndDict() = delete;
        BaseItemContext EndArray() = delete;
};

class Builder {
    public:
        Builder();
        KeyItemContext Key(std::string key);
        BaseItemContext Value(Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        BaseItemContext EndDict();
        BaseItemContext EndArray();
        Node Build();

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        std::vector<std::string> keys_;
};

}
