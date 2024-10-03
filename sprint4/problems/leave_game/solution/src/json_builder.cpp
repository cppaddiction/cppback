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

    Builder::Builder()
    {
        nodes_stack_.push_back(&root_);
    }

    KeyItemContext Builder::Key(std::string key)
    {
        auto node = nodes_stack_.back();
        if (node->IsMap())
        {
            keys_.push_back(key);
        }
        return KeyItemContext(*this);
    }

    BaseItemContext Builder::Value(Node::Value value)
    {
        auto node = nodes_stack_.back();
        if (node->IsNull())
        {
            node->GetValue() = std::move(value);
            nodes_stack_.pop_back();
        }
        else if (node->IsArray())
        {
            std::get<Array>(node->GetValue()).push_back(Node(value));
        }
        else
        {
            std::get<Dict>(node->GetValue())[keys_.back()] = std::move(value);
        }
        return BaseItemContext(*this);
    }

    DictItemContext Builder::StartDict()
    {
        auto node = nodes_stack_.back();
        if (node->IsNull())
        {
            node->GetValue() = Dict{};
        }
        else if (node->IsArray())
        {
            std::get<Array>(node->GetValue()).push_back(Node(Dict{}));
            nodes_stack_.push_back(&std::get<Array>(node->GetValue()).back());
        }
        else
        {
            std::get<Dict>(node->GetValue())[keys_.back()] = Node(Dict{});
            nodes_stack_.push_back(&std::get<Dict>(node->GetValue())[keys_.back()]);
        }
        return DictItemContext(*this);
    }

    ArrayItemContext Builder::StartArray()
    {
        auto node = nodes_stack_.back();
        if (node->IsNull())
        {
            node->GetValue() = Array{};
        }
        else if (node->IsArray())
        {
            std::get<Array>(node->GetValue()).push_back(Node(Array{}));
            nodes_stack_.push_back(&std::get<Array>(node->GetValue()).back());
        }
        else
        {
            std::get<Dict>(node->GetValue())[keys_.back()] = Node(Array{});
            nodes_stack_.push_back(&std::get<Dict>(node->GetValue())[keys_.back()]);
        }
        return ArrayItemContext(*this);
    }

    BaseItemContext Builder::EndDict()
    {
        auto node = nodes_stack_.back();
        if (node->IsMap())
        {
            nodes_stack_.pop_back();
        }
        return BaseItemContext(*this);
    }

    BaseItemContext Builder::EndArray()
    {
        auto node = nodes_stack_.back();
        if (node->IsArray())
        {
            nodes_stack_.pop_back();
        }
        return BaseItemContext(*this);
    }

    Node Builder::Build()
    {
        if (nodes_stack_.size() == 0)
        {
            return root_;
        }
        else
        {
            throw std::logic_error("build failed");
        }
    }

}
