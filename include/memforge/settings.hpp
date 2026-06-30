// MemForge - settings and configuration loader.
//
// A simple key=value settings loader with environment variable overrides.

#ifndef MEMFORGE_SETTINGS_HPP
#define MEMFORGE_SETTINGS_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace memforge {

class Settings {
public:
    Settings() = default;
    ~Settings() = default;

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, const std::string& default_value = "") const;

    bool get_bool(const std::string& key, bool default_value = false) const;
    std::uint32_t get_u32(const std::string& key, std::uint32_t default_value = 0) const;

    void load_from_env(const char* prefix);

    std::size_t size() const noexcept { return values_.size(); }
    void clear() noexcept { values_.clear(); }

private:
    std::unordered_map<std::string, std::string> values_;
};

}  // namespace memforge

#endif  // MEMFORGE_SETTINGS_HPP
