#pragma once
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node();
    Node(Value value); // can't put explicit keyword here because Builder::Value method requires this constructor to be non-explicit
    explicit Node(Array array);
    explicit Node(Dict map);
    explicit Node(int value);
    explicit Node(std::string value);
    explicit Node(double x);
    explicit Node(bool x);
    explicit Node(nullptr_t x);

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const Value& GetValue() const { return value_; }
    Value& GetValue() { return value_; }
    bool operator==(const Node& n) const {return GetValue()==n.GetValue();}
    bool operator!=(const Node& n) const {return !(*this==n);}

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;
    bool operator==(const Document& d) const {return GetRoot()==d.GetRoot()?true:false;}
    bool operator!=(const Document& d) const {return !(*this==d);}

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

std::string Print(const Node& node);

Document LoadJSON(const std::string& s);

}
