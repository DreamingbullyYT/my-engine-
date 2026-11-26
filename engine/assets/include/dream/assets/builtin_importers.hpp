#pragma once

#include <dream/assets/import_pipeline.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <set>
#include <sstream>
#include <locale>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace dream::assets {

inline constexpr std::string_view builtin_obj_importer_id{"dream.builtin.obj"};
inline constexpr std::string_view builtin_mtl_importer_id{"dream.builtin.mtl"};
inline constexpr std::uint64_t builtin_mesh_asset_type_v1{0x445245414D4D4553ULL};
inline constexpr std::uint32_t builtin_mesh_asset_version{1U};
inline constexpr std::uint64_t builtin_material_asset_type_v1{0x445245414D4D4154ULL};
inline constexpr std::uint32_t builtin_material_asset_version{1U};

struct BuiltinMeshVertexV1 final {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};
};

struct BuiltinMeshDataV1 final {
    std::vector<BuiltinMeshVertexV1> vertices;
    std::vector<std::uint32_t> indices;
};

struct BuiltinMaterialV1 final {
    std::string name;
    std::array<float,3> ambient{0.0F,0.0F,0.0F};
    std::array<float,3> diffuse{1.0F,1.0F,1.0F};
    std::array<float,3> specular{0.0F,0.0F,0.0F};
    float shininess{0.0F};
    float opacity{1.0F};
    std::string diffuse_map;
    std::string specular_map;
    std::string normal_map;
};

struct BuiltinMaterialDataV1 final {
    std::vector<BuiltinMaterialV1> materials;
};

namespace builtin_importer_detail {

struct FaceIndex final {
    std::int64_t position{-1};
    std::int64_t texcoord{-1};
    std::int64_t normal{-1};
};

struct VertexKey final {
    std::int64_t position{-1};
    std::int64_t texcoord{-1};
    std::int64_t normal{-1};
    std::uint64_t generated_normal_face{0};
    friend bool operator<(const VertexKey& a,const VertexKey& b) noexcept {
        if(a.position!=b.position)return a.position<b.position;
        if(a.texcoord!=b.texcoord)return a.texcoord<b.texcoord;
        if(a.normal!=b.normal)return a.normal<b.normal;
        return a.generated_normal_face<b.generated_normal_face;
    }
};

[[nodiscard]] inline bool parse_float(std::string_view token,float& output) {
    if(token.empty()||token.size()>128U)return false;
    std::istringstream input{std::string(token)};
    input.imbue(std::locale::classic());
    float parsed{};
    if(!(input>>parsed)||input.peek()!=std::char_traits<char>::eof()||
       !std::isfinite(parsed))return false;
    output=parsed;
    return true;
}

[[nodiscard]] inline bool parse_integer(
    std::string_view token,std::int64_t& output) noexcept {
    if(token.empty())return false;
    const auto parsed=std::from_chars(token.data(),token.data()+token.size(),output);
    return parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&output!=0;
}

[[nodiscard]] inline bool resolve_index(
    std::int64_t source,std::size_t count,std::int64_t& resolved) noexcept {
    if(source==0)return false;
    const std::int64_t signed_count=static_cast<std::int64_t>(count);
    const std::int64_t value=source>0?source-1:signed_count+source;
    if(value<0||value>=signed_count)return false;
    resolved=value;
    return true;
}

[[nodiscard]] inline bool parse_face_index(
    std::string_view token,std::size_t positions,std::size_t texcoords,
    std::size_t normals,FaceIndex& output) noexcept {
    std::array<std::string_view,3> fields{};
    std::size_t field=0U,start=0U;
    for(std::size_t i=0;i<=token.size();++i) {
        if(i==token.size()||token[i]=='/') {
            if(field>=fields.size())return false;
            fields[field++]=token.substr(start,i-start);
            start=i+1U;
        }
    }
    if(field==0U||fields[0].empty())return false;
    std::int64_t raw{};
    if(!parse_integer(fields[0],raw)||
       !resolve_index(raw,positions,output.position))return false;
    if(field>=2U&&!fields[1].empty()) {
        if(!parse_integer(fields[1],raw)||
           !resolve_index(raw,texcoords,output.texcoord))return false;
    }
    if(field>=3U&&!fields[2].empty()) {
        if(!parse_integer(fields[2],raw)||
           !resolve_index(raw,normals,output.normal))return false;
    }
    return true;
}

[[nodiscard]] inline std::array<float,3> subtract(
    const std::array<float,3>& a,const std::array<float,3>& b) noexcept {
    return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};
}
[[nodiscard]] inline std::array<float,3> cross(
    const std::array<float,3>& a,const std::array<float,3>& b) noexcept {
    return {a[1]*b[2]-a[2]*b[1],
        a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
[[nodiscard]] inline bool normalize(
    std::array<float,3>& value) noexcept {
    const float length_squared=value[0]*value[0]+value[1]*value[1]+
        value[2]*value[2];
    if(!std::isfinite(length_squared)||length_squared<=1.0e-16F)return false;
    const float inverse=1.0F/std::sqrt(length_squared);
    for(auto& component:value)component*=inverse;
    return true;
}

inline void append_u32(std::vector<std::byte>& bytes,std::uint32_t value) {
    for(unsigned shift=0U;shift<32U;shift+=8U)
        bytes.push_back(static_cast<std::byte>((value>>shift)&0xFFU));
}
inline void append_float(std::vector<std::byte>& bytes,float value) {
    append_u32(bytes,std::bit_cast<std::uint32_t>(value));
}


[[nodiscard]] inline std::string_view trim_ascii(std::string_view value) noexcept {
    while(!value.empty()&&(value.front()==' '||value.front()=='\t'))value.remove_prefix(1U);
    while(!value.empty()&&(value.back()==' '||value.back()=='\t'||value.back()=='\r'))
        value.remove_suffix(1U);
    return value;
}

[[nodiscard]] inline bool resolve_asset_reference(
    std::string_view source_path,std::string_view reference,std::string& output) {
    reference=trim_ascii(reference);
    if(reference.empty()||reference.size()>4096U||reference.front()=='/'||
       reference.find('\\')!=std::string_view::npos||
       reference.find(':')!=std::string_view::npos)return false;
    const auto slash=source_path.find_last_of('/');
    output=slash==std::string_view::npos
        ?std::string(reference)
        :std::string(source_path.substr(0U,slash+1U))+std::string(reference);
    return output.size()<=4096U&&platform::is_valid_virtual_path(output);
}

inline void append_string(std::vector<std::byte>& bytes,std::string_view value) {
    append_u32(bytes,static_cast<std::uint32_t>(value.size()));
    for(const unsigned char c:value)bytes.push_back(static_cast<std::byte>(c));
}

[[nodiscard]] inline AssetImporterOutput import_obj(
    std::string_view source_path,std::span<const std::byte> source_bytes,
    std::string_view options) {
    AssetImporterOutput output;
    if(!options.empty()) {
        output.error="The built-in OBJ importer currently accepts no import options";
        return output;
    }
    if(source_bytes.empty()||source_bytes.size()>64U*1024U*1024U) {
        output.error="OBJ source is empty or exceeds the 64 MiB parser limit";
        return output;
    }

    std::vector<std::array<float,3>> positions;
    std::vector<std::array<float,2>> texcoords;
    std::vector<std::array<float,3>> normals;
    BuiltinMeshDataV1 mesh;
    std::map<VertexKey,std::uint32_t> unique_vertices;
    const std::string_view source{
        reinterpret_cast<const char*>(source_bytes.data()),source_bytes.size()};
    constexpr std::size_t maximum_source_attributes=1'000'000U;
    constexpr std::size_t maximum_mesh_vertices=1'000'000U;
    constexpr std::size_t maximum_mesh_indices=3'000'000U;
    std::uint64_t face_id=0U;
    std::set<std::string,std::less<>> material_libraries;

    for(std::size_t line_begin=0U;line_begin<source.size();) {
        const auto newline=source.find('\n',line_begin);
        const auto line_end=newline==std::string_view::npos?source.size():newline;
        auto line=source.substr(line_begin,line_end-line_begin);
        line_begin=line_end+(newline==std::string_view::npos?0U:1U);
        if(!line.empty()&&line.back()=='\r')line.remove_suffix(1U);
        if(line.size()>65536U) {
            output.error="OBJ line exceeds the 64 KiB parser limit";
            return output;
        }

        std::size_t cursor=0U;
        auto next_token=[&]() -> std::string_view {
            while(cursor<line.size()&&(line[cursor]==' '||line[cursor]=='\t'))++cursor;
            const auto start=cursor;
            while(cursor<line.size()&&line[cursor]!=' '&&line[cursor]!='\t'&&
                  line[cursor]!='\r')++cursor;
            return line.substr(start,cursor-start);
        };
        const auto command=next_token();
        if(command.empty()||command.front()=='#')continue;

        if(command=="v"||command=="vn") {
            std::array<float,3> value{};
            for(auto& component:value) {
                const auto token=next_token();
                if(!parse_float(token,component)) {
                    output.error="OBJ position or normal contains an invalid component";
                    return output;
                }
            }
            if(command=="v")positions.push_back(value);
            else normals.push_back(value);
            if(positions.size()>maximum_source_attributes||
               normals.size()>maximum_source_attributes) {
                output.error="OBJ source exceeds the attribute-count limit";
                return output;
            }
        } else if(command=="vt") {
            std::array<float,2> value{};
            for(auto& component:value) {
                const auto token=next_token();
                if(!parse_float(token,component)) {
                    output.error="OBJ texture coordinate contains an invalid component";
                    return output;
                }
            }
            texcoords.push_back(value);
            if(texcoords.size()>maximum_source_attributes) {
                output.error="OBJ source exceeds the texture-coordinate limit";
                return output;
            }
        } else if(command=="mtllib") {
            for(auto token=next_token();!token.empty()&&token.front()!='#';
                token=next_token()) {
                std::string dependency;
                if(!resolve_asset_reference(source_path,token,dependency)) {
                    output.error="OBJ mtllib contains an unsafe or invalid relative path";
                    return output;
                }
                material_libraries.emplace(std::move(dependency));
                if(material_libraries.size()>256U) {
                    output.error="OBJ references more than 256 material libraries";
                    return output;
                }
            }
        } else if(command=="f") {
            std::vector<FaceIndex> face;
            for(auto token=next_token();!token.empty()&&token.front()!='#';
                token=next_token()) {
                FaceIndex index;
                if(!parse_face_index(token,positions.size(),texcoords.size(),
                                     normals.size(),index)) {
                    output.error="OBJ face contains an invalid position/UV/normal index";
                    return output;
                }
                face.push_back(index);
                if(face.size()>4096U) {
                    output.error="OBJ polygon exceeds the 4096-vertex face limit";
                    return output;
                }
            }
            if(face.size()<3U) {
                output.error="OBJ face has fewer than three vertices";
                return output;
            }
            ++face_id;
            auto generated_normal=cross(
                subtract(positions[static_cast<std::size_t>(face[1].position)],
                         positions[static_cast<std::size_t>(face[0].position)]),
                subtract(positions[static_cast<std::size_t>(face[2].position)],
                         positions[static_cast<std::size_t>(face[0].position)]));
            if(!normalize(generated_normal)) {
                output.error="OBJ face is degenerate and has no usable normal";
                return output;
            }

            const auto append_vertex=[&](const FaceIndex& index)->std::optional<std::uint32_t> {
                const VertexKey key{
                    index.position,index.texcoord,index.normal,
                    index.normal<0?face_id:0U};
                if(const auto found=unique_vertices.find(key);
                   found!=unique_vertices.end())return found->second;
                if(mesh.vertices.size()>=maximum_mesh_vertices)return std::nullopt;
                BuiltinMeshVertexV1 vertex;
                vertex.position=positions[static_cast<std::size_t>(index.position)];
                vertex.normal=index.normal>=0
                    ?normals[static_cast<std::size_t>(index.normal)]:generated_normal;
                if(index.texcoord>=0)
                    vertex.uv=texcoords[static_cast<std::size_t>(index.texcoord)];
                const auto id=static_cast<std::uint32_t>(mesh.vertices.size());
                mesh.vertices.push_back(vertex);
                unique_vertices.emplace(key,id);
                return id;
            };

            for(std::size_t fan=1U;fan+1U<face.size();++fan) {
                for(const auto corner:{std::size_t{0U},fan,fan+1U}) {
                    const auto vertex=append_vertex(face[corner]);
                    if(!vertex) {
                        output.error="OBJ output exceeds the one-million-vertex limit";
                        return output;
                    }
                    mesh.indices.push_back(*vertex);
                    if(mesh.indices.size()>maximum_mesh_indices) {
                        output.error="OBJ output exceeds the three-million-index limit";
                        return output;
                    }
                }
            }
        }
    }

    if(mesh.vertices.empty()||mesh.indices.empty()) {
        output.error="OBJ file did not contain any renderable faces";
        return output;
    }
    if(mesh.vertices.size()>std::numeric_limits<std::uint32_t>::max()||
       mesh.indices.size()>std::numeric_limits<std::uint32_t>::max()) {
        output.error="OBJ mesh exceeds the serialized index range";
        return output;
    }

    std::string output_path(source_path);
    const auto slash=output_path.find_last_of('/');
    const auto dot=output_path.find_last_of('.');
    if(dot!=std::string::npos&&(slash==std::string::npos||dot>slash))
        output_path.resize(dot);
    output_path.append(".dmesh");
    if(output_path.size()>4096U) {
        output.error="OBJ output path exceeds the virtual-path limit";
        return output;
    }

    std::vector<std::byte> payload;
    const std::size_t vertex_bytes=mesh.vertices.size()*32U;
    const std::size_t index_bytes=mesh.indices.size()*4U;
    if(vertex_bytes>std::numeric_limits<std::size_t>::max()-16U||
       index_bytes>std::numeric_limits<std::size_t>::max()-16U-vertex_bytes) {
        output.error="OBJ mesh payload size overflow";
        return output;
    }
    payload.reserve(16U+vertex_bytes+index_bytes);
    for(const char c:std::array<char,4>{'D','M','E','S'})
        payload.push_back(static_cast<std::byte>(c));
    append_u32(payload,builtin_mesh_asset_version);
    append_u32(payload,static_cast<std::uint32_t>(mesh.vertices.size()));
    append_u32(payload,static_cast<std::uint32_t>(mesh.indices.size()));
    for(const auto& vertex:mesh.vertices) {
        for(const auto value:vertex.position)append_float(payload,value);
        for(const auto value:vertex.normal)append_float(payload,value);
        for(const auto value:vertex.uv)append_float(payload,value);
    }
    for(const auto index:mesh.indices)append_u32(payload,index);

    ImportedArtifact artifact;
    artifact.output_path=std::move(output_path);
    artifact.type_id=builtin_mesh_asset_type_v1;
    artifact.version=builtin_mesh_asset_version;
    artifact.payload=std::move(payload);
    artifact.dependencies.assign(material_libraries.begin(),material_libraries.end());
    output.artifacts.push_back(std::move(artifact));
    return output;
}

[[nodiscard]] inline AssetImporterOutput import_mtl(
    std::string_view source_path,std::span<const std::byte> source_bytes,
    std::string_view options) {
    AssetImporterOutput output;
    if(!options.empty()) {
        output.error="The built-in MTL importer currently accepts no import options";
        return output;
    }
    if(source_bytes.empty()||source_bytes.size()>16U*1024U*1024U) {
        output.error="MTL source is empty or exceeds the 16 MiB parser limit";
        return output;
    }

    const std::string_view source{
        reinterpret_cast<const char*>(source_bytes.data()),source_bytes.size()};
    BuiltinMaterialDataV1 data;
    std::set<std::string,std::less<>> names;
    std::set<std::string,std::less<>> dependencies;
    BuiltinMaterialV1* current=nullptr;

    for(std::size_t line_begin=0U;line_begin<source.size();) {
        const auto newline=source.find('\n',line_begin);
        const auto line_end=newline==std::string_view::npos?source.size():newline;
        auto line=source.substr(line_begin,line_end-line_begin);
        line_begin=line_end+(newline==std::string_view::npos?0U:1U);
        if(!line.empty()&&line.back()=='\r')line.remove_suffix(1U);
        if(line.size()>65536U) {
            output.error="MTL line exceeds the 64 KiB parser limit";
            return output;
        }
        auto content=trim_ascii(line);
        if(content.empty()||content.front()=='#')continue;
        std::size_t cursor=0U;
        while(cursor<content.size()&&content[cursor]!=' '&&content[cursor]!='\t')++cursor;
        const auto command=content.substr(0U,cursor);
        const auto value=trim_ascii(content.substr(cursor));

        if(command=="newmtl") {
            if(value.empty()||value.size()>256U||value.front()=='#') {
                output.error="MTL material name is empty or exceeds 256 bytes";
                return output;
            }
            const auto comment=value.find('#');
            const auto name=trim_ascii(value.substr(0U,comment));
            if(name.empty()||!names.emplace(name).second||data.materials.size()>=10000U) {
                output.error="MTL contains an empty/duplicate material name or too many materials";
                return output;
            }
            data.materials.emplace_back();
            data.materials.back().name=std::string(name);
            current=&data.materials.back();
            continue;
        }
        if(!current) {
            output.error="MTL material properties must follow a newmtl declaration";
            return output;
        }
        const auto parse_scalar=[&](float& target,float minimum,float maximum)->bool {
            return parse_float(value,target)&&target>=minimum&&target<=maximum;
        };
        const auto parse_colour=[&](std::array<float,3>& target)->bool {
            std::istringstream input{std::string(value)};
            input.imbue(std::locale::classic());
            if(!(input>>target[0]>>target[1]>>target[2]))return false;
            std::string extra;
            if(input>>extra)return false;
            return std::all_of(target.begin(),target.end(),[](float v) {
                return std::isfinite(v)&&v>=0.0F&&v<=1.0e6F;
            });
        };
        if(command=="Ka") {
            if(!parse_colour(current->ambient)) {
                output.error="MTL Ka ambient colour is invalid";
                return output;
            }
        } else if(command=="Kd") {
            if(!parse_colour(current->diffuse)) {
                output.error="MTL Kd diffuse colour is invalid";
                return output;
            }
        } else if(command=="Ks") {
            if(!parse_colour(current->specular)) {
                output.error="MTL Ks specular colour is invalid";
                return output;
            }
        } else if(command=="Ns") {
            if(!parse_scalar(current->shininess,0.0F,1000.0F)) {
                output.error="MTL Ns shininess must be in the range 0..1000";
                return output;
            }
        } else if(command=="d") {
            if(!parse_scalar(current->opacity,0.0F,1.0F)) {
                output.error="MTL d opacity must be in the range 0..1";
                return output;
            }
        } else if(command=="Tr") {
            float transparency{};
            if(!parse_scalar(transparency,0.0F,1.0F)) {
                output.error="MTL Tr transparency must be in the range 0..1";
                return output;
            }
            current->opacity=1.0F-transparency;
        } else if(command=="map_Kd"||command=="map_Ks"||
                  command=="map_Bump"||command=="bump") {
            auto path=value;
            const auto comment=path.find('#');
            if(comment!=std::string_view::npos)path=trim_ascii(path.substr(0U,comment));
            if(path.empty()||path.front()=='-') {
                output.error="MTL texture maps must use a plain relative path; map options are unsupported";
                return output;
            }
            std::string resolved;
            if(!resolve_asset_reference(source_path,path,resolved)) {
                output.error="MTL texture map contains an unsafe or invalid relative path";
                return output;
            }
            if(command=="map_Kd")current->diffuse_map=resolved;
            else if(command=="map_Ks")current->specular_map=resolved;
            else current->normal_map=resolved;
            dependencies.emplace(std::move(resolved));
            if(dependencies.size()>10000U) {
                output.error="MTL references too many textures";
                return output;
            }
        }
        // Unused standard material fields (illum, Ni, Tf, Ke, etc.) are ignored.
    }

    if(data.materials.empty()) {
        output.error="MTL file did not declare any materials";
        return output;
    }

    std::vector<std::byte> payload;
    for(const char c:std::array<char,4>{'D','M','A','T'})
        payload.push_back(static_cast<std::byte>(c));
    append_u32(payload,builtin_material_asset_version);
    append_u32(payload,static_cast<std::uint32_t>(data.materials.size()));
    for(const auto& material:data.materials) {
        append_string(payload,material.name);
        for(const auto value:material.ambient)append_float(payload,value);
        for(const auto value:material.diffuse)append_float(payload,value);
        for(const auto value:material.specular)append_float(payload,value);
        append_float(payload,material.shininess);
        append_float(payload,material.opacity);
        append_string(payload,material.diffuse_map);
        append_string(payload,material.specular_map);
        append_string(payload,material.normal_map);
        if(payload.size()>64U*1024U*1024U) {
            output.error="MTL serialized material data exceeds the 64 MiB limit";
            return output;
        }
    }

    std::string output_path(source_path);
    const auto slash=output_path.find_last_of('/');
    const auto dot=output_path.find_last_of('.');
    if(dot!=std::string::npos&&(slash==std::string::npos||dot>slash))
        output_path.resize(dot);
    output_path.append(".dmat");
    if(output_path.size()>4096U) {
        output.error="MTL output path exceeds the virtual-path limit";
        return output;
    }
    ImportedArtifact artifact;
    artifact.output_path=std::move(output_path);
    artifact.type_id=builtin_material_asset_type_v1;
    artifact.version=builtin_material_asset_version;
    artifact.payload=std::move(payload);
    artifact.dependencies.assign(dependencies.begin(),dependencies.end());
    output.artifacts.push_back(std::move(artifact));
    return output;
}

} // namespace builtin_importer_detail

[[nodiscard]] inline bool decode_builtin_mesh_v1(
    std::span<const std::byte> payload,BuiltinMeshDataV1& output,
    std::string& error) {
    const auto read_u32=[&](std::size_t offset,std::uint32_t& value)->bool {
        if(offset>payload.size()||payload.size()-offset<4U)return false;
        value=0U;
        for(unsigned byte=0U;byte<4U;++byte)
            value|=static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(payload[offset+byte]))<<(byte*8U);
        return true;
    };
    if(payload.size()<16U||payload[0]!=std::byte{'D'}||
       payload[1]!=std::byte{'M'}||payload[2]!=std::byte{'E'}||
       payload[3]!=std::byte{'S'}) {
        error="Mesh payload has an invalid DMES header";
        return false;
    }
    std::uint32_t version{},vertex_count{},index_count{};
    if(!read_u32(4U,version)||!read_u32(8U,vertex_count)||
       !read_u32(12U,index_count)||version!=builtin_mesh_asset_version||
       vertex_count==0U||index_count==0U||index_count%3U!=0U) {
        error="Mesh payload header or counts are invalid";
        return false;
    }
    constexpr std::size_t header=16U,vertex_size=32U,index_size=4U;
    const std::size_t vertex_bytes=static_cast<std::size_t>(vertex_count)*vertex_size;
    if(vertex_bytes/vertex_size!=vertex_count) {
        error="Mesh vertex payload size overflows";
        return false;
    }
    if(index_count>(std::numeric_limits<std::size_t>::max()-header-vertex_bytes)/index_size) {
        error="Mesh index payload size overflows";
        return false;
    }
    const std::size_t expected=header+vertex_bytes+
        static_cast<std::size_t>(index_count)*index_size;
    if(payload.size()!=expected) {
        error="Mesh payload length does not match the declared counts";
        return false;
    }

    BuiltinMeshDataV1 decoded;
    decoded.vertices.resize(vertex_count);
    decoded.indices.resize(index_count);
    std::size_t offset=header;
    const auto read_float=[&](float& value)->bool {
        std::uint32_t bits{};
        if(!read_u32(offset,bits))return false;
        value=std::bit_cast<float>(bits);
        offset+=4U;
        return std::isfinite(value);
    };
    for(auto& vertex:decoded.vertices) {
        for(auto& value:vertex.position)if(!read_float(value)) {
            error="Mesh vertex contains a non-finite position";return false;
        }
        for(auto& value:vertex.normal)if(!read_float(value)) {
            error="Mesh vertex contains a non-finite normal";return false;
        }
        for(auto& value:vertex.uv)if(!read_float(value)) {
            error="Mesh vertex contains non-finite texture coordinates";return false;
        }
    }
    for(auto& index:decoded.indices) {
        if(!read_u32(offset,index)||index>=vertex_count) {
            error="Mesh index is outside the vertex array";return false;
        }
        offset+=4U;
    }
    output=std::move(decoded);
    error.clear();
    return true;
}


[[nodiscard]] inline bool decode_builtin_material_v1(
    std::span<const std::byte> payload,BuiltinMaterialDataV1& output,
    std::string& error) {
    if(payload.size()<12U||payload.size()>64U*1024U*1024U||
       payload[0]!=std::byte{'D'}||payload[1]!=std::byte{'M'}||
       payload[2]!=std::byte{'A'}||payload[3]!=std::byte{'T'}) {
        error="Material payload has an invalid DMAT header or size";
        return false;
    }
    std::size_t offset=4U;
    const auto read_u32=[&](std::uint32_t& value)->bool {
        if(offset>payload.size()||payload.size()-offset<4U)return false;
        value=0U;
        for(unsigned byte=0U;byte<4U;++byte)
            value|=static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(payload[offset+byte]))<<(byte*8U);
        offset+=4U;
        return true;
    };
    const auto read_float=[&](float& value)->bool {
        std::uint32_t bits{};
        if(!read_u32(bits))return false;
        value=std::bit_cast<float>(bits);
        return std::isfinite(value);
    };
    const auto read_string=[&](std::string& value,std::size_t maximum)->bool {
        std::uint32_t length{};
        if(!read_u32(length)||length>maximum||offset>payload.size()||
           payload.size()-offset<length)return false;
        std::string candidate;
        candidate.reserve(length);
        for(std::size_t i=0U;i<length;++i) {
            const char c=static_cast<char>(static_cast<std::uint8_t>(payload[offset+i]));
            if(c=='\0')return false;
            candidate.push_back(c);
        }
        offset+=length;
        value=std::move(candidate);
        return true;
    };

    std::uint32_t version{},count{};
    if(!read_u32(version)||!read_u32(count)||
       version!=builtin_material_asset_version||count==0U||count>10000U) {
        error="Material payload version or material count is invalid";
        return false;
    }
    BuiltinMaterialDataV1 decoded;
    decoded.materials.reserve(count);
    std::set<std::string,std::less<>> names;
    for(std::uint32_t i=0U;i<count;++i) {
        BuiltinMaterialV1 material;
        if(!read_string(material.name,256U)||material.name.empty()||
           !names.emplace(material.name).second) {
            error="Material payload contains an invalid or duplicate name";
            return false;
        }
        for(auto& value:material.ambient)if(!read_float(value)||value<0.0F) {
            error="Material ambient colour is invalid";return false;
        }
        for(auto& value:material.diffuse)if(!read_float(value)||value<0.0F) {
            error="Material diffuse colour is invalid";return false;
        }
        for(auto& value:material.specular)if(!read_float(value)||value<0.0F) {
            error="Material specular colour is invalid";return false;
        }
        if(!read_float(material.shininess)||material.shininess<0.0F||
           material.shininess>1000.0F||!read_float(material.opacity)||
           material.opacity<0.0F||material.opacity>1.0F||
           !read_string(material.diffuse_map,4096U)||
           !read_string(material.specular_map,4096U)||
           !read_string(material.normal_map,4096U)) {
            error="Material scalar or texture path is invalid";
            return false;
        }
        for(const auto* path:{&material.diffuse_map,&material.specular_map,&material.normal_map}) {
            if(!path->empty()&&!platform::is_valid_virtual_path(*path)) {
                error="Material payload contains an unsafe texture dependency";
                return false;
            }
        }
        decoded.materials.push_back(std::move(material));
    }
    if(offset!=payload.size()) {
        error="Material payload contains trailing bytes";
        return false;
    }
    output=std::move(decoded);
    error.clear();
    return true;
}

[[nodiscard]] inline AssetImporterRegistryError register_builtin_importers(
    AssetImporterRegistry& registry) {
    const auto obj_status=registry.register_importer(AssetImporterRegistration{
        std::string(builtin_obj_importer_id),
        {".obj"},
        [](std::string_view path,std::span<const std::byte> bytes,
           std::string_view options) {
            return builtin_importer_detail::import_obj(path,bytes,options);
        }});
    if(obj_status!=AssetImporterRegistryError::None)return obj_status;
    return registry.register_importer(AssetImporterRegistration{
        std::string(builtin_mtl_importer_id),
        {".mtl"},
        [](std::string_view path,std::span<const std::byte> bytes,
           std::string_view options) {
            return builtin_importer_detail::import_mtl(path,bytes,options);
        }});
}

} // namespace dream::assets
