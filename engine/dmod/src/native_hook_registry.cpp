#include <dream/dmod/native_hook_registry.hpp>

#include <algorithm>
#include <limits>
#include <mutex>
#include <utility>

namespace dream::dmod {

bool NativeHookRegistry::valid_identifier(std::string_view value) noexcept {
    if (value.empty() || value.size() > 128) return false;

    bool segment_start = true;
    for (const unsigned char character : value) {
        if (character == '.') {
            if (segment_start) return false;
            segment_start = true;
            continue;
        }
        if (!((character >= 'a' && character <= 'z') ||
              (character >= '0' && character <= '9') ||
              character == '_' || character == '-')) {
            return false;
        }
        segment_start = false;
    }
    return !segment_start;
}

NativeHookRegistrationId NativeHookRegistry::register_hook(
    std::string_view owner_mod_id,
    std::string_view hook_point_id,
    NativeHookStage stage,
    std::int32_t priority,
    NativeHookCallbackV1 callback,
    void* callback_context) noexcept {
    if (!valid_identifier(owner_mod_id) || !valid_identifier(hook_point_id) ||
        callback == nullptr ||
        (stage != NativeHookStage::Before && stage != NativeHookStage::Around &&
         stage != NativeHookStage::Replace && stage != NativeHookStage::After)) {
        return 0;
    }

    try {
        std::string owner(owner_mod_id);
        std::string point(hook_point_id);
        std::unique_lock lock(mutex_);
        if (next_registration_ == 0 ||
            next_registration_ == std::numeric_limits<NativeHookRegistrationId>::max()) {
            return 0;
        }

        const auto id = next_registration_++;
        entries_.push_back(Entry{
            id, std::move(owner), std::move(point), stage, priority,
            callback, callback_context
        });
        return id;
    } catch (...) {
        return 0;
    }
}

bool NativeHookRegistry::remove_hook(NativeHookRegistrationId registration) noexcept {
    if (registration == 0) return false;
    try {
        std::unique_lock lock(mutex_);
        const auto it = std::find_if(entries_.begin(), entries_.end(),
            [registration](const Entry& entry) {
                return entry.registration == registration;
            });
        if (it == entries_.end()) return false;
        entries_.erase(it);
        return true;
    } catch (...) {
        return false;
    }
}

std::size_t NativeHookRegistry::registered_count() const noexcept {
    try {
        std::shared_lock lock(mutex_);
        return entries_.size();
    } catch (...) {
        return 0;
    }
}

bool NativeHookRegistry::set_owner_enabled(
    std::string_view owner, bool enabled) noexcept {
    if (!valid_identifier(owner)) return false;
    try {
        std::string key(owner);
        std::unique_lock lock(mutex_);
        const auto it = owner_enabled_.find(key);
        if (it != owner_enabled_.end()) {
            it->second = enabled;
            return true;
        }
        if (!enabled) return true;
        owner_enabled_.emplace(std::move(key), true);
        return true;
    } catch (...) {
        return false;
    }
}

std::size_t NativeHookRegistry::remove_owner(std::string_view owner) noexcept {
    if (!valid_identifier(owner)) return 0;
    try {
        std::unique_lock lock(mutex_);
        std::size_t removed = 0;
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->owner_mod_id == owner) {
                it = entries_.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        owner_enabled_.erase(std::string(owner));
        return removed;
    } catch (...) {
        return 0;
    }
}

std::uint8_t NativeHookRegistry::call_next(void* context) noexcept {
    if (context == nullptr) return 0;
    auto& next = *static_cast<NextState*>(context);
    if (next.chain == nullptr) return 0;
    invoke_chain(*next.chain, next.next_index);
    return 1;
}

void NativeHookRegistry::invoke_chain(ChainState& state, std::size_t index) noexcept {
    if (state.entries == nullptr) return;
    if (index >= state.entries->size()) {
        if (state.original != nullptr) {
            state.original(state.user_data);
            state.original_called = true;
        }
        return;
    }

    const auto& entry = (*state.entries)[index];
    NextState next{&state, index + 1U};
    NativeHookInvocationV1 invocation{
        state.user_data, &next, &NativeHookRegistry::call_next
    };
    entry.callback(entry.callback_context, &invocation);
    ++state.callbacks_invoked;
}

NativeHookDispatchResult NativeHookRegistry::dispatch(
    std::string_view hook_point_id,
    void* user_data,
    NativeHookOriginalV1 original) noexcept {
    if (!valid_identifier(hook_point_id) || original == nullptr) return {};

    std::vector<Entry> snapshot;
    try {
        {
            std::shared_lock lock(mutex_);
            snapshot.reserve(entries_.size());
            for (const auto& entry : entries_) {
                if (entry.hook_point_id != hook_point_id) continue;
                const auto owner = owner_enabled_.find(entry.owner_mod_id);
                if (owner != owner_enabled_.end() && !owner->second) continue;
                snapshot.push_back(entry);
            }
        }
        std::sort(snapshot.begin(), snapshot.end(),
            [](const Entry& left, const Entry& right) {
                if (left.priority != right.priority)
                    return left.priority > right.priority;
                return left.registration < right.registration;
            });
    } catch (...) {
        return {};
    }

    std::vector<Entry> chain;
    try {
        chain.reserve(snapshot.size());
        for (const auto& entry : snapshot) {
            if (entry.stage == NativeHookStage::Around ||
                entry.stage == NativeHookStage::Replace) {
                chain.push_back(entry);
            }
        }
    } catch (...) {
        return {};
    }

    NativeHookDispatchResult result;
    result.success = true;

    for (const auto& entry : snapshot) {
        if (entry.stage != NativeHookStage::Before) continue;
        NativeHookInvocationV1 invocation{user_data, nullptr, nullptr};
        entry.callback(entry.callback_context, &invocation);
        ++result.callbacks_invoked;
    }

    ChainState state{&chain, original, user_data, false, 0};
    invoke_chain(state, 0);
    result.original_called = state.original_called;
    result.callbacks_invoked += state.callbacks_invoked;

    // After callbacks unwind in reverse priority/registration order.
    for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it) {
        if (it->stage != NativeHookStage::After) continue;
        NativeHookInvocationV1 invocation{user_data, nullptr, nullptr};
        it->callback(it->callback_context, &invocation);
        ++result.callbacks_invoked;
    }

    return result;
}

} // namespace dream::dmod
