#include <dream/physics3d/broadphase.hpp>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
using namespace dream::physics3d;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
Aabb3D box(float x,float y=0.0F,float z=0.0F) {
    return Aabb3D::from_center_half_extents({x,y,z},{1,1,1});
}
}
int main() {
    Broadphase3D broadphase;
    require(!broadphase.upsert(0,box(0)),"zero proxy ID is rejected");
    require(!broadphase.upsert(1,Aabb3D{{2,0,0},{1,1,1}}),
        "invalid bounds are rejected");
    require(broadphase.proxy_count()==0,"invalid insertion leaves broadphase unchanged");

    require(broadphase.upsert(40,box(0)),"first proxy inserts");
    require(broadphase.upsert(10,box(1.5F)),"second proxy inserts");
    require(broadphase.upsert(30,box(0,1.5F)),"third proxy inserts");
    require(broadphase.upsert(20,box(8)),"distant proxy inserts");
    const auto pairs=broadphase.compute_pairs();
    require(pairs.size()==3,"all overlapping AABB pairs are reported");
    require((pairs[0]==BroadphasePair3D{10,30}&&
             pairs[1]==BroadphasePair3D{10,40}&&
             pairs[2]==BroadphasePair3D{30,40}),
        "pairs are canonical and sorted independently of insertion order");

    require(broadphase.upsert(20,box(0)),"existing proxy can update bounds");
    const auto updated=broadphase.compute_pairs();
    require(updated.size()==6,"updating a proxy refreshes candidate pairs");
    require(broadphase.contains(20),"proxy lookup succeeds");
    require(broadphase.remove(20),"proxy removal succeeds");
    require(!broadphase.remove(20)&&!broadphase.contains(20),
        "removed proxy is no longer present");
    require(broadphase.compute_pairs().size()==3,
        "removing a proxy removes its candidate pairs");
    broadphase.clear();
    require(broadphase.proxy_count()==0&&broadphase.compute_pairs().empty(),
        "clear removes every proxy");
    return EXIT_SUCCESS;
}
