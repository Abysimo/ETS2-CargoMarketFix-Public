#include "hook_lifecycle.h"

namespace cmf::lifecycle {
namespace {
void change(PatchMemory& memory, Bytes replacement, PatchResult& result) noexcept {
    if (!memory.writable()) {
        result.reason = "write_protection_failed";
        result.protection_restored = memory.restore_protection();
        result.after = memory.classify();
        return;
    }
    result.write_attempted = true;
    const bool written = memory.write(replacement);
    result.cache_synchronized = memory.flush();
    result.protection_restored = memory.restore_protection();
    result.after = memory.classify();
    result.committed = written && result.after == replacement &&
                       result.cache_synchronized && result.protection_restored;
    result.reason = result.committed ? "committed" :
        (!written || result.after != replacement) ? "write_verification_failed" :
        !result.cache_synchronized ? "instruction_cache_flush_failed" :
        "page_protection_restore_failed";
}
}  // namespace

PatchResult transact(PatchMemory& memory, bool install) noexcept {
    PatchResult result;
    result.before = result.after = memory.classify();
    if (result.before == Bytes::unknown ||
        (install && result.before != Bytes::original)) {
        result.protection_restored = false; // Not inspected/repaired on refusal.
        result.reason = "target_ownership_unknown_or_unexpected";
        return result;  // Never overwrite unknown third-party changes.
    }
    if (!install && result.before == Bytes::original) {
        // Retrying after an uncertain flush/protection failure still needs a
        // cache synchronization proof, not just memcmp of the data view.
        result.cache_synchronized = memory.flush();
        result.protection_restored = memory.restore_protection();
        result.after = memory.classify();
        result.committed = result.after == Bytes::original &&
                           result.cache_synchronized && result.protection_restored;
        result.reason = result.committed ? "already_original_verified" :
            !result.cache_synchronized ? "instruction_cache_flush_failed" :
            "original_bytes_verified_with_auxiliary_failure";
        return result;
    }
    change(memory, install ? Bytes::detour : Bytes::original, result);
    if (install && !result.write_attempted && result.after == Bytes::original) {
        // This transaction never exposed a detour; the original instruction
        // stream was not modified. No new cache invalidation is needed.
        result.cache_synchronized = true;
    }
    if (install && !result.committed && result.after == Bytes::detour) {
        PatchResult rollback;
        rollback.before = Bytes::detour;
        rollback.after = Bytes::detour;
        change(memory, Bytes::original, rollback);
        result.rollback_attempted = true;
        result.after = rollback.after;
        result.rollback_original_verified = rollback.after == Bytes::original;
        result.cache_synchronized = rollback.cache_synchronized;
        result.protection_restored = rollback.protection_restored;
        result.reason = result.rollback_original_verified && result.cache_synchronized
            ? "installation_failed_original_restored" : "rollback_unconfirmed";
        // The original installation failed even if its rollback succeeded.
        result.committed = false;
    }
    return result;
}

const char* name(Phase phase) noexcept {
    switch (phase) {
    case Phase::empty: return "empty";
    case Phase::prepared: return "prepared";
    case Phase::installing: return "installing";
    case Phase::installed: return "installed";
    case Phase::restoring: return "restoring";
    case Phase::detached_original: return "detached_original";
    case Phase::containment: return "containment";
    case Phase::releasable: return "releasable";
    case Phase::detached_resources_retained: return "detached_resources_retained";
    case Phase::retained_until_exit: return "retained_until_exit";
    }
    return "invalid";
}
const char* name(Bytes bytes) noexcept {
    switch (bytes) {
    case Bytes::original: return "original";
    case Bytes::detour: return "detour";
    case Bytes::unknown: return "unknown";
    }
    return "unknown";
}

bool Controller::cleanup(Operations& ops) noexcept {
    // Unexposed preparation resources are also safe: no target has ever been
    // allowed to reference them. For exposed resources, detach() is mandatory.
    if (may_be_live_ || published_) {
        reason_ = "cleanup_refused_live_or_published";
        return false;
    }
    if (ops.has_unwind() && !ops.unregister_unwind()) {
        phase_ = Phase::detached_resources_retained;
        reason_ = "unwind_cleanup_failed_backing_allocation_retained";
        return false;
    }
    if (ops.has_allocation() && !ops.free_allocation()) {
        phase_ = Phase::detached_resources_retained;
        reason_ = "allocation_cleanup_failed_detached";
        return false;
    }
    phase_ = Phase::releasable;
    return true;
}

void Controller::detach(Operations& ops) noexcept {
    // Called ONLY while frozen, after exact-original + cache + active proof.
    original_verified_ = true;
    may_be_live_ = false;
    phase_ = Phase::detached_original;
    if (published_) {
        ops.clear_continuation();
        published_ = false;
    }
}

bool Controller::install(bool requested, Operations& ops) noexcept {
    if (phase_ != Phase::empty) {
        reason_ = "additional_installation_refused_existing_state";
        return false;
    }
    if (!requested) {
        reason_ = "disabled_no_preparation_scan_pin_or_patch";
        return false;
    }
    phase_ = Phase::prepared;
    if (!ops.prepare()) {
        reason_ = "preparation_failed_never_exposed";
        cleanup(ops);
        return false;
    }
    if (!ops.pin_module()) {
        reason_ = "module_pin_failed_no_target_write";
        cleanup(ops);
        return false;
    }
    pinned_ = true;
    ops.publish();
    published_ = true;
    if (!ops.freeze()) {
        // No patch was attempted, and these new resources were never exposed.
        ops.clear_continuation();
        published_ = false;
        reason_ = "prepatch_quiescence_failed_never_exposed";
        cleanup(ops);
        return false;
    }
    phase_ = Phase::installing;
    may_be_live_ = true; // BEFORE the first API can change target bytes.
    last_patch_ = ops.patch(true);
    if (last_patch_.committed && last_patch_.after == Bytes::detour) {
        phase_ = Phase::installed;
        ops.recording(true);
        reason_ = "installed_verified";
        ops.thaw();
        return true;
    }
    ops.recording(false);
    reason_ = last_patch_.reason;
    if (last_patch_.after == Bytes::original &&
        last_patch_.cache_synchronized && ops.no_active_calls()) {
        detach(ops);
    } else {
        phase_ = Phase::containment;
    }
    ops.thaw();
    if (!may_be_live_) cleanup(ops);
    return false;
}

bool Controller::stop(Operations& ops) noexcept {
    if (phase_ == Phase::empty) return true;
    if (phase_ == Phase::releasable) {
        if (!last_patch_.protection_restored) {
            // Resources are already safely detached. Retry only the separate
            // original-page/cache repair; never republish a continuation.
            last_patch_ = ops.patch(false);
            reason_ = last_patch_.reason;
            return last_patch_.committed;
        }
        return true;
    }
    if (phase_ == Phase::retained_until_exit) return false;
    ops.recording(false); // Forwarding remains published in containment.
    if (!may_be_live_ && !published_) {
        if (!cleanup(ops)) return false;
        // A prior unwind/free failure must not hide a still-pending page
        // protection failure when cleanup eventually succeeds.
        return last_patch_.protection_restored ? true : stop(ops);
    }
    phase_ = Phase::restoring;
    if (!ops.freeze()) {
        phase_ = Phase::containment;
        reason_ = "restoration_quiescence_failed";
        return false;
    }
    if (!ops.no_active_calls()) {
        phase_ = Phase::containment;
        reason_ = "active_calls_remain";
        ops.thaw();
        return false;
    }
    last_patch_ = ops.patch(false);
    reason_ = last_patch_.reason;
    if (last_patch_.after == Bytes::original &&
        last_patch_.cache_synchronized && ops.no_active_calls()) {
        detach(ops);
    } else {
        phase_ = Phase::containment;
    }
    ops.thaw();
    if (may_be_live_) return false;
    const bool reclaimed = cleanup(ops);
    // Original bytes and resource reclamation can be safe even when a page
    // protection error makes this stop unsuccessful as an overall operation.
    return reclaimed && last_patch_.protection_restored;
}

bool Controller::release_allowed() const noexcept {
    return !may_be_live_ && !published_ &&
           (phase_ == Phase::empty || phase_ == Phase::releasable);
}
void Controller::abandon_until_exit() noexcept {
    phase_ = Phase::retained_until_exit;
}
}  // namespace cmf::lifecycle
