// MemForge - op stream validator implementation.

#include <memforge/op_validator.hpp>

namespace memforge {

Status OpStreamValidator::validate_one(const Op& op) {
    switch (op.kind) {
    case OpKind::Noop:
        return Ok();

    case OpKind::ArenaAppendBlock:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0) return Status{ErrorCode::InvalidArgument};
        if (op.c == 0 || (op.c & (op.c - 1)) != 0) {
            return Status{ErrorCode::InvalidArgument};
        }
        return Ok();

    case OpKind::PoolAlloc:
    case OpKind::PoolFree:
    case OpKind::PoolCompact:
        if (op.a >= 6) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::HandleInsert:
    case OpKind::HandleResolve:
        if (op.a == 0 || op.a >= 4096) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::EpochEnqueue:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0 || op.b > 65536) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::EpochTick:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::EpochDrain:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::TxBegin:
    case OpKind::TxCommit:
    case OpKind::TxRollback:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::TxRecord:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::MigratePlan:
        if (op.a >= 6 || op.b >= 6) return Status{ErrorCode::OutOfRange};
        if (op.c == 0 || op.c > 4096) return Status{ErrorCode::InvalidArgument};
        if (op.d == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::MigrateCommit:
        if (op.a >= 6 || op.b >= 6) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::StatsSample:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b >= 16) return Status{ErrorCode::OutOfRange};
        if (op.c == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::StatsFold:
        if (op.a >= 16) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::CheckpointBegin:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0) return Status{ErrorCode::InvalidArgument};
        if (op.c == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::CheckpointWriteHandleTable:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0) return Status{ErrorCode::InvalidArgument};
        if (op.c == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::Pin:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b > 4096) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::Unpin:
    case OpKind::PinReleaseAll:
    case OpKind::DeferCompact:
    case OpKind::DeferFlush:
    case OpKind::HubDestroy:
        return Ok();

    case OpKind::DeferPush:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0 || op.b > 65536) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::ShardRebalance:
        if (op.a >= 4 || op.b >= 4) return Status{ErrorCode::OutOfRange};
        if (op.a == op.b) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::SpanGuardAttach:
        if (op.a == 0 || op.a >= 256) return Status{ErrorCode::OutOfRange};
        if (op.b >= 8) return Status{ErrorCode::OutOfRange};
        if (op.c > 64) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::SpanGuardVerify:
        if (op.a == 0 || op.a >= 256) return Status{ErrorCode::OutOfRange};
        return Ok();

    case OpKind::BatchStage:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        if (op.b == 0) return Status{ErrorCode::InvalidArgument};
        if (op.c == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::BatchCommit:
        if (op.a == 0) return Status{ErrorCode::InvalidArgument};
        return Ok();

    case OpKind::HubEvict:
        if (op.a == 0 || op.a > 8) return Status{ErrorCode::OutOfRange};
        if (op.b > 1) return Status{ErrorCode::InvalidArgument};
        return Ok();
    }
    return Status{ErrorCode::Unknown};
}

ValidationReport OpStreamValidator::validate(const OpStream& ops) {
    ValidationReport report{};
    report.total = static_cast<std::uint32_t>(ops.size());
    for (std::size_t i = 0; i < ops.size(); ++i) {
        Status s = validate_one(ops[i]);
        if (s.ok()) {
            ++report.valid;
        } else {
            if (report.invalid == 0) {
                report.first_error = s;
                report.first_error_index = i;
            }
            ++report.invalid;
        }
    }
    return report;
}

}  // namespace memforge
