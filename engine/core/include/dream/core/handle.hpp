#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <type_traits>
#include <vector>

namespace dream::core {

// Tag types make handles from different resource domains non-interchangeable.
template <typename Tag>
class Handle final {
public:
    using value_type = std::uint32_t;
    static constexpr value_type invalid_index = std::numeric_limits<value_type>::max();

    constexpr Handle() noexcept = default;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return index_ != invalid_index && generation_ != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return is_valid();
    }

    [[nodiscard]] constexpr value_type index() const noexcept { return index_; }
    [[nodiscard]] constexpr value_type generation() const noexcept { return generation_; }

    [[nodiscard]] constexpr bool operator==(const Handle&) const noexcept = default;

private:
    template <typename>
    friend class HandlePool;

    constexpr Handle(value_type index, value_type generation) noexcept
        : index_(index), generation_(generation) {}

    value_type index_{invalid_index};
    value_type generation_{0};
};

// Owns handle identity only; resource storage and destruction remain with the subsystem.
template <typename Tag>
class HandlePool final {
public:
    using handle_type = Handle<Tag>;

    HandlePool() = default;
    HandlePool(const HandlePool&) = delete;
    HandlePool& operator=(const HandlePool&) = delete;

    [[nodiscard]] handle_type allocate() {
        std::lock_guard lock(mutex_);

        if (!free_indices_.empty()) {
            const auto index = free_indices_.back();
            free_indices_.pop_back();
            auto& slot = slots_[index];
            slot.active = true;
            return handle_type{index, slot.generation};
        }

        if (slots_.size() >= handle_type::invalid_index) {
            return {};
        }

        const auto index = static_cast<std::uint32_t>(slots_.size());
        slots_.push_back(Slot{});
        slots_.back().active = true;
        return handle_type{index, slots_.back().generation};
    }

    // Returns false for invalid, stale or already released handles.
    [[nodiscard]] bool release(handle_type handle) {
        if (!handle) return false;

        std::lock_guard lock(mutex_);
        if (handle.index_ >= slots_.size()) return false;

        auto& slot = slots_[handle.index_];
        if (!slot.active || slot.generation != handle.generation_) return false;

        // Push first: if allocation throws, the live slot remains unchanged.
        free_indices_.push_back(handle.index_);
        slot.active = false;
        ++slot.generation_;
        if (slot.generation_ == 0) ++slot.generation_;
        return true;
    }

    [[nodiscard]] bool is_alive(handle_type handle) const {
        if (!handle) return false;

        std::lock_guard lock(mutex_);
        return handle.index_ < slots_.size()
            && slots_[handle.index_].active
            && slots_[handle.index_].generation == handle.generation_;
    }

    [[nodiscard]] std::size_t capacity() const {
        std::lock_guard lock(mutex_);
        return slots_.size();
    }

private:
    struct Slot final {
        std::uint32_t generation{1};
        bool active{false};
    };

    mutable std::mutex mutex_;
    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_indices_;
};

} // namespace dream::core
