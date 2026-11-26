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
    AssetImporter mtl_importer;
    require(registry.find(".MTL",id,mtl_importer)&&id==builtin_mtl_importer_id,
            "MTL extension matching is case-insensitive");

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


    constexpr std::string_view material_source=
        "newmtl bronze\n"
        "Ka 0.1 0.1 0.1\n"
        "Kd 0.8 0.4 0.2\n"
        "Ks 0.5 0.5 0.5\n"
        "Ns 32\n"
        "d 0.75\n"
        "map_Kd textures/bronze.png\n"
        "map_Bump textures/bronze_normal.png\n";
    const auto material_bytes=std::as_bytes(
        std::span{material_source.data(),material_source.size()});
    const auto material_result=mtl_importer(
        "models/materials/bronze.mtl",material_bytes,{});
    require(material_result.error.empty()&&material_result.artifacts.size()==1U,
            "valid MTL produces one typed material artifact");
    const auto& material_artifact=material_result.artifacts.front();
    require(material_artifact.output_path=="models/materials/bronze.dmat"&&
            material_artifact.type_id==builtin_material_asset_type_v1&&
            material_artifact.dependencies.size()==2U,
            "MTL artifact stores a canonical path, material type and texture dependencies");
    BuiltinMaterialDataV1 materials;
    require(decode_builtin_material_v1(material_artifact.payload,materials,error),
            "serialized MTL material data decodes");
    require(materials.materials.size()==1U&&materials.materials[0].name=="bronze"&&
            materials.materials[0].diffuse[0]>0.79F&&
            materials.materials[0].opacity>0.74F&&
            materials.materials[0].diffuse_map=="models/materials/textures/bronze.png",
            "decoded material preserves colours, opacity and resolved texture paths");

    constexpr std::string_view unsafe_material=
        "newmtl bad\nmap_Kd ../../outside.png\n";
    const auto unsafe_bytes=std::as_bytes(
        std::span{unsafe_material.data(),unsafe_material.size()});
    require(!mtl_importer("models/bad.mtl",unsafe_bytes,{}).error.empty(),
            "MTL importer rejects traversal in texture dependencies");

    constexpr std::string_view obj_with_material=
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "mtllib materials/scene.mtl\nf 1 2 3\n";
    const auto obj_with_material_bytes=std::as_bytes(
        std::span{obj_with_material.data(),obj_with_material.size()});
    const auto linked_obj=importer("models/scene.obj",obj_with_material_bytes,{});
    require(linked_obj.error.empty()&&
            linked_obj.artifacts.front().dependencies==
                std::vector<std::string>{"models/materials/scene.mtl"},
            "OBJ records its relative mtllib dependency");

    std::cout<<"Dream Engine built-in importer tests passed.\n";
    return EXIT_SUCCESS;
}
