#pragma once

#include <core.hpp>

namespace uva
{   
    namespace json
    {
        namespace web_token
        {
            std::string enconde(const std::map<var, var>& values, const std::string& secret);
            bool decode(const std::string& text, const std::string& secret, var& output);
        };
    }; // namespace json
    
}; // namespace uva