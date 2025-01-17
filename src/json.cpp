#include <fstream>

#include <uva/json.hpp>
#include <uva/core.hpp>

#define JSON_BUFFR_SIZE 512

static size_t indentation_level = 0;
#define INDENTATION_SIZE 1

void encode_indentation(std::string& buffer)
{
    static std::string indentation_string;

    if(indentation_string.size() > indentation_level*INDENTATION_SIZE) {
        indentation_string.resize(indentation_level*INDENTATION_SIZE);
    } else {
        indentation_string.resize(indentation_level*INDENTATION_SIZE, '\t');
    }

    buffer.append(indentation_string);
}

void json_pretty_begin_object(std::string& buffer)
{
    buffer.push_back('\n');
    encode_indentation(buffer);
}

void encode_var(const var& value, std::string& buffer, bool pretty = false)
{
    if(value.is_a<var::string>())
    {
        buffer.append(value.to_typed_s());
    } else if(value.is_a<var::array>())
    { 
        var::array_type array = value.as<var::array>();
        buffer.push_back('[');
        
        size_t count = array.size();

        for(size_t i = 0; i < count; ++i) {
            encode_var(array[i], buffer, pretty);

            if(i < count-1)
            {
                buffer.push_back(',');
            }

            buffer.reserve(buffer.size()+20);
        }

        buffer.push_back(']');
    }
    else if(value.is_a<var::dictionary>()) {
        auto map = value.as<var::dictionary>();

        indentation_level++;

        if(pretty) {
            if(buffer.size()) {
                buffer.push_back(' ');
            }
            buffer.push_back('{');
            buffer.push_back('\n');
        } else {
            buffer.push_back('{');
        }

        size_t index = 0;
        size_t count = map.size();

        for(const auto& pair : map)
        {
            if(pretty) {
                encode_indentation(buffer);
            }

            buffer.push_back('\"');
            buffer.append(pair.first);
            buffer.push_back('\"');

            buffer.push_back(':');

            if(pretty) {
                buffer.push_back(' ');
            }

            encode_var(pair.second, buffer, pretty);

            if(index < count-1)
            {
                buffer.push_back(',');
            }

            if(pretty) {
                buffer.push_back('\n');
            }

            index++;
        }

        if(pretty) {
            if(indentation_level) {
                indentation_level--;
            }
            
            encode_indentation(buffer);
        }

        buffer.push_back('}');
    }
    else {
        buffer += value.to_typed_s();
    }
}

std::string uva::json::enconde(const var& values, bool pretty)
{
    indentation_level = 0;

    std::string buffer;
    buffer.reserve(512);
    //recurse loop in values
    encode_var(values, buffer, pretty);

    return buffer;
}

void uva::json::encode(const var& values, const std::filesystem::path& path)
{
    std::string text_json = enconde(values, true);

    std::ofstream stream(path, std::ios::binary);

    if(!stream.is_open()) {
        throw std::runtime_error("cannot open output file");
    }

    stream.write(text_json.data(), text_json.size());
}

void next_non_white_space(std::string_view& sv)
{
    while(sv.size() && isspace(sv[0]))
    {
        sv.remove_prefix(1);
    }
}

std::string_view extract_string(std::string_view& sv)
{
    char quote = sv[0];
    sv.remove_prefix(1);

    size_t count = 0;
    while(count < sv.size()-1 && sv[count] != quote)
    {
        count++;
    }

    std::string_view str = sv.substr(0, count);
    sv.remove_prefix(str.size()+1);

    return str;
}

#define THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION() throw std::runtime_error(std::format("failed to parse json: unexpected token at {}", text_view.data() - begin))
#define THROW_UNEXPECTED_END_OF_INPUT() \
if(!text_view.size()) {\
    throw std::runtime_error(std::format("failed to parse json: unexpected end of input"));\
}

var json_parse_object(std::string_view& text_view, const char* begin);
var json_parse_array(std::string_view& text_view, const char* begin);

var json_parse_value(std::string_view& text_view, const char* begin)
{
    //todo switch
    if(text_view[0] == '"') {
        std::string_view value = extract_string(text_view);

        return uva::string::unescape<char>(value);
    } else if(text_view[0] == '[')
    {
        return json_parse_array(text_view, begin);
    }
    else if(text_view[0] == '{')
    {
        return json_parse_object(text_view, begin);
    }
    //todo: map
    else if(text_view.starts_with("true"))
    {
        text_view.remove_prefix(4);
        return true;
    }
    else if(text_view.starts_with("false"))
    {
        text_view.remove_prefix(5);
        return false;
    }
    else if(text_view.starts_with("null"))
    {
        text_view.remove_prefix(4);
        return null;
    }
    else
    {
        bool is_negative = false;
        bool is_double = false;

        if(text_view.starts_with('-')) {
            is_negative = true;
            text_view.remove_prefix(1);

            THROW_UNEXPECTED_END_OF_INPUT();
        }

        std::string str;

        while(text_view.size() && !isspace(text_view[0]) && text_view[0] != ',' && text_view[0] != '}') {
            THROW_UNEXPECTED_END_OF_INPUT();

            if(text_view[0] == '.') {
                if(is_double) {
                    throw std::runtime_error("error: unexpected '.'");
                } else {
                    is_double = true;
                }
            } else {
                if(!isdigit(text_view[0])) {
                    const std::string n = "null";
                    if(text_view.starts_with(n)) {
                        return null;
                        text_view.remove_prefix(n.size());
                        break;
                    } else {
                        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
                    }
                }
            }

            str.push_back(text_view.front());
            text_view.remove_prefix(1);
        }

        if(!str.empty() && is_double) {
            double d = std::stod(str);
            return is_negative ? (d * -1) : d;
        } else if (!str.empty()) {
            size_t i = std::stol(str);
            return is_negative ? (i * -1) : i;
        }
    }

    throw std::runtime_error("failed to parse json: unexpected token");
}

var json_parse_array(std::string_view& text_view, const char* begin)
{
    std::vector<var> array;

    if(text_view.empty()) {
        return array;
    }
    
    if(!text_view.starts_with('[')) {
        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    text_view.remove_prefix(1);
    next_non_white_space(text_view);

    while(text_view.size())
    {
        if(text_view.starts_with(']')) {
            break;
        }

        THROW_UNEXPECTED_END_OF_INPUT();

        array.push_back(json_parse_value(text_view, begin));

        next_non_white_space(text_view);

        if(text_view.starts_with(',')) {
            text_view.remove_prefix(1);
            next_non_white_space(text_view);
            continue;
        }

        if(text_view.starts_with(']')) {
            break;
        }

        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    if(!text_view.starts_with(']')) {
        THROW_UNEXPECTED_END_OF_INPUT();
    }

    text_view.remove_prefix(1);

    return array;
}

var json_parse_object(std::string_view& text_view, const char* begin)
{
    var dictionary = uva::core::var::dictionary();
    var::dictionary_type& map = dictionary.as<var::dictionary>();

    if(text_view.empty()) {
        return map;
    }
    
    next_non_white_space(text_view);

    if(!text_view.starts_with('{')) {
        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    text_view.remove_prefix(1);
    next_non_white_space(text_view);

    if(!text_view.starts_with('}')) {

        while(text_view.size())
        {
            THROW_UNEXPECTED_END_OF_INPUT();

            if(!text_view.starts_with('\'') && !text_view.starts_with('\"')) {
                THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
            }

            std::string_view key = extract_string(text_view);
            
            next_non_white_space(text_view);

            THROW_UNEXPECTED_END_OF_INPUT();

            if(!text_view.starts_with(':')) {
                THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
            }

            text_view.remove_prefix(1);
            next_non_white_space(text_view);

            THROW_UNEXPECTED_END_OF_INPUT();

            var value = json_parse_value(text_view, begin);

            map.insert({std::string(key), var(std::move(value))});

            next_non_white_space(text_view);

            if(text_view.starts_with('}')) {
                break;
            }

            if(text_view.starts_with(',')) {
                text_view.remove_prefix(1);
                next_non_white_space(text_view);
                continue;
            }

            THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
        }

        next_non_white_space(text_view);
    }

    if(!text_view.starts_with('}')) {
        THROW_UNEXPECTED_END_OF_INPUT();
    }

    text_view.remove_prefix(1);

    return dictionary;
}

var uva::json::decode(const std::string& text)
{
    std::string_view text_view(text);
    const char* begin = text.data();

    var json = json_parse_object(text_view, begin);

    next_non_white_space(text_view);

    if(text_view.size()) {
        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    return json;
}
var uva::json::decode(const std::filesystem::path &path)
{
    std::string text;
    {
        std::ifstream stream(path, std::ios::binary);

        if(!stream.is_open()) {
            throw std::runtime_error("cannot open input file");
        }

        stream.seekg(0, std::ios::end);

        auto file_len = stream.tellg();

        stream.seekg(0, std::ios::beg);

        text.resize(file_len);

        stream.read(text.data(), file_len);
    }
    return uva::json::decode(text);
}