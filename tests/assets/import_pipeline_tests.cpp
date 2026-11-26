#include <dream/assets/import_pipeline.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace dream::assets;
using namespace dream::platform;
void require(bool ok,std::string_view message) {
    if(!ok){std::cerr<<"FAILED: "<<message<<'\n';std::exit(EXIT_FAILURE);}
}
class MemoryFileSystem final:public IFileSystem {
public:
    std::map<std::string,std::vector<std::byte>> files;
    bool read_file(std::string_view path,std::vector<std::byte>& output,
        std::size_t max_bytes,FileError& error)const override {
        const auto it=files.find(std::string(path));
        if(it==files.end()){error=FileError::NotFound;return false;}
        if(it->second.size()>max_bytes){error=FileError::TooLarge;return false;}
        output=it->second;error=FileError::None;return true;
    }
    bool write_file(std::string_view,std::span<const std::byte>,FileError& error)override{
        error=FileError::Unsupported;return false;
    }
    bool exists(std::string_view,bool&,FileError& error)const override{
        error=FileError::Unsupported;return false;
    }
    bool create_directories(std::string_view,FileError& error)override{
        error=FileError::Unsupported;return false;
    }
    bool remove_file(std::string_view,FileError& error)override{
        error=FileError::Unsupported;return false;
    }
    bool list_directory(std::string_view,std::vector<DirectoryEntry>&,
        FileError& error)const override {
        error=FileError::Unsupported;return false;
    }
};
AssetImporterOutput make_outputs(
    std::string_view,std::span<const std::byte> bytes,std::string_view options) {
    AssetImporterOutput out;
    ImportedArtifact z;
    z.output_path="cooked/zeta.asset";z.type_id=2;z.payload.assign(bytes.begin(),bytes.end());
    z.dependencies={"textures/albedo.bin"};
    ImportedArtifact a;
    a.output_path="cooked/alpha.asset";a.type_id=1;a.payload={std::byte{1}};
    if(options=="with dependency")a.dependencies={"cooked/zeta.asset"};
    out.artifacts.push_back(std::move(z));out.artifacts.push_back(std::move(a));
    return out;
}
}
int main() {
    MemoryFileSystem files;
    files.files["models/ship.TEST"]={std::byte{1},std::byte{2},std::byte{3}};
    AssetImporterRegistry registry;
    require(registry.register_importer({
        "test.importer",{".test"},&make_outputs})==AssetImporterRegistryError::None,
        "importer registers for a lowercase extension");
    require(registry.register_importer({
        "duplicate",{".TEST"},&make_outputs})==
        AssetImporterRegistryError::DuplicateExtension,
        "case-insensitive extension collisions are rejected");
    require(registry.extension_count()==1,"only the first extension remains registered");

    AssetImportPipeline pipeline(files,registry);
    auto result=pipeline.import("models/ship.TEST","with dependency");
    require(static_cast<bool>(result)&&result.artifacts.size()==2,
        "pipeline reads source and invokes registered importer");
    require(result.importer_id=="test.importer"&&result.source_hash!=0&&
        result.aggregate_hash!=0,"import metadata and deterministic hashes are generated");
    require(result.artifacts[0].output_path=="cooked/alpha.asset"&&
        result.artifacts[1].output_path=="cooked/zeta.asset",
        "artifacts are returned in deterministic path order");
    require(result.artifacts[1].content_hash!=0&&
        result.artifacts[1].dependencies.size()==1,
        "artifact payload hashes and dependency metadata are preserved");
    const auto repeat=pipeline.import("models/ship.TEST","with dependency");
    require(repeat.aggregate_hash==result.aggregate_hash,
        "same source and importer output produce the same aggregate hash");

    require(pipeline.import("../ship.test").status==AssetImportStatus::InvalidSourcePath,
        "path traversal is rejected before I/O");
    require(pipeline.import("models/missing.test").status==AssetImportStatus::SourceReadFailed,
        "missing source reports read failure");
    require(pipeline.import("models/ship.unknown").status==AssetImportStatus::NoImporter,
        "unknown extensions report no importer");

    AssetImportPipeline limited(files,registry,AssetImportLimits{
        .maximum_source_bytes=2U,.maximum_total_output_bytes=1024U});
    require(limited.import("models/ship.test").status==AssetImportStatus::SourceTooLarge,
        "source read limit is enforced before import");

    AssetImporterRegistry invalid_registry;
    require(invalid_registry.register_importer({"empty",{".bad"},{} })==
        AssetImporterRegistryError::EmptyImporter,
        "empty importer callback is rejected");
    require(invalid_registry.register_importer({"bad",{"bad"},&make_outputs})==
        AssetImporterRegistryError::InvalidExtension,
        "extension without a leading dot is rejected");

    AssetImporterRegistry malformed;
    require(malformed.register_importer({"bad.output",{".bad"},[](
        std::string_view,std::span<const std::byte>,std::string_view) {
            AssetImporterOutput out;
            ImportedArtifact artifact;
            artifact.output_path="../escape";artifact.type_id=1;
            artifact.payload={std::byte{1}};
            out.artifacts.push_back(std::move(artifact));
            return out;
        }})==AssetImporterRegistryError::None,
        "malformed output test importer registers");
    AssetImportPipeline malformed_pipeline(files,malformed);
    files.files["broken.bad"]={std::byte{9}};
    require(malformed_pipeline.import("broken.bad").status==
        AssetImportStatus::InvalidImporterOutput,
        "unsafe importer output paths are rejected");

    AssetImporterRegistry duplicate_outputs;
    require(duplicate_outputs.register_importer({"dup",{".dup"},[](
        std::string_view,std::span<const std::byte>,std::string_view) {
            AssetImporterOutput out;
            ImportedArtifact artifact;
            artifact.output_path="same.asset";artifact.type_id=1;
            artifact.payload={std::byte{1}};
            out.artifacts.push_back(artifact);out.artifacts.push_back(artifact);
            return out;
        }})==AssetImporterRegistryError::None,
        "duplicate-output test importer registers");
    files.files["duplicate.dup"]={std::byte{1}};
    AssetImportPipeline duplicate_pipeline(files,duplicate_outputs);
    require(duplicate_pipeline.import("duplicate.dup").status==
        AssetImportStatus::DuplicateOutputPath,
        "duplicate artifact paths are rejected");

    require(registry.unregister_importer("test.importer")==
        AssetImporterRegistryError::None&&registry.extension_count()==0,
        "importer can be removed cleanly");
    require(registry.unregister_importer("missing")==AssetImporterRegistryError::NotFound,
        "removing unknown importer reports not found");
    return EXIT_SUCCESS;
}
