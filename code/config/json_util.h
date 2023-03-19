#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <iostream>
#include <utility>
#include <variant>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <fstream>
#include <sstream>

struct Node;
using Null = std::monostate;
using Bool = bool;
using Int = int32_t;
using Float = double;
using String = std::string;
using Array = std::vector<Node>;
using Object = std::map<std::string, Node>;
using Value = std::variant<Null, Bool, Int, Float, String, Array, Object>;

struct Node {
    Value value;

    explicit Node(Value _value) : value(std::move(_value)) {}

    Node() : value(Null{}) {}

    Node &operator[](const std::string &key) {
        if (auto object = std::get_if<Object>(&value)) {
            return (*object)[key];
        }
        throw std::runtime_error("not an object");
    }

    Node operator[](size_t index) {
        if (auto array = std::get_if<Array>(&value)) {
            return array->at(index);
        }
        throw std::runtime_error("not an array");
    }

    int getInt() {
        if (auto object = std::get_if<Int>(&value)) {
            return *object;
        }
        throw std::runtime_error("not an int");
    }

    std::string getString() {
        if (auto object = std::get_if<String>(&value)) {
            return *object;
        }
        throw std::runtime_error("not a string");
    }

    bool getBool() {
        if (auto object = std::get_if<Bool>(&value)) {
            return *object;
        }
        throw std::runtime_error("not a bool");
    }

    void push(const Node &rhs) {
        if (auto array = std::get_if<Array>(&value)) {
            array->push_back(rhs);
        }
    }
};

struct JsonParser {
    std::string_view json_str;
    size_t pos = 0;

    void parse_whitespace() {
        while (pos < json_str.size() && std::isspace(json_str[pos])) {
            ++pos;
        }
    }

    std::optional<Value> parse_null() {
        if (json_str.substr(pos, 4) == "null") {
            pos += 4;
            return Null{};
        }
        return {};
    }

    std::optional<Value> parse_true() {
        if (json_str.substr(pos, 4) == "true") {
            pos += 4;
            return true;
        }
        return {};
    }

    std::optional<Value> parse_false() {
        if (json_str.substr(pos, 5) == "false") {
            pos += 5;
            return false;
        }
        return {};
    }

    std::optional<Value> parse_number() {
        size_t endPos = pos;
        while (endPos < json_str.size() && (
                std::isdigit(json_str[endPos]) ||
                json_str[endPos] == 'e' ||
                json_str[endPos] == '.')) {
            ++endPos;
        }
        std::string number = std::string{json_str.substr(pos, endPos - pos)};
        pos = endPos;
        static auto is_Float = [](std::string &number) {
            return number.find('.') != std::string::npos ||
                   number.find('e') != std::string::npos;
        };
        if (is_Float(number)) {
            try
            {
                Float ret = std::stod(number);
                return ret;
            }
            catch (...) {
                return {};
            }
        } else {
            try
            {
                Int ret = std::stoi(number);
                return ret;
            }
            catch (...) {
                return {};
            }
        }
    }

    std::optional<Value> parse_string() {
        ++pos;// "
        size_t endPos = pos;
        while (pos < json_str.size() && json_str[endPos] != '"') {
            ++endPos;
        }
        std::string str = std::string{json_str.substr(pos, endPos - pos)};
        pos = endPos + 1;// "
        return str;
    }

    std::optional<Value> parse_array() {
        ++pos;// [
        Array arr;
        while (pos < json_str.size() && json_str[pos] != ']') {
            auto value = parse_value();
            arr.emplace_back(value.value());
            parse_whitespace();
            if (pos < json_str.size() && json_str[pos] == ',') {
                ++pos;// ,
            }
            parse_whitespace();
        }
        ++pos;// ]
        return arr;
    }

    std::optional<Value> parse_object() {
        ++pos;// {
        Object obj;
        while (pos < json_str.size() && json_str[pos] != '}') {
            auto key = parse_value();
            parse_whitespace();
            if (!std::holds_alternative<String>(key.value())) {
                return {};
            }
            if (pos < json_str.size() && json_str[pos] == ':') {
                ++pos;// ,
            }
            parse_whitespace();
            auto val = parse_value();
            obj[std::get<String>(key.value())] = static_cast<Node>(val.value());
            parse_whitespace();
            if (pos < json_str.size() && json_str[pos] == ',') {
                ++pos;// ,
            }
            parse_whitespace();
        }
        ++pos;// }
        return obj;

    }

    std::optional<Value> parse_value() {
        parse_whitespace();
        switch (json_str[pos]) {
            case 'n':
                return parse_null();
            case 't':
                return parse_true();
            case 'f':
                return parse_false();
            case '"':
                return parse_string();
            case '[':
                return parse_array();
            case '{':
                return parse_object();
            default:
                return parse_number();
        }
    }

    std::optional<Node> parse() {
        parse_whitespace();
        auto value = parse_value();
        if (!value) {
            return {};
        }
        return Node{*value};
    }
};

inline std::optional<Node> parser(std::string_view json_str) {
    JsonParser p{json_str};
    return p.parse();
}

class JsonGenerator {
public:
    static std::string generate(const Node &node) {
        return std::visit(
                [](auto &&arg) -> std::string {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, Null>) {
                        return "null";
                    } else if constexpr (std::is_same_v<T, Bool>) {
                        return arg ? "true" : "false";
                    } else if constexpr (std::is_same_v<T, Int>) {
                        return std::to_string(arg);
                    } else if constexpr (std::is_same_v<T, Float>) {
                        return std::to_string(arg);
                    } else if constexpr (std::is_same_v<T, String>) {
                        return generate_string(arg);
                    } else if constexpr (std::is_same_v<T, Array>) {
                        return generate_array(arg);
                    } else if constexpr (std::is_same_v<T, Object>) {
                        return generate_object(arg);
                    }
                },
                node.value);
    }

    static std::string generate_string(const String &str) {
        std::string json_str = "\"";
        json_str += str;
        json_str += '"';
        return json_str;
    }

    static std::string generate_array(const Array &array) {
        std::string json_str = "[";
        for (const auto &node: array) {
            json_str += generate(node);
            json_str += ',';
        }
        if (!array.empty()) json_str.pop_back();
        json_str += ']';
        return json_str;
    }

    static std::string generate_object(const Object &object) {
        std::string json_str = "{";
        for (const auto &[key, node]: object) {
            json_str += generate_string(key);
            json_str += ':';
            json_str += generate(node);
            json_str += ',';
        }
        if (!object.empty()) json_str.pop_back();
        json_str += '}';
        return json_str;
    }
};

inline std::string generate(const Node &node) { return JsonGenerator::generate(node); }

inline std::ostream &operator<<(std::ostream &out, const Node &t) {
    out << JsonGenerator::generate(t);
    return out;
}

#endif //JSON_UTIL_H
