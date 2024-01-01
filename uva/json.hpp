#pragma once

#include <filesystem>

#include <uva/core.hpp>

namespace uva
{   
    namespace json
    {
        std::string enconde(const var& values, bool pretty = false);
        void encode(const var& values, const std::filesystem::path& path);

        var decode(const std::string& text);
        var decode(const std::filesystem::path& path);
    }; // namespace json
    
}; // namespace uva
