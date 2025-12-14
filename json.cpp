#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <vector>
#include <map>
#include <format>

enum class JSON_Type
{
    BOOLEAN = 0,
    NUMBER  = 1,
    STRING  = 2,
    ARRAY   = 3,
    OBJECT  = 4,
    NIL     = 5,    // Unfortunately NULL is already defined in C++
};

struct JSON_Value;

using JSON_Object  = std::map<std::string, JSON_Value>;
using JSON_Array   = std::vector<JSON_Value>;
using JSON_Variant = std::variant<bool, double, std::string, JSON_Array, JSON_Object, std::monostate>;

struct JSON_Value
{
    JSON_Variant value;
    
    constexpr JSON_Type type() const {
        return static_cast<JSON_Type>(value.index());
    }
    
    bool boolean() const
    {
        assert(type() == JSON_Type::BOOLEAN);
        return std::get<bool>(value);
    }
    
    double number() const
    {
        assert(type() == JSON_Type::NUMBER);
        return std::get<double>(value);
    }
    
    std::string string() const
    {
        assert(type() == JSON_Type::STRING);
        return std::get<std::string>(value);
    }
    
    JSON_Array array() const
    {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value);
    }
    
    JSON_Object object() const
    {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value);
    }
    
    bool is_null() const {
        return type() == JSON_Type::NIL;
    }
    
    size_t size() const {
        if (type() == JSON_Type::ARRAY) return array().size();
        if (type() == JSON_Type::OBJECT) return object().size();
        if (type() == JSON_Type::STRING) return string().size();
        return 0;
    }
    
    JSON_Value() : value(std::monostate{}) {}
    JSON_Value(bool boolean) : value(boolean) {}
    JSON_Value(double number) : value(number) {}
    JSON_Value(int number) : JSON_Value(static_cast<double>(number)) {}
    JSON_Value(float number) : JSON_Value(static_cast<double>(number)) {}
    JSON_Value(const JSON_Array &array) : value(array) {}
    JSON_Value(const JSON_Object &object) : value(object) {}
    JSON_Value(const char *cstr) : value(std::string(cstr)) {}
    JSON_Value(const std::string& str) : value(str) {}
    
    JSON_Value& operator[](const std::string& key) {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value)[key];
    }

    const JSON_Value& operator[](const std::string& key) const {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value).at(key);
    }
    
    JSON_Value& operator[](int index) {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value)[index];
    }

    const JSON_Value& operator[](int index) const {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value).at(index);
    }
    
    JSON_Value& operator =(const std::monostate& nil)
    {
        value = nil;
        return *this;
    }
    
    JSON_Value& operator =(bool boolean)
    {
        value = boolean;
        return *this;
    }
    
    JSON_Value& operator =(double number)
    {
        value = number;
        return *this;
    }
    
    JSON_Value& operator=(int number) {
        value = static_cast<double>(number);
        return *this;
    }
    
    JSON_Value& operator =(const JSON_Array &array)
    {
        value = array;
        return *this;
    }
    
    JSON_Value& operator =(const JSON_Object &object)
    {
        value = object;
        return *this;
    }
    
    JSON_Value& operator =(const std::string& str)
    {
        value = str;
        return *this;
    }
    
    JSON_Value& operator =(const char *cstr)
    {
        value = std::string(cstr);
        return *this;
    }
    
    std::string to_string() const
    {
        switch (type())
        {
            case JSON_Type::BOOLEAN:
                return std::format("{}", boolean());
            case JSON_Type::NUMBER:
                return std::format("{}", number());
            case JSON_Type::STRING:
                return std::format("\"{}\"", string());
            case JSON_Type::ARRAY: {
                std::stringstream ss;
                ss << "[";
                for (const auto& v : array())
                {
                    ss << v.to_string() << ",";
                }
                std::string buffer = ss.str();
                if (buffer.size() > 1) {
                    buffer.pop_back();
                }
                return buffer + "]";
            }
            case JSON_Type::OBJECT: {
                std::stringstream ss;
                ss << "{";
                for (const auto& [k, v] : object())
                {
                    ss << std::quoted(k) << ":" << v.to_string() << ",";
                }
                std::string buffer = ss.str();
                if (buffer.size() > 1) {
                    buffer.pop_back();
                }
                return buffer + "}";
            }
            default:
                return "null";
        };
    }
    
    std::string type_to_string() const
    {
        switch (type())
        {
            case JSON_Type::BOOLEAN:
                return "boolean";
            case JSON_Type::NUMBER:
                return "number";
            case JSON_Type::STRING:
                return "string";
            case JSON_Type::ARRAY:
                return "array";
            case JSON_Type::OBJECT:
                return "object";
            default:
                return "null";
        };
    }
};
 
int main()
{
    JSON_Object data;
    data["id"] = 123.45f;
    data["flag"] = true;
    data["name"] = "test_str";
    data["what"] = std::monostate{};
    data["arr"] = {1.0f, "sdfsdf"};

    JSON_Object nested;
    nested["name"] = "Jon";
    nested["id"] = 1234.0f;
    data["nested"] = nested;

    JSON_Value json = data;

    std::cout << "size: " << json.size() << std::endl;
    std::cout << json.to_string() << std::endl;
    std::cout << json["nested"]["name"].string() << std::endl;
    std::cout << json["arr"][0].number() << std::endl;

    return 0;
}
