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
        Builder()
        {
            nodes_stack_.push_back(&root_);
        }

        KeyItemContext Key(std::string key)
        {
            auto node=nodes_stack_.back();
            if(node->IsMap())
            {
                keys_.push_back(key);
            }
            return KeyItemContext(*this);
        }

        BaseItemContext Value(Node::Value value)
        {
            auto node=nodes_stack_.back();
            if(node->IsNull())
            {
                node->GetValue()=std::move(value);
                nodes_stack_.pop_back();
            }
            else if(node->IsArray())
            {
                std::get<Array>(node->GetValue()).push_back(Node(value));
            }
            else
            {
                std::get<Dict>(node->GetValue())[keys_.back()]=std::move(value);
            }
            return BaseItemContext(*this);
        }

        DictItemContext StartDict()
        {
            auto node=nodes_stack_.back();
            if(node->IsNull())
            {
                node->GetValue()=Dict{};
            }
            else if(node->IsArray())
            {
                std::get<Array>(node->GetValue()).push_back(Node(Dict{}));
                nodes_stack_.push_back(&std::get<Array>(node->GetValue()).back());
            }
            else
            {
                std::get<Dict>(node->GetValue())[keys_.back()]=Node(Dict{});
                nodes_stack_.push_back(&std::get<Dict>(node->GetValue())[keys_.back()]);
            }
            return DictItemContext(*this);
        }

        ArrayItemContext StartArray()
        {
            auto node=nodes_stack_.back();
            if(node->IsNull())
            {
                node->GetValue()=Array{};
            }
            else if(node->IsArray())
            {
                std::get<Array>(node->GetValue()).push_back(Node(Array{}));
                nodes_stack_.push_back(&std::get<Array>(node->GetValue()).back());
            }
            else
            {
                std::get<Dict>(node->GetValue())[keys_.back()]=Node(Array{});
                nodes_stack_.push_back(&std::get<Dict>(node->GetValue())[keys_.back()]);
            }
            return ArrayItemContext(*this);
        }

        BaseItemContext EndDict()
        {
            auto node=nodes_stack_.back();
            if(node->IsMap())
            {
                nodes_stack_.pop_back();
            }
            return BaseItemContext(*this);
        }

        BaseItemContext EndArray()
        {
            auto node=nodes_stack_.back();
            if(node->IsArray())
            {
                nodes_stack_.pop_back();
            }
            return BaseItemContext(*this);
        }

        Node Build()
        {
            if(nodes_stack_.size()==0)
            {
                return root_;
            }
            else
            {
                throw std::logic_error("build failed");
            }
        }

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
        std::vector<std::string> keys_;
};

}
