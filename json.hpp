#pragma once

#include <core.hpp>

namespace uva
{   
    namespace json
    {
        std::string enconde(const var& values);
        var decode(const std::string& text);
    }; // namespace json
    
}; // namespace uva
