#pragma once

#include <core.hpp>

namespace uva
{   
    class json
    {
    private:
        var m_values;
    public:
        json(std::map<var, var>&& values);
    public:
        std::string enconde() const;
    public:
        static std::string enconde(const var& values);
        static var decode(const std::string& text);
    };
    
}; // namespace uva
