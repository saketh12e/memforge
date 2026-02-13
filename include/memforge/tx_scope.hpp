// MemForge - transactional mutation scope.

#ifndef MEMFORGE_TX_SCOPE_HPP
#define MEMFORGE_TX_SCOPE_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kTxScopeCap = 1024;

struct TxRecord {
    std::uint32_t slot{0};
    std::uint8_t* rollback_buffer{nullptr};
    std::uint32_t size{0};
};

class TxScope {
public:
    TxScope();
    ~TxScope();

    TxScope(const TxScope&) = delete;
    TxScope& operator=(const TxScope&) = delete;

    Status init();
    void shutdown() noexcept;

    Status begin(std::uint32_t scope_id);
    Status record(std::uint32_t scope_id, const TxRecordOp& op);
    Status commit(std::uint32_t scope_id);
    Status rollback(std::uint32_t scope_id);

    std::size_t scopeCount() const noexcept { return scopes_.size(); }

    static std::uint32_t recordsPerScopeLimit() noexcept;

    Status apply(const OpStream& ops);

private:
    struct Scope {
        std::uint32_t id{0};
        std::vector<TxRecord> records;
        std::vector<std::uint8_t*> scratch_buffers;
        bool active{false};
    };

    std::vector<Scope> scopes_;
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_TX_SCOPE_HPP
