#pragma once

#include <dream/platform/filesystem.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <exception>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace dream::assets {

enum class AssetImportStatus : std::uint8_t {
    Success,
    InvalidSourcePath,
    SourceReadFailed,
    SourceTooLarge,
    NoImporter,
    ImporterFailed,
    InvalidImporterOutput,
    DuplicateOutputPath,
    OutputTooLarge,
    TooManyOutputs,
    InvalidDependency
};

struct ImportedArtifact final {
    std::string output_path;
    std::uint64_t type_id{0};
    std::uint32_t version{1};
    std::vector<std::byte> payload;
    std::vector<std::string> dependencies;
    std::uint64_t content_hash{0};
};

struct AssetImporterOutput final {
    std::vector<ImportedArtifact> artifacts;
    std::string error;
};

using AssetImporter = std::function<AssetImporterOutput(
    std::string_view source_path,
    std::span<const std::byte> source_bytes,
    std::string_view options)>;

struct AssetImporterRegistration final {
    std::string id;
    std::vector<std::string> extensions;
    AssetImporter import;
};

enum class AssetImporterRegistryError : std::uint8_t {
    None, InvalidId, InvalidExtension, EmptyImporter, DuplicateExtension,
    NotFound
};

struct AssetImportLimits final {
    std::size_t maximum_source_bytes{64U*1024U*1024U};
    std::size_t maximum_total_output_bytes{256U*1024U*1024U};
    std::size_t maximum_outputs{10000U};
    std::size_t maximum_dependencies_per_output{10000U};
};

struct AssetImportResult final {
    AssetImportStatus status{AssetImportStatus::Success};
    platform::FileError file_error{platform::FileError::None};
    std::string source_path;
    std::string importer_id;
    std::string error;
    std::uint64_t source_hash{0};
    std::uint64_t aggregate_hash{0};
    std::vector<ImportedArtifact> artifacts;

    [[nodiscard]] explicit operator bool() const noexcept {
        return status==AssetImportStatus::Success;
    }
};

// Thread-safe extension-to-importer registry. Callbacks are copied while the
// registry lock is held and invoked after unlocking, so importers may register
// other importers or perform long-running work without blocking registry reads.
class AssetImporterRegistry final {
public:
    AssetImporterRegistry()=default;
    AssetImporterRegistry(const AssetImporterRegistry&)=delete;
    AssetImporterRegistry& operator=(const AssetImporterRegistry&)=delete;

    [[nodiscard]] AssetImporterRegistryError register_importer(
        AssetImporterRegistration registration) {
        if(registration.id.empty()||registration.id.size()>128U)
            return AssetImporterRegistryError::InvalidId;
        if(!registration.import) return AssetImporterRegistryError::EmptyImporter;
        if(registration.extensions.empty()||registration.extensions.size()>64U)
            return AssetImporterRegistryError::InvalidExtension;

        std::vector<std::string> normalized;
        normalized.reserve(registration.extensions.size());
        for(auto extension:registration.extensions) {
            if(!normalize_extension(extension))
                return AssetImporterRegistryError::InvalidExtension;
            if(std::find(normalized.begin(),normalized.end(),extension)!=normalized.end())
                return AssetImporterRegistryError::InvalidExtension;
            normalized.push_back(std::move(extension));
        }

        std::unique_lock lock(mutex_);
        for(const auto& extension:normalized)
            if(importers_.contains(extension))
                return AssetImporterRegistryError::DuplicateExtension;

        auto next=importers_;
        for(const auto& extension:normalized)
            next.emplace(extension,Entry{registration.id,registration.import});
        importers_.swap(next);
        return AssetImporterRegistryError::None;
    }

    [[nodiscard]] AssetImporterRegistryError unregister_importer(
        std::string_view id) {
        if(id.empty()) return AssetImporterRegistryError::InvalidId;
        std::unique_lock lock(mutex_);
        auto next=importers_;
        std::size_t removed=0;
        for(auto it=next.begin();it!=next.end();) {
            if(it->second.id==id) {it=next.erase(it);++removed;}
            else ++it;
        }
        if(removed==0U) return AssetImporterRegistryError::NotFound;
        importers_.swap(next);
        return AssetImporterRegistryError::None;
    }

    [[nodiscard]] std::size_t extension_count() const noexcept {
        std::shared_lock lock(mutex_);
        return importers_.size();
    }

    [[nodiscard]] bool find(
        std::string_view extension,std::string& importer_id,
        AssetImporter& importer) const {
        std::string normalized(extension);
        if(!normalize_extension(normalized)) return false;
        std::shared_lock lock(mutex_);
        const auto it=importers_.find(normalized);
        if(it==importers_.end()) return false;
        importer_id=it->second.id;
        importer=it->second.import;
        return true;
    }

    [[nodiscard]] bool find_for_path(
        std::string_view virtual_path,std::string& importer_id,
        AssetImporter& importer) const {
        if(!platform::is_valid_virtual_path(virtual_path) ||
           virtual_path.size()>4096U)
            return false;
        const auto slash=virtual_path.find_last_of('/');
        const auto dot=virtual_path.find_last_of('.');
        if(dot==std::string_view::npos ||
           (slash!=std::string_view::npos && dot<slash) ||
           dot+1U>=virtual_path.size())
            return false;
        return find(virtual_path.substr(dot),importer_id,importer);
    }

private:
    struct Entry final {
        std::string id;
        AssetImporter import;
    };

    [[nodiscard]] static bool normalize_extension(std::string& extension) noexcept {
        if(extension.size()<2U||extension.size()>16U||extension.front()!='.')
            return false;
        for(auto& character:extension) {
            const auto c=static_cast<unsigned char>(character);
            if(c>='A'&&c<='Z') character=static_cast<char>(c-'A'+'a');
            else if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'))
                return false;
        }
        return true;
    }

    mutable std::shared_mutex mutex_;
    std::map<std::string,Entry,std::less<>> importers_;
};

[[nodiscard]] inline std::uint64_t asset_import_hash(
    std::span<const std::byte> bytes,
    std::uint64_t seed=14695981039346656037ULL) noexcept {
    std::uint64_t hash=seed;
    for(const auto byte:bytes) {
        hash^=static_cast<std::uint8_t>(byte);
        hash*=1099511628211ULL;
    }
    return hash;
}

[[nodiscard]] inline std::uint64_t asset_import_hash(
    std::string_view text,std::uint64_t seed=14695981039346656037ULL) noexcept {
    std::uint64_t hash=seed;
    for(const unsigned char c:text) {
        hash^=c;
        hash*=1099511628211ULL;
    }
    return hash;
}

class AssetImportPipeline final {
public:
    explicit AssetImportPipeline(
        const platform::IFileSystem& file_system,
        const AssetImporterRegistry& registry,
        AssetImportLimits limits={}) noexcept
        :file_system_(file_system),registry_(registry),limits_(limits) {}

    [[nodiscard]] AssetImportResult import(
        std::string_view source_path,std::string_view options={}) const {
        AssetImportResult result;
        result.source_path=std::string(source_path);
        if(!platform::is_valid_virtual_path(source_path)||
           source_path.size()>4096U||options.size()>4096U||
           limits_.maximum_source_bytes==0U||
           limits_.maximum_total_output_bytes==0U||
           limits_.maximum_outputs==0U||
           limits_.maximum_dependencies_per_output==0U) {
            result.status=AssetImportStatus::InvalidSourcePath;
            result.error="Import source path, options or limits are invalid";
            return result;
        }

        const auto extension=path_extension(source_path);
        AssetImporter importer;
        if(extension.empty()||!registry_.find(extension,result.importer_id,importer)) {
            result.status=AssetImportStatus::NoImporter;
            result.error="No importer is registered for the source extension";
            return result;
        }

        std::vector<std::byte> source;
        if(!file_system_.read_file(
            source_path,source,limits_.maximum_source_bytes,result.file_error)) {
            result.status=result.file_error==platform::FileError::TooLarge
                ?AssetImportStatus::SourceTooLarge:AssetImportStatus::SourceReadFailed;
            result.error="Unable to read the bounded source asset";
            return result;
        }
        result.source_hash=asset_import_hash(
            std::span<const std::byte>{source});

        AssetImporterOutput output;
        try {
            output=importer(source_path,std::span<const std::byte>{source},options);
        } catch(const std::exception& exception) {
            result.status=AssetImportStatus::ImporterFailed;
            result.error=exception.what();
            return result;
        } catch(...) {
            result.status=AssetImportStatus::ImporterFailed;
            result.error="Importer threw an unknown exception";
            return result;
        }
        if(!output.error.empty()) {
            result.status=AssetImportStatus::ImporterFailed;
            result.error=std::move(output.error);
            return result;
        }
        if(output.artifacts.empty()||output.artifacts.size()>limits_.maximum_outputs) {
            result.status=AssetImportStatus::TooManyOutputs;
            result.error="Importer produced no outputs or exceeded the output count limit";
            return result;
        }

        std::set<std::string,std::less<>> output_paths;
        std::size_t total_bytes=0U;
        for(auto& artifact:output.artifacts) {
            if(!platform::is_valid_virtual_path(artifact.output_path)||
               artifact.output_path.size()>4096U||artifact.type_id==0U||
               artifact.version==0U||
               artifact.dependencies.size()>limits_.maximum_dependencies_per_output) {
                result.status=AssetImportStatus::InvalidImporterOutput;
                result.error="Importer returned invalid artifact metadata";
                return result;
            }
            if(!output_paths.emplace(artifact.output_path).second) {
                result.status=AssetImportStatus::DuplicateOutputPath;
                result.error="Importer returned duplicate output paths";
                return result;
            }
            if(artifact.payload.size()>limits_.maximum_total_output_bytes||
               total_bytes>limits_.maximum_total_output_bytes-artifact.payload.size()) {
                result.status=AssetImportStatus::OutputTooLarge;
                result.error="Imported artifacts exceed the aggregate output size limit";
                return result;
            }
            total_bytes+=artifact.payload.size();

            std::set<std::string,std::less<>> dependencies;
            for(const auto& dependency:artifact.dependencies) {
                if(!platform::is_valid_virtual_path(dependency)||
                   dependency.size()>4096U||
                   !dependencies.emplace(dependency).second) {
                    result.status=AssetImportStatus::InvalidDependency;
                    result.error="Importer returned an invalid or duplicate dependency path";
                    return result;
                }
            }
            artifact.content_hash=asset_import_hash(
                std::span<const std::byte>{artifact.payload});
        }

        std::sort(output.artifacts.begin(),output.artifacts.end(),
            [](const ImportedArtifact& a,const ImportedArtifact& b) {
                return a.output_path<b.output_path;
            });
        std::uint64_t aggregate=asset_import_hash(source_path,result.source_hash);
        for(const auto& artifact:output.artifacts) {
            aggregate=asset_import_hash(artifact.output_path,aggregate);
            aggregate=hash_u64(aggregate,artifact.type_id);
            aggregate=hash_u32(aggregate,artifact.version);
            aggregate=hash_u64(aggregate,artifact.content_hash);
            auto dependencies=artifact.dependencies;
            std::sort(dependencies.begin(),dependencies.end());
            for(const auto& dependency:dependencies)
                aggregate=asset_import_hash(dependency,aggregate);
        }
        result.aggregate_hash=aggregate;
        result.artifacts=std::move(output.artifacts);
        result.status=AssetImportStatus::Success;
        result.error.clear();
        return result;
    }

private:
    [[nodiscard]] static std::string path_extension(std::string_view path) {
        const auto slash=path.find_last_of('/');
        const auto dot=path.find_last_of('.');
        if(dot==std::string_view::npos||
           (slash!=std::string_view::npos&&dot<slash)||dot+1U>=path.size())
            return {};
        std::string extension(path.substr(dot));
        for(auto& c:extension) {
            const auto u=static_cast<unsigned char>(c);
            if(u>='A'&&u<='Z') c=static_cast<char>(u-'A'+'a');
        }
        return extension;
    }
    [[nodiscard]] static std::uint64_t hash_u64(
        std::uint64_t hash,std::uint64_t value) noexcept {
        for(unsigned i=0;i<8U;++i) {
            hash^=static_cast<std::uint8_t>(value&0xFFU);
            hash*=1099511628211ULL;
            value>>=8U;
        }
        return hash;
    }
    [[nodiscard]] static std::uint64_t hash_u32(
        std::uint64_t hash,std::uint32_t value) noexcept {
        for(unsigned i=0;i<4U;++i) {
            hash^=static_cast<std::uint8_t>(value&0xFFU);
            hash*=1099511628211ULL;
            value>>=8U;
        }
        return hash;
    }

    const platform::IFileSystem& file_system_;
    const AssetImporterRegistry& registry_;
    AssetImportLimits limits_;
};

} // namespace dream::assets
