#pragma once

#include <dream/assets/import_pipeline.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace dream::assets {

inline constexpr std::string_view builtin_obj_importer_id{"dream.builtin.obj"};
inline constexpr std::uint64_t builtin_mesh_asset_type_v1{0x445245414D4D4553ULL};
inline constexpr std::uint32_t builtin_mesh_asset_version{1U};

struct BuiltinMeshVertexV1 final {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};
};

struct BuiltinMeshDataV1 final {
    std::vector<BuiltinMeshVertexV1> vertices;
    std::vector<std::uint32_t> indices;
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

[[nodiscard]] inline bool parse_float(std::string_view token,float& output) noexcept {
    if(token.empty())return false;
    const auto parsed=std::from_chars(token.data(),token.data()+token.size(),output);
    return parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&
        std::isfinite(output);
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
        const auto next_token=[&]() mutable -> std::string_view {
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
    if(vertex_count>(std::numeric_limits<std::size_t>::max()-header)/vertex_size) {
        error="Mesh vertex payload size overflows";
        return false;
    }
    const std::size_t vertex_bytes=static_cast<std::size_t>(vertex_count)*vertex_size;
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

[[nodiscard]] inline AssetImporterRegistryError register_builtin_importers(
    AssetImporterRegistry& registry) {
    return registry.register_importer(AssetImporterRegistration{
        std::string(builtin_obj_importer_id),
        {".obj"},
        [](std::string_view path,std::span<const std::byte> bytes,
           std::string_view options) {
            return builtin_importer_detail::import_obj(path,bytes,options);
        }});
}

} // namespace dream::assets
