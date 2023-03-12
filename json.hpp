#pragma once

#include <core.hpp>

namespace uva
{   
    namespace json
    {
        std::string enconde(const var& values, bool pretty = false);
        var decode(const std::string& text);
    }; // namespace json
    
}; // namespace uva
