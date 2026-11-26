#include <dream/assets/builtin_importers.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
void require(bool value,std::string_view message) {
    if(!value) {
        std::cerr<<"FAILED: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main() {
    using namespace dream::assets;
    AssetImporterRegistry registry;
    require(register_builtin_importers(registry)==AssetImporterRegistryError::None,
            "built-in OBJ importer registers");
    std::string id;
    AssetImporter importer;
    require(registry.find(".OBJ",id,importer)&&id==builtin_obj_importer_id,
            "OBJ extension matching is case-insensitive");

    constexpr std::string_view source=
        "# one quad with negative indices and UV coordinates\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
        "f -4/1 -3/2 -2/3 -1/4\n";
    const auto bytes=std::as_bytes(std::span{source.data(),source.size()});
    const auto result=importer("models/quad.obj",bytes,{});
    require(result.error.empty()&&result.artifacts.size()==1U,
            "valid OBJ file produces exactly one mesh artifact");
    const auto& artifact=result.artifacts.front();
    require(artifact.output_path=="models/quad.dmesh"&&
            artifact.type_id==builtin_mesh_asset_type_v1&&
            artifact.version==builtin_mesh_asset_version,
            "mesh artifact includes canonical path and typed version metadata");

    BuiltinMeshDataV1 decoded;
    std::string error;
    require(decode_builtin_mesh_v1(artifact.payload,decoded,error),
            "serialized built-in mesh payload decodes");
    require(decoded.vertices.size()==4U&&decoded.indices.size()==6U,
            "quad is deduplicated and fan-triangulated");
    require(decoded.indices==std::vector<std::uint32_t>{0U,1U,2U,0U,2U,3U},
            "triangulation preserves the face winding");
    for(const auto& vertex:decoded.vertices)
        require(vertex.normal[2]>0.99F,
                "missing OBJ normals are generated from the face winding");

    constexpr std::string_view invalid=
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 4\n";
    const auto invalid_bytes=std::as_bytes(std::span{invalid.data(),invalid.size()});
    const auto rejected=importer("bad.obj",invalid_bytes,{});
    require(!rejected.error.empty()&&rejected.artifacts.empty(),
            "out-of-range OBJ indices are rejected without partial output");

    auto corrupt=artifact.payload;
    corrupt.back()=std::byte{0xFF};
    require(!decode_builtin_mesh_v1(corrupt,decoded,error),
            "mesh decoder rejects indices outside the vertex payload");

    std::cout<<"Dream Engine built-in importer tests passed.\n";
    return EXIT_SUCCESS;
}
