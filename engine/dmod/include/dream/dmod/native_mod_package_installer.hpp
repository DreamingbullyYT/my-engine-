#pragma once

#include <dream/dmod/package_reader.hpp>

#include <filesystem>
#include <string>

namespace dream::dmod {

struct NativeModInstallResult final {
    std::filesystem::path library_path;
    std::string error;
    [[nodiscard]] bool success() const noexcept {
        return error.empty() && !library_path.empty();
    }
};

// Materializes the already-validated native library from a .dmod ZIP into
// an application-private directory. POSIX installs use openat/O_NOFOLLOW
// for every package-controlled path component and an atomic final rename.
class NativeModPackageInstaller final {
public:
    [[nodiscard]] static NativeModInstallResult install(
        const PackageReader& package,
        const std::filesystem::path& private_mod_root);
};

} // namespace dream::dmod
