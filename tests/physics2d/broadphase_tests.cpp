#include <dream/physics2d/broadphase.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
using namespace dream::physics2d;

void require(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    SweepAndPrune broadphase;
    require(broadphase.size() == 0U, "new broadphase has no proxies");
    require(broadphase.upsert(30U, Aabb{{4, 0}, {5, 1}}) == BroadphaseError::None,
            "first proxy inserts");
    require(broadphase.upsert(10U, Aabb{{0, 0}, {2, 2}}) == BroadphaseError::None,
            "second proxy inserts");
    require(broadphase.upsert(20U, Aabb{{1, 1}, {3, 3}}) == BroadphaseError::None,
            "third proxy inserts");
    require(broadphase.upsert(40U, Aabb{{1, 4}, {3, 5}}) == BroadphaseError::None,
            "Y-separated proxy inserts");
    require(broadphase.size() == 4U, "proxy count tracks inserts");

    const auto pairs = broadphase.compute_pairs();
    require(pairs.pairs == std::vector<BroadphasePair>{{10U, 20U}},
            "sweep-and-prune returns only overlapping AABB pairs");
    require(pairs.candidate_tests == 3U,
            "sweep checks active X intervals before Y filtering");

    const auto stable_again = broadphase.compute_pairs();
    require(stable_again.pairs == pairs.pairs &&
            stable_again.candidate_tests == pairs.candidate_tests,
            "pair ordering and candidate count are deterministic");

    require(broadphase.upsert(30U, Aabb{{2, 0}, {4, 1}}) == BroadphaseError::None,
            "upsert updates an existing proxy");
    const auto touching_pairs = broadphase.compute_pairs();
    require(touching_pairs.pairs ==
                std::vector<BroadphasePair>{{10U, 20U}, {10U, 30U}},
            "touching AABBs remain broadphase candidates");

    require(broadphase.upsert(50U, Aabb{{2.5F, 2.5F}, {3.5F, 3.5F}}) ==
                BroadphaseError::None,
            "fifth proxy inserts");
    const auto expanded = broadphase.compute_pairs();
    require(expanded.pairs ==
                std::vector<BroadphasePair>{
                    {10U, 20U}, {10U, 30U}, {20U, 50U}},
            "candidate pairs are canonicalized and sorted by proxy ID");

    require(broadphase.contains(10U), "contains finds live proxy");
    require(broadphase.remove(10U), "proxy removal succeeds");
    require(!broadphase.remove(10U), "duplicate removal fails");
    require(!broadphase.contains(10U), "removed proxy is absent");
    const auto after_remove = broadphase.compute_pairs();
    require(after_remove.pairs ==
                std::vector<BroadphasePair>{{20U, 50U}},
            "removed proxies disappear from subsequent queries");

    require(broadphase.upsert(60U, Aabb{{2, 0}, {1, 1}}) ==
                BroadphaseError::InvalidBounds,
            "invalid AABB cannot enter broadphase");
    require(!broadphase.contains(60U),
            "rejected proxy does not mutate broadphase");

    const OrientedBox rotated{{0, 0}, {2, 0.5F}, 0.78539816339F};
    require(broadphase.upsert(70U, rotated.bounds()) == BroadphaseError::None,
            "oriented box bounds can be inserted as a broadphase proxy");

    broadphase.clear();
    require(broadphase.size() == 0U &&
            broadphase.compute_pairs().pairs.empty(),
            "clear removes every proxy");

    std::cout << "Dream Engine 2D broadphase tests passed.\n";
    return EXIT_SUCCESS;
}
