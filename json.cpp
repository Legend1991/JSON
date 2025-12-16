#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <stack>
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
    NIL     = 5, // Unfortunately NULL is already defined in C++
};

struct JSON_Value;
struct JSON_Parser
{
    static JSON_Value parse(std::istringstream&& isstream);
    static JSON_Value parse(const std::string& str);
    static JSON_Value parse(const char *c_str);
};

using JSON_Object  = std::map<std::string, JSON_Value>;
using JSON_Array   = std::vector<JSON_Value>;
using JSON_Variant = std::variant<bool, double, std::string, JSON_Array, JSON_Object, std::monostate>;

struct JSON_Value
{
    JSON_Variant value;
    
    constexpr JSON_Type type() const
    {
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
    
    const JSON_Array& array() const
    {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value);
    }
    
    const JSON_Object& object() const
    {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value);
    }
    
    JSON_Array& array()
    {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value);
    }
    
    JSON_Object& object()
    {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value);
    }
    
    bool is_null() const
    {
        return type() == JSON_Type::NIL;
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
    
    JSON_Value& operator[](const std::string& key)
    {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value)[key];
    }

    const JSON_Value& operator[](const std::string& key) const
    {
        assert(type() == JSON_Type::OBJECT);
        return std::get<JSON_Object>(value).at(key);
    }
    
    JSON_Value& operator[](int index)
    {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value)[index];
    }

    const JSON_Value& operator[](int index) const
    {
        assert(type() == JSON_Type::ARRAY);
        return std::get<JSON_Array>(value).at(index);
    }
    
    JSON_Value& operator=(const std::monostate& nil)
    {
        value = nil;
        return *this;
    }
    
    JSON_Value& operator=(bool boolean)
    {
        value = boolean;
        return *this;
    }
    
    JSON_Value& operator=(double number)
    {
        value = number;
        return *this;
    }
    
    JSON_Value& operator=(int number) {
        value = static_cast<double>(number);
        return *this;
    }
    
    JSON_Value& operator=(const JSON_Array &array)
    {
        value = array;
        return *this;
    }
    
    JSON_Value& operator=(const JSON_Object &object)
    {
        value = object;
        return *this;
    }
    
    JSON_Value& operator=(const std::string& str)
    {
        value = str;
        return *this;
    }
    
    JSON_Value& operator=(const char *cstr)
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
                if (buffer.size() > 1)
                {
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
                if (buffer.size() > 1)
                {
                    buffer.pop_back();
                }
                return buffer + "}";
            }
            default:
                return "null";
        };
    }
    
    std::string type_name() const
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
    
    static JSON_Value parse(std::istringstream& isstream)
    {
        return JSON_Parser::parse(std::move(isstream));
    }
    
    static JSON_Value parse(const std::string& str)
    {
        return JSON_Parser::parse(std::istringstream(str));
    }
    
    static JSON_Value parse(const char *c_str)
    {
        return JSON_Parser::parse(std::string(c_str));
    }
};

JSON_Value JSON_Parser::parse(std::istringstream&& isstream)
{
    JSON_Value root;
    std::stack<JSON_Value*> stack;
    stack.push(&root);
        
    std::string key;
    JSON_Type token_type = JSON_Type::NIL;
    size_t token_size = 0;
    std::stringstream token_buff;

    char c;
    while (isstream.read(&c, 1))
    {
        switch (c)
        {
            case '{': {
                switch (stack.top()->type())
                {
                    case JSON_Type::ARRAY: {
                        JSON_Value& value = stack.top()->array().emplace_back(JSON_Object{});
                        stack.push(&value);
                        break;
                    }
                    case JSON_Type::OBJECT: {
                        assert(!key.empty());
                        stack.top()->object()[key] = JSON_Value(JSON_Object{});
                        JSON_Value& value = stack.top()->object()[key];
                        stack.push(&value);
                        break;
                    }
                    default:
                        *stack.top() = JSON_Object{};
                };
                continue;
            }
            case '[': {        
                switch (stack.top()->type())
                {
                    case JSON_Type::ARRAY: {
                        JSON_Value& value = stack.top()->array().emplace_back(JSON_Array{});
                        stack.push(&value);
                        break;
                    }
                    case JSON_Type::OBJECT: {
                        assert(!key.empty());
                        stack.top()->object()[key] = JSON_Value(JSON_Array{});
                        JSON_Value& value = stack.top()->object()[key];
                        stack.push(&value);
                        break;
                    }
                    default:
                        *stack.top() = JSON_Array{};
                };
                continue;
            }
            case ':': {
                assert(token_type == JSON_Type::STRING);
                assert(token_size > 0);

                key = token_buff.str();
                // std::cout << "key: " << key << '\t' << (int)token_type << std::endl;
                token_buff = std::stringstream{};
                token_size = 0;
                token_type = JSON_Type::NIL;
                continue;
            }
            case '}':
            case ']':
            case ',': {
                if (token_size > 0)
                {
                    std::string token = token_buff.str();
                    if (token == "true" || token == "false")
                    {
                        token_type = JSON_Type::BOOLEAN;
                    }
                        
                    // std::cout << "token: " << token << '\t' << token_size << '\t' << (int)token_type << std::endl;
                        
                    JSON_Value value;
                    switch (token_type)
                    {
                        case JSON_Type::BOOLEAN:
                            value = token == "true";
                            break;
                        case JSON_Type::NUMBER:
                            value = std::stod(token);
                            break;
                        case JSON_Type::STRING:
                            value = token;
                            break;
                        default:
                            value = std::monostate{};
                    };

                    switch (stack.top()->type())
                    {
                        case JSON_Type::ARRAY:
                            stack.top()->array().push_back(value);
                            break;
                        case JSON_Type::OBJECT:
                            assert(!key.empty());
                            stack.top()->object()[key] = value;
                            break;
                        default:
                            break;
                    };
                    
                    token_buff = std::stringstream{};
                    token_size = 0;
                    token_type = JSON_Type::NIL;
                }
                else // '}' or ']' case
                {
                    stack.pop();
                }
                continue;
            }
            case '"': {
                token_type = JSON_Type::STRING;
                continue;
            }
        }

        token_buff << c;
        token_size++;
        
        if (std::isdigit(c) && token_type != JSON_Type::STRING)
        {
            token_type = JSON_Type::NUMBER;
        }
        
        if (!std::isdigit(c) && token_type == JSON_Type::NUMBER)
        {
            token_type = JSON_Type::NIL;
        }
        // std::cout << c << std::endl;
    }
        
    return root;
}
    
JSON_Value JSON_Parser::parse(const std::string& str)
{
    return JSON_Parser::parse(std::istringstream(str));
}
    
JSON_Value JSON_Parser::parse(const char *c_str)
{
    return JSON_Parser::parse(std::string(c_str));
}
 
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

    std::cout << json.to_string() << std::endl;
    std::cout << json["nested"]["name"].string() << std::endl;
    std::cout << json["arr"][1].string() << std::endl;
    
    const char *input = "[134234,\"sdfsdf\",true,false,null,[1,true,{\"arr\":[2,3],\"id\":\"XY23\",\"obj\":{\"key\":1}}]]";
    JSON_Value parsed = JSON_Value::parse(input);
    bool assert_parsed = strcmp(input, parsed.to_string().c_str()) == 0;
    std::cout << "assert parsed: " << (assert_parsed ? "true" : "false") << std::endl;
    std::cout << "parsed output: " << parsed.to_string() << std::endl;

    return 0;
}
