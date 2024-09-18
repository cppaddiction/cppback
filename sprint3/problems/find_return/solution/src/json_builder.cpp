#include "json_builder.h"

namespace json {

    BaseItemContext::BaseItemContext(Builder& builder) : builder_(builder) {}

    Node BaseItemContext::Build() {
        return builder_.Build();
    }

    KeyItemContext BaseItemContext::Key(std::string key) {
        return builder_.Key(std::move(key));
    }

    BaseItemContext BaseItemContext::Value(Node::Value value) {
        return builder_.Value(std::move(value));
    }

    DictItemContext BaseItemContext::StartDict() {
        return builder_.StartDict();
    }

    ArrayItemContext BaseItemContext::StartArray() {
        return builder_.StartArray();
    }

    BaseItemContext BaseItemContext::EndDict() {
        return builder_.EndDict();
    }

    BaseItemContext BaseItemContext::EndArray() {
        return builder_.EndArray();
    }

}
