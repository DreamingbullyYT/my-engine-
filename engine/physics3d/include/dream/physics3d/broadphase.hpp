#pragma once

#include <dream/physics3d/geometry.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace dream::physics3d {

using BroadphaseProxyId3D = std::uint64_t;

struct BroadphasePair3D final {
    BroadphaseProxyId3D first{0};
    BroadphaseProxyId3D second{0};
    friend constexpr bool operator==(BroadphasePair3D,BroadphasePair3D) noexcept = default;
};

// Deterministic sweep-and-prune broadphase. Proxy IDs are supplied by the
// owning physics world; zero is reserved. The class is single-thread-owned.
class Broadphase3D final {
    struct Entry final {
        BroadphaseProxyId3D id{0U};
        Aabb3D bounds{};
    };

public:
    [[nodiscard]] bool upsert(BroadphaseProxyId3D id,const Aabb3D& bounds) {
        if(id==0||!bounds.valid()) return false;
        proxies_.insert_or_assign(id,bounds);
        index_dirty_=true;
        return true;
    }
    [[nodiscard]] bool remove(BroadphaseProxyId3D id) noexcept {
        const bool removed=proxies_.erase(id)!=0U;
        index_dirty_=index_dirty_||removed;
        return removed;
    }
    [[nodiscard]] bool contains(BroadphaseProxyId3D id) const noexcept {
        return proxies_.contains(id);
    }
    [[nodiscard]] std::size_t proxy_count() const noexcept {
        return proxies_.size();
    }
    void clear() noexcept {
        proxies_.clear();
        ordered_.clear();
        prefix_maximum_x_.clear();
        index_dirty_=false;
    }

    // Returns candidate proxy IDs in stable ascending ID order. A cached
    // interval index finds the first prefix whose maximum X reaches the query
    // and stops once proxy minimum X passes the query's maximum X.
    [[nodiscard]] std::vector<BroadphaseProxyId3D> query_aabb(
        const Aabb3D& bounds) const {
        std::vector<BroadphaseProxyId3D> result;
        if(!bounds.valid()||proxies_.empty()) return result;
        ensure_interval_index();

        const auto first=std::lower_bound(
            prefix_maximum_x_.begin(),prefix_maximum_x_.end(),bounds.minimum.x);
        if(first==prefix_maximum_x_.end()) return result;
        const auto first_index=static_cast<std::size_t>(
            std::distance(prefix_maximum_x_.begin(),first));
        for(std::size_t i=first_index;i<ordered_.size();++i) {
            const auto& entry=ordered_[i];
            if(entry.bounds.minimum.x>bounds.maximum.x) break;
            if(intersects(bounds,entry.bounds)) result.push_back(entry.id);
        }
        std::sort(result.begin(),result.end());
        return result;
    }

    [[nodiscard]] std::vector<BroadphasePair3D> compute_pairs() const {
        ensure_interval_index();
        std::vector<BroadphasePair3D> pairs;
        for(std::size_t i=0;i<ordered_.size();++i) {
            const auto& a=ordered_[i];
            for(std::size_t j=i+1U;j<ordered_.size();++j) {
                const auto& b=ordered_[j];
                if(b.bounds.minimum.x>a.bounds.maximum.x) break;
                if(!intersects(a.bounds,b.bounds)) continue;
                pairs.push_back({std::min(a.id,b.id),std::max(a.id,b.id)});
            }
        }
        std::sort(pairs.begin(),pairs.end(),[](const auto& a,const auto& b) {
            return a.first==b.first?a.second<b.second:a.first<b.first;
        });
        return pairs;
    }

private:
    void ensure_interval_index() const {
        if(!index_dirty_) return;
        ordered_.clear();
        ordered_.reserve(proxies_.size());
        for(const auto& [id,bounds]:proxies_) ordered_.push_back({id,bounds});
        std::sort(ordered_.begin(),ordered_.end(),[](const Entry& a,const Entry& b) {
            if(a.bounds.minimum.x!=b.bounds.minimum.x)
                return a.bounds.minimum.x<b.bounds.minimum.x;
            return a.id<b.id;
        });
        prefix_maximum_x_.resize(ordered_.size());
        float maximum_x=-std::numeric_limits<float>::infinity();
        for(std::size_t i=0;i<ordered_.size();++i) {
            maximum_x=std::max(maximum_x,ordered_[i].bounds.maximum.x);
            prefix_maximum_x_[i]=maximum_x;
        }
        index_dirty_=false;
    }

    std::map<BroadphaseProxyId3D,Aabb3D> proxies_;
    mutable bool index_dirty_{true};
    mutable std::vector<Entry> ordered_;
    mutable std::vector<float> prefix_maximum_x_;
};

} // namespace dream::physics3d
