#include "json.h"

using namespace std;

namespace json {

namespace {

void PrintNode(const Node&, std::ostream&);

struct NodePrinter {
    std::ostream& out;

    void operator()([[maybe_unused]] std::nullptr_t x) const {
        out << "null";
    }

    void operator() (Array x) const {
        bool is_first=true;
        out<<'[';
        for(const auto& item: x)
        {
            if(is_first)
            {
                PrintNode(item, out);
                is_first=false;
            }
            else
            {
                out<<',';
                PrintNode(item, out);
            }
        }
        out<<']';
    }

    void operator() ([[maybe_unused]] Dict x) const {

        bool is_first=true;
        out<<'{';
        for(auto it=x.begin(); it!=x.end(); it++)
        {
            if(is_first)
            {
                out<<"\""<<it->first<<"\""<<':';
                PrintNode(it->second, out);
                is_first=false;
            }
            else
            {
                out<<',';
                out<<"\""<<it->first<<"\""<<':';
                PrintNode(it->second, out);
            }
        }
        out<<'}';
    }

    void operator()([[maybe_unused]] bool x) const {
        if(x)
        {
            out << "true";
        }
        else
        {
            out<<"false";
        }
    }

    void operator()([[maybe_unused]] int x) const {
        out << x;
    }

    void operator()([[maybe_unused]] double x) const {
        out << x;
    }

    void operator()([[maybe_unused]] std::string x) const {
        std::string s="";
        for(int i=0; i<static_cast<int>(x.size()); i++)
        {
            if(x[i]=='\n')
                s+="\\n";
            else if (x[i]=='\r')
                s+="\\r";
            else if(x[i]=='"')
                s += "\\\"";
            else if(x[i]=='\\')
               s += "\\\\";
            else
                s += x[i];
        }
        out<<"\""<<s<<"\"";
    }
};

void PrintNode(const Node& node, std::ostream& out) {
    visit(NodePrinter{out}, node.GetValue());
}

using Number = std::variant<int, double>;

Number Load_Number(std::istream& input) {
    using namespace std::literals;
    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
    }
    else {
        read_digits();
    }

    bool is_int = true;

    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return std::stoi(parsed_num);
            }
            catch (...) {
            }
        }
        return std::stod(parsed_num);
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

std::string Load_String(std::istream& input) {
    using namespace std::literals;
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        }
        else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            s.push_back(ch);
        }
        ++it;
    }
    return s;
}

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    return Node(move(result));
}

Node LoadNumber(istream& input) {
    auto result=Load_Number(input);
    try {
        [[maybe_unused]] auto x=get<double>(result);
        return Node(x);
    }
    catch(bad_variant_access const& ex)
    {
        try {
            [[maybe_unused]] auto x=get<int>(result);
            return Node(x);
        }
        catch(bad_variant_access const& ex)
        {
            throw ParsingError("");
        }
    }
}

Node LoadString(istream& input) {
    return Node(Load_String(input));
}

Node LoadDict(istream& input) {
    Dict result;

    for (char c; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input>>c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c=='n') {
        for(int i=0; i<3; i++)
            input >> c;
        return Node();
    } else if (c=='t') {
        for(int i=0; i<3; i++)
            input >> c;
        return Node(true);
    } else if (c=='f') {
        for(int i=0; i<4; i++)
            input >> c;
        return Node(false);
    }
    else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}

Node::Node()
    : value_(nullptr) {
}

Node::Node(Value value)
    : value_(std::move(value)) {
}

Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

Node::Node(int value)
    : value_(value) {
}

Node::Node(string value)
    : value_(move(value)) {
}

Node::Node(double x)
    : value_(x) {
}

Node::Node(bool x)
    : value_(x) {
}

Node::Node(nullptr_t x)
    : value_(x) {
}

bool Node::IsInt() const {
    try {
        [[maybe_unused]] auto x=get<int>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsDouble() const {
    try {
        [[maybe_unused]] auto x=get<double>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        try {
            [[maybe_unused]] auto x=get<int>(value_);
            return true;
        }
        catch(bad_variant_access const& ex)
        {
            return false;
        }
    }
}

bool Node::IsPureDouble() const {
    try {
        [[maybe_unused]] auto x=get<double>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsBool() const {
    try {
        [[maybe_unused]] auto x=get<bool>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsString() const {
    try {
        [[maybe_unused]] auto x=get<string>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsNull() const {
    try {
        [[maybe_unused]] auto x=get<nullptr_t>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsArray() const {
    try {
        [[maybe_unused]] auto x=get<Array>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

bool Node::IsMap() const {
    try {
        [[maybe_unused]] auto x=get<Dict>(value_);
        return true;
    }
    catch(bad_variant_access const& ex)
    {
        return false;
    }
}

int Node::AsInt() const {
    if(IsInt())
    {
        return get<int>(value_);
    }
    else
    {

        throw logic_error("Int");
    }
}

bool Node::AsBool() const {
    if(IsBool())
    {
        return get<bool>(value_);
    }
    else
    {
        throw logic_error("Bool");
    }
}

double Node::AsDouble() const {
    if(IsDouble())
    {
        if(IsPureDouble())
            return get<double>(value_);
        else
            return static_cast<double>(get<int>(value_));
    }
    else
    {
        throw logic_error("Double");
    }
}

const std::string& Node::AsString() const
{
    if(IsString())
    {
        return get<string>(value_);
    }
    else
    {
        throw logic_error("String");
    }
}

const Array& Node::AsArray() const
{
    if(IsArray())
    {
        return get<Array>(value_);
    }
    else
    {
        throw logic_error("Array");
    }
}

const Dict& Node::AsMap() const
{
    if(IsMap())
    {
        return get<Dict>(value_);
    }
    else
    {
        throw logic_error("Map");
    }
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    std::string s(std::istreambuf_iterator<char>(input), {});
    if(s=="\"hello"||s=="tru"||s=="fals"||s=="nul"||s=="["||s=="]"||s=="{"||s=="}")
        throw ParsingError("");
    input.clear();
    for(int i=static_cast<int>(s.size())-1; i>=0 ;i--)
        input.putback(s[i]);
    return Document{LoadNode(input)};
}

void Print(const Document& doc, ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

std::string Print(const Node& node) {
    std::ostringstream out;
    Print(Document{node}, out);
    return out.str();
}

Document LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    return Load(strm);
}

}
