#pragma once

#include <dream/physics2d/geometry.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <shared_mutex>
#include <vector>

namespace dream::physics2d {

using ColliderProxyId = std::uint64_t;

enum class BroadphaseError : std::uint8_t {
    None,
    InvalidBounds
};

struct BroadphasePair final {
    ColliderProxyId first{0};
    ColliderProxyId second{0};

    friend constexpr bool operator==(
        const BroadphasePair&, const BroadphasePair&) noexcept = default;
};

struct BroadphaseResult final {
    std::vector<BroadphasePair> pairs;
    std::size_t candidate_tests{0};
};

// Deterministic sweep-and-prune broadphase. It reports AABB-overlapping pairs
// only; narrowphase collision tests must still be run for exact shapes.
// Upsert/remove are thread-safe. Each query holds a shared lock while taking
// its stable snapshot. Pair IDs are normalized and output is lexically sorted.
class SweepAndPrune final {
public:
    SweepAndPrune() = default;
    SweepAndPrune(const SweepAndPrune&) = delete;
    SweepAndPrune& operator=(const SweepAndPrune&) = delete;

    [[nodiscard]] BroadphaseError upsert(
        ColliderProxyId id, Aabb bounds) {
        if (!bounds.valid()) return BroadphaseError::InvalidBounds;
        std::unique_lock lock(mutex_);
        proxies_.insert_or_assign(id, bounds);
        return BroadphaseError::None;
    }

    [[nodiscard]] bool remove(ColliderProxyId id) {
        std::unique_lock lock(mutex_);
        return proxies_.erase(id) != 0U;
    }

    [[nodiscard]] bool contains(ColliderProxyId id) const {
        std::shared_lock lock(mutex_);
        return proxies_.contains(id);
    }

    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return proxies_.size();
    }

    void clear() {
        std::unique_lock lock(mutex_);
        proxies_.clear();
    }

    [[nodiscard]] BroadphaseResult compute_pairs() const {
        struct Proxy final {
            ColliderProxyId id;
            Aabb bounds;
        };

        std::shared_lock lock(mutex_);
        std::vector<Proxy> sorted;
        sorted.reserve(proxies_.size());
        for (const auto& [id, bounds] : proxies_)
            sorted.push_back({id, bounds});

        std::sort(sorted.begin(), sorted.end(),
            [](const Proxy& left, const Proxy& right) {
                if (left.bounds.minimum.x != right.bounds.minimum.x)
                    return left.bounds.minimum.x < right.bounds.minimum.x;
                return left.id < right.id;
            });

        BroadphaseResult result;
        std::vector<std::size_t> active;
        active.reserve(sorted.size());

        for (std::size_t current = 0; current < sorted.size(); ++current) {
            const auto& candidate = sorted[current];
            active.erase(
                std::remove_if(active.begin(), active.end(),
                    [&](std::size_t index) {
                        return sorted[index].bounds.maximum.x <
                               candidate.bounds.minimum.x;
                    }),
                active.end());

            for (const std::size_t index : active) {
                ++result.candidate_tests;
                const auto& other = sorted[index];
                if (other.bounds.minimum.y > candidate.bounds.maximum.y ||
                    other.bounds.maximum.y < candidate.bounds.minimum.y) {
                    continue;
                }

                const auto first = std::min(other.id, candidate.id);
                const auto second = std::max(other.id, candidate.id);
                result.pairs.push_back({first, second});
            }
            active.push_back(current);
        }

        std::sort(result.pairs.begin(), result.pairs.end(),
            [](const BroadphasePair& left, const BroadphasePair& right) {
                if (left.first != right.first) return left.first < right.first;
                return left.second < right.second;
            });
        result.pairs.erase(
            std::unique(result.pairs.begin(), result.pairs.end()),
            result.pairs.end());
        return result;
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<ColliderProxyId, Aabb> proxies_;
};

} // namespace dream::physics2d
