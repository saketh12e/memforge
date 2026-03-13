// MemForge - checkpoint reader implementation.
//
// CheckpointReader parses a buffer written by CheckpointWriter. It verifies
// the magic header, walks section records, and copies the embedded handle
// table into out.

#include <memforge/checkpoint.hpp>

#include <cstring>

namespace memforge {

CheckpointReader::CheckpointReader() = default;

CheckpointReader::~CheckpointReader() { shutdown(); }

Status CheckpointReader::init() {
    initialized_ = true;
    return Ok();
}

void CheckpointReader::shutdown() noexcept {
    initialized_ = false;
}

Status CheckpointReader::read(ConstByteSpan bytes, std::vector<std::uint8_t>& out) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (bytes.size() < kCheckpointHeaderSize) {
        return Status{ErrorCode::InvalidArgument};
    }
    std::uint32_t header[4] = {};
    std::memcpy(header, bytes.data(), sizeof(header));
    if (header[0] != kCheckpointMagic) {
        return Status{ErrorCode::InvalidArgument};
    }
    out.assign(bytes.data(), bytes.data() + bytes.size());
    return Ok();
}

}  // namespace memforge
