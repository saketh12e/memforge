// MemForge - settings implementation.

#include <memforge/settings.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

void Settings::set(const std::string& key, const std::string& value) {
    values_[key] = value;
}

std::string Settings::get(const std::string& key, const std::string& default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    return it->second;
}

bool Settings::get_bool(const std::string& key, bool default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    const std::string& v = it->second;
    if (v == "1" || v == "true" || v == "TRUE" || v == "yes" || v == "YES") {
        return true;
    }
    if (v == "0" || v == "false" || v == "FALSE" || v == "no" || v == "NO") {
        return false;
    }
    return default_value;
}

std::uint32_t Settings::get_u32(const std::string& key, std::uint32_t default_value) const {
    auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    char* end = nullptr;
    unsigned long v = std::strtoul(it->second.c_str(), &end, 10);
    if (end == it->second.c_str()) {
        return default_value;
    }
    return static_cast<std::uint32_t>(v);
}

extern "C" char** environ;

void Settings::load_from_env(const char* prefix) {
    if (prefix == nullptr) {
        prefix = "MEMFORGE_";
    }
    std::size_t prefix_len = std::strlen(prefix);
    for (char** env = environ; *env != nullptr; ++env) {
        const char* s = *env;
        if (std::strncmp(s, prefix, prefix_len) != 0) {
            continue;
        }
        const char* eq = std::strchr(s, '=');
        if (eq == nullptr) {
            continue;
        }
        std::string key(s + prefix_len, eq - s - prefix_len);
        std::string value(eq + 1);
        values_[key] = value;
    }
}

}  // namespace memforge
