#pragma once

#include <dream/physics3d/geometry.hpp>

#include <algorithm>
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
public:
    [[nodiscard]] bool upsert(BroadphaseProxyId3D id,const Aabb3D& bounds) {
        if(id==0||!bounds.valid()) return false;
        proxies_.insert_or_assign(id,bounds);
        return true;
    }
    [[nodiscard]] bool remove(BroadphaseProxyId3D id) noexcept {
        return proxies_.erase(id)!=0U;
    }
    [[nodiscard]] bool contains(BroadphaseProxyId3D id) const noexcept {
        return proxies_.contains(id);
    }
    [[nodiscard]] std::size_t proxy_count() const noexcept {
        return proxies_.size();
    }
    void clear() noexcept { proxies_.clear(); }

    [[nodiscard]] std::vector<BroadphasePair3D> compute_pairs() const {
        struct Entry final { BroadphaseProxyId3D id; Aabb3D bounds; };
        std::vector<Entry> ordered;
        ordered.reserve(proxies_.size());
        for(const auto& [id,bounds]:proxies_) ordered.push_back({id,bounds});
        std::sort(ordered.begin(),ordered.end(),[](const Entry& a,const Entry& b) {
            if(a.bounds.minimum.x!=b.bounds.minimum.x)
                return a.bounds.minimum.x<b.bounds.minimum.x;
            return a.id<b.id;
        });

        std::vector<BroadphasePair3D> pairs;
        for(std::size_t i=0;i<ordered.size();++i) {
            const auto& a=ordered[i];
            for(std::size_t j=i+1U;j<ordered.size();++j) {
                const auto& b=ordered[j];
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
    std::map<BroadphaseProxyId3D,Aabb3D> proxies_;
};

} // namespace dream::physics3d
