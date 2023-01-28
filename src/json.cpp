#include "json.hpp"

#include <core.hpp>

#define JSON_BUFFR_SIZE 512

uva::json::json(std::map<var, var>&& values)
{
    m_values = std::forward<std::map<var, var>&&>(values);
}

std::string uva::json::enconde() const
{
    return enconde(m_values);
}

void encode_var(const var& value, std::string& buffer)
{
    if(value.type == var::var_type::string)
    {
        buffer.push_back('"');
        buffer += value.as<var::var_type::string>();
        buffer.push_back('"');
    } else if(value.type == var::var_type::array)
    { 
        buffer.push_back('[');
        size_t count = value.size();

        for(size_t i = 0; i < count; ++i) {
            encode_var(value[i], buffer);

            if(i < count-1)
            {
                buffer.push_back(',');
            }

            buffer.reserve(buffer.size()+20);
        }

        buffer.push_back(']');
    }
    else if(value.type == var::var_type::map) {
        auto map = value.as<var::var_type::map>();
        buffer.push_back('{');

        size_t index = 0;
        size_t count = map.size();

        for(const auto& pair : map)
        {
            if(pair.first.type != var::var_type::string)
            {
                throw std::runtime_error(std::format("error: cannot generate JSON entry with key of type '{}'", pair.first.type));
            }

            //TODO 
            //escape unsafe characters

            buffer.push_back('"');
            buffer += pair.first.as<var::var_type::string>();
            buffer.push_back('"');
            buffer.push_back(':');

             encode_var(pair.second, buffer);

            if(index < count-1)
            {
                buffer.push_back(',');
            }

            index++;
        }

        buffer.push_back('}');
    }
    else {
        buffer += value.to_s();
    }
}

std::string uva::json::enconde(const var& values)
{
    std::string buffer;
    buffer.reserve(512);
    //recurse loop in values
    encode_var(values, buffer);

    return buffer;
}

std::string_view next_non_white_space(std::string_view sv)
{
    while(sv.size() && isspace(sv[0]))
    {
        sv.remove_prefix(1);
    }

    return sv;
}

std::string_view extract_string(std::string_view sv)
{
    char quote = sv[0];
    sv.remove_prefix(1);

    size_t count = 0;
    while(count < sv.size()-1 && sv[count] != quote)
    {
        count++;
    }

    return sv.substr(0, count);
}

#define THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION() throw std::runtime_error(std::format("failed to parse json: unexpected token at {}", text_view.data() - text.data()))
#define THROW_UNEXPECTED_END_OF_INPUT() \
if(!text_view.size()) {\
    throw std::runtime_error(std::format("failed to parse json: unexpected end of input"));\
}

var uva::json::decode(const std::string& text)
{
    std::map<var, var> map;

    if(text.empty()) {
        return map;
    }
    
    std::string_view text_view = next_non_white_space(text);

    if(!text_view.starts_with('{')) {
        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    text_view.remove_prefix(1);
    text_view = next_non_white_space(text_view);

    while(text_view.size())
    {
        THROW_UNEXPECTED_END_OF_INPUT();

        if(!text_view.starts_with('\'') && !text_view.starts_with('\"')) {
            THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
        }

        std::string_view key = extract_string(text_view);
        
        text_view = text_view.substr(key.size()+2/* 2 quotes characters*/);
        text_view = next_non_white_space(text_view);

        THROW_UNEXPECTED_END_OF_INPUT();

        if(!text_view.starts_with(':')) {
            THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
        }

        text_view.remove_prefix(1);
        text_view = next_non_white_space(text_view);

        THROW_UNEXPECTED_END_OF_INPUT();

        if(text_view[0] == '\'' || text_view[0] == '"') {
            std::string_view value = extract_string(text_view);
            map.insert({std::string(key), std::string(value)});

            text_view = text_view.substr(value.size()+2/* 2 quotes characters*/);
        } else
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
                        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
                    }
                }

                str.push_back(text_view.front());
                text_view.remove_prefix(1);
            }

            if(is_double) {
                double d = std::stod(str);
                map.insert({std::string(key), is_negative ? (d * -1) : d});
            } else {
                size_t i = std::stol(str);
                map.insert({std::string(key), is_negative ? (i * -1) : i});
            }
        }

        text_view = next_non_white_space(text_view);

        if(text_view.starts_with('}')) {
            break;
        }

        if(text_view.starts_with(',')) {
            text_view.remove_prefix(1);
            text_view = next_non_white_space(text_view);
            continue;
        }

        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    }

    text_view.remove_prefix(1);
    text_view = next_non_white_space(text_view);

    if(text_view.size()) {
        THROW_UNEXPECTED_TOKEN_AT_THIS_LOCATION();
    } 

    return map;
}