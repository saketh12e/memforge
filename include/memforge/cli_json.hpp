// MemForge - JSON output helper for the CLI.

#ifndef MEMFORGE_CLI_JSON_HPP
#define MEMFORGE_CLI_JSON_HPP

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace memforge {

class JsonWriter {
public:
    explicit JsonWriter(FILE* out) : out_(out) {}
    ~JsonWriter() = default;

    void begin_object() { put('{'); indent_++; first_ = true; }
    void end_object() { indent_--; put_line('}'); first_ = false; }
    void begin_array() { put('['); indent_++; first_ = true; }
    void end_array() { indent_--; put_line(']'); first_ = false; }

    void field(const std::string& key, std::uint64_t value) {
        comma();
        std::fprintf(out_, "\"%s\": %llu", key.c_str(),
                     static_cast<unsigned long long>(value));
    }
    void field(const std::string& key, std::int64_t value) {
        comma();
        std::fprintf(out_, "\"%s\": %lld", key.c_str(),
                     static_cast<long long>(value));
    }
    void field(const std::string& key, double value) {
        comma();
        std::fprintf(out_, "\"%s\": %g", key.c_str(), value);
    }
    void field(const std::string& key, const std::string& value) {
        comma();
        std::fprintf(out_, "\"%s\": \"", key.c_str());
        for (char c : value) {
            if (c == '"') std::fputc('\\', out_);
            std::fputc(c, out_);
        }
        std::fputc('"', out_);
    }

private:
    void comma() {
        if (!first_) std::fputc(',', out_);
        std::fputc('\n', out_);
        for (int i = 0; i < indent_; ++i) std::fputc(' ', out_);
        first_ = false;
    }
    void put(char c) { std::fputc(c, out_); }
    void put_line(char c) {
        std::fputc('\n', out_);
        for (int i = 0; i < indent_; ++i) std::fputc(' ', out_);
        std::fputc(c, out_);
    }

    FILE* out_;
    int indent_{0};
    bool first_{true};
};

}  // namespace memforge

#endif  // MEMFORGE_CLI_JSON_HPP
