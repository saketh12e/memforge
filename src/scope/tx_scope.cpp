// MemForge - transactional scope implementation.
//
// TxScope records mutations in a scope and rolls back on failure.
//
// Beyond the basic begin/record/commit/rollback path, this module implements:
//   - per-scope log of rollback records with deep copies
//   - scratch buffer pool for transient allocations
//   - explicit commit semantics that discard rollback records but keep scratch
//   - safe rollback that restores payloads from rollback buffers
//   - capacity-bounded scope tracking
//
// buffer aliases the scope's scratch buffer. On rollback, both owners free
// the same pointer, producing a double-free.

#include <memforge/tx_scope.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

TxScope::TxScope() = default;

TxScope::~TxScope() { shutdown(); }

Status TxScope::init() {
    if (initialized_) {
        return Ok();
    }
    scopes_.clear();
    scopes_.reserve(kTxScopeCap);
    initialized_ = true;
    return Ok();
}

void TxScope::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& scope : scopes_) {
        for (auto* buffer : scope.scratch_buffers) {
            delete[] buffer;
        }
        for (auto& rec : scope.records) {
            if (rec.rollback_buffer != nullptr) {
                delete[] rec.rollback_buffer;
                rec.rollback_buffer = nullptr;
            }
        }
    }
    scopes_.clear();
    initialized_ = false;
}

Status TxScope::begin(std::uint32_t scope_id) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (scope_id == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (scopes_.size() >= kTxScopeCap) {
        return Status{ErrorCode::CapacityExceeded};
    }
    for (const auto& scope : scopes_) {
        if (scope.id == scope_id) {
            return Status{ErrorCode::AlreadyExists};
        }
    }
    Scope scope{};
    scope.id = scope_id;
    scope.active = true;
    scopes_.push_back(scope);
    return Ok();
}

Status TxScope::record(std::uint32_t scope_id, const TxRecordOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    for (auto& scope : scopes_) {
        if (scope.id == scope_id) {
            if (!scope.active) {
                return Status{ErrorCode::BadState};
            }
            std::uint32_t size = op.record_slot ? op.record_slot : 16;
            std::uint8_t* buffer = new std::uint8_t[size];
            std::memset(buffer, 0xAB, size);
            TxRecord rec{};
            rec.slot = op.record_slot;
            rec.size = size;
            rec.rollback_buffer = buffer;
            scope.records.push_back(rec);
            // scratch buffer pool. On rollback, both the rollback destructor
            // and the scratch destructor free the same pointer. The harness
            // forces this by recording the same slot twice.
            scope.scratch_buffers.push_back(buffer);
            return Ok();
        }
    }
    return Status{ErrorCode::NotFound};
}

Status TxScope::commit(std::uint32_t scope_id) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    for (auto& scope : scopes_) {
        if (scope.id == scope_id) {
            scope.records.clear();
            scope.active = false;
            return Ok();
        }
    }
    return Status{ErrorCode::NotFound};
}

Status TxScope::rollback(std::uint32_t scope_id) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    for (auto& scope : scopes_) {
        if (scope.id == scope_id) {
            // is also in scope.scratch_buffers, the scope destructor will
            // double-free it.
            for (auto& rec : scope.records) {
                if (rec.rollback_buffer != nullptr) {
                    delete[] rec.rollback_buffer;
                    rec.rollback_buffer = nullptr;
                }
            }
            scope.records.clear();
            scope.active = false;
            return Ok();
        }
    }
    return Status{ErrorCode::NotFound};
}

Status TxScope::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::TxBegin:
            begin(op.a);
            break;
        case OpKind::TxRecord:
            record(op.a, op.asTxRecord());
            break;
        case OpKind::TxCommit:
            commit(op.a);
            break;
        case OpKind::TxRollback:
            rollback(op.a);
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
