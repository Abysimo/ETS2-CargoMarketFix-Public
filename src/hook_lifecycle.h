#pragma once

// No Windows dependencies or test failpoints. This controller and transaction
// algorithm are compiled unchanged into the DLL and the deterministic tests.
namespace cmf::lifecycle {

enum class Phase {
    empty, prepared, installing, installed, restoring, detached_original,
    containment, releasable, detached_resources_retained, retained_until_exit
};
enum class Bytes { original, detour, unknown };

struct PatchResult {
    Bytes before = Bytes::unknown;
    Bytes after = Bytes::unknown;
    bool committed = false;
    bool write_attempted = false;
    bool rollback_attempted = false;
    bool rollback_original_verified = false;
    bool cache_synchronized = false;
    bool protection_restored = true;
    const char* reason = "not_attempted";
};

class PatchMemory {
public:
    virtual ~PatchMemory() = default;
    virtual Bytes classify() noexcept = 0;
    virtual bool writable() noexcept = 0;
    virtual bool write(Bytes bytes) noexcept = 0;
    virtual bool flush() noexcept = 0;
    virtual bool restore_protection() noexcept = 0;
};

PatchResult transact(PatchMemory& memory, bool install) noexcept;
// After detach, auxiliary repair never opens a write transaction. Any byte
// change requires a full transaction under freshly established quiescence.
PatchResult repair_original_auxiliaries(PatchMemory& memory) noexcept;
const char* name(Phase phase) noexcept;
const char* name(Bytes bytes) noexcept;

class Operations {
public:
    virtual ~Operations() = default;
    virtual bool prepare() noexcept = 0;
    virtual bool pin_module() noexcept = 0;
    virtual void publish() noexcept = 0;
    virtual void recording(bool enabled) noexcept = 0;
    virtual bool freeze() noexcept = 0;
    virtual void thaw() noexcept = 0;
    virtual bool no_active_calls() noexcept = 0;
    virtual PatchResult patch(bool install) noexcept = 0;
    virtual PatchResult repair_original_auxiliaries() noexcept = 0;
    virtual void clear_continuation() noexcept = 0;
    virtual bool has_unwind() const noexcept = 0;
    virtual bool has_allocation() const noexcept = 0;
    virtual bool unregister_unwind() noexcept = 0;
    virtual bool free_allocation() noexcept = 0;
};

class Controller final {
public:
    bool install(bool requested, Operations& ops) noexcept;
    bool stop(Operations& ops) noexcept;
    // No resource operation is performed by release/destructor policy.
    bool release_allowed() const noexcept;
    void abandon_until_exit() noexcept;

    Phase phase() const noexcept { return phase_; }
    bool module_pinned() const noexcept { return pinned_; }
    bool may_be_live() const noexcept { return may_be_live_; }
    bool original_verified() const noexcept { return original_verified_; }
    bool published() const noexcept { return published_; }
    const PatchResult& last_patch() const noexcept { return last_patch_; }
    const char* reason() const noexcept { return reason_; }

private:
    bool cleanup(Operations& ops) noexcept;
    void detach(Operations& ops) noexcept;
    Phase phase_ = Phase::empty;
    bool pinned_ = false;
    bool may_be_live_ = false;
    bool original_verified_ = false;
    bool auxiliary_pending_ = false;
    bool published_ = false;
    PatchResult last_patch_{};
    const char* reason_ = "empty";
};
}  // namespace cmf::lifecycle
