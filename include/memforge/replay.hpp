// MemForge - production replay API entry point.
//
// The replay layer exposes a thin set of public workflow APIs that integrate
// the underlying ResourceHub and its modules in real production-shaped call
// sequences. Each replay class is paired with a single fuzzer target and
// drives a distinct subset of production modules.

#ifndef MEMFORGE_REPLAY_HPP
#define MEMFORGE_REPLAY_HPP

#include <memforge/api.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/resource_hub.hpp>

namespace memforge {

// Decode an op-stream byte buffer into a typed OpStream and validate it.
// Returns false (and leaves `out` empty) if the input is too short or
// contains an op with out-of-range operands.
bool decodeAndValidate(const std::uint8_t* data, std::size_t size,
                       OpStream& out);

// Apply a decoded op stream to a ResourceHub. Thin adapter around
// hub.applyOps that exists so replay classes can call into the public
// surface rather than touching hub internals directly.
Status applyToHub(ResourceHub& hub, const OpStream& ops);

}  // namespace memforge

#endif  // MEMFORGE_REPLAY_HPP