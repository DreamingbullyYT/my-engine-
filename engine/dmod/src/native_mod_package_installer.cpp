#include <dream/dmod/native_mod_package_installer.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace dream::dmod {
namespace {

#if defined(__unix__) || defined(__APPLE__)
class FileDescriptor final {
public:
    explicit FileDescriptor(int value = -1) noexcept : value_(value) {}
    ~FileDescriptor() { reset(); }
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;
    FileDescriptor(FileDescriptor&& other) noexcept : value_(other.release()) {}
    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) { reset(); value_ = other.release(); }
        return *this;
    }
    [[nodiscard]] int get() const noexcept { return value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return value_ >= 0; }
    [[nodiscard]] int release() noexcept {
        const int result = value_; value_ = -1; return result;
    }
    void reset(int next = -1) noexcept {
        if (value_ >= 0) (void)::close(value_);
        value_ = next;
    }
private:
    int value_{-1};
};

NativeModInstallResult failure(std::string message) {
    return {{}, std::move(message)};
}

FileDescriptor open_or_create_directory(int parent, const std::string& name,
                                        std::string& error) {
    if (::mkdirat(parent, name.c_str(), 0700) != 0 && errno != EEXIST) {
        error = "Cannot create native mod directory '" + name + "': " +
            std::error_code(errno, std::generic_category()).message();
        return {};
    }
    const int fd = ::openat(parent, name.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        error = "Native mod path component is not a real directory: '" + name + "'.";
        return {};
    }
    struct stat status {};
    if (::fstat(fd, &status) != 0 || !S_ISDIR(status.st_mode)) {
        (void)::close(fd);
        error = "Native mod path component is not a directory: '" + name + "'.";
        return {};
    }
    return FileDescriptor(fd);
}

std::vector<std::string> split_path(std::string_view path, bool& valid) {
    valid = !path.empty() && path.front() != '/' && path.find('\\') == std::string_view::npos;
    std::vector<std::string> parts;
    std::size_t begin = 0;
    while (valid && begin < path.size()) {
        const auto end = path.find('/', begin);
        const auto part = path.substr(begin, end == std::string_view::npos
            ? std::string_view::npos : end - begin);
        if (part.empty() || part == "." || part == ".." ||
            part.find(':') != std::string_view::npos) {
            valid = false;
            break;
        }
        parts.emplace_back(part);
        begin = end == std::string_view::npos ? path.size() : end + 1;
    }
    if (parts.empty()) valid = false;
    return parts;
}
#endif

} // namespace

NativeModInstallResult NativeModPackageInstaller::install(
    const PackageReader& package,
    const std::filesystem::path& private_mod_root) {
    const auto& manifest = package.manifest();
    if (manifest.runtime_kind != ModRuntimeKind::Native) {
        return failure("Package does not contain a native runtime module.");
    }
    const auto payload = package.read_file(manifest.native_library);
    if (!payload.success()) {
        return failure("Cannot read declared native library: " + payload.error);
    }
    if (payload.bytes.empty()) {
        return failure("Declared native library is empty.");
    }
    if (private_mod_root.empty() || !private_mod_root.is_absolute()) {
        return failure("Native mod root must be an absolute application-private directory.");
    }

#if defined(__unix__) || defined(__APPLE__)
    std::error_code filesystem_error;
    const auto root = std::filesystem::canonical(private_mod_root, filesystem_error);
    if (filesystem_error || !std::filesystem::is_directory(root, filesystem_error) ||
        filesystem_error) {
        return failure("Native mod root must already exist and be a directory.");
    }

    FileDescriptor root_fd(::open(root.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!root_fd) return failure("Cannot open the application-private native mod root.");

    std::string error;
    auto id_fd = open_or_create_directory(root_fd.get(), manifest.id, error);
    if (!id_fd) return failure(std::move(error));

    const auto version = std::to_string(manifest.version.major) + "." +
        std::to_string(manifest.version.minor) + "." +
        std::to_string(manifest.version.patch);
    auto version_fd = open_or_create_directory(id_fd.get(), version, error);
    if (!version_fd) return failure(std::move(error));

    bool path_valid = false;
    auto parts = split_path(manifest.native_library, path_valid);
    if (!path_valid) return failure("Native library path is not a safe relative path.");

    FileDescriptor parent_fd = std::move(version_fd);
    for (std::size_t index = 0; index + 1 < parts.size(); ++index) {
        auto child = open_or_create_directory(parent_fd.get(), parts[index], error);
        if (!child) return failure(std::move(error));
        parent_fd = std::move(child);
    }

    static std::atomic<std::uint64_t> temporary_sequence{0};
    const auto temporary_name = ".dmod-install." + std::to_string(::getpid()) + "." +
        std::to_string(temporary_sequence.fetch_add(1, std::memory_order_relaxed));
    const auto& final_name = parts.back();

    FileDescriptor file(::openat(parent_fd.get(), temporary_name.c_str(),
        O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (!file) {
        return failure("Cannot create temporary native library file: " +
            std::error_code(errno, std::generic_category()).message());
    }

    std::size_t written = 0;
    while (written < payload.bytes.size()) {
        const auto result = ::write(file.get(), payload.bytes.data() + written,
                                    payload.bytes.size() - written);
        if (result < 0) {
            if (errno == EINTR) continue;
            const auto message = std::error_code(errno, std::generic_category()).message();
            (void)::unlinkat(parent_fd.get(), temporary_name.c_str(), 0);
            return failure("Cannot write native library: " + message);
        }
        if (result == 0) {
            (void)::unlinkat(parent_fd.get(), temporary_name.c_str(), 0);
            return failure("Native library write returned zero bytes.");
        }
        written += static_cast<std::size_t>(result);
    }

    if (::fchmod(file.get(), 0500) != 0 || ::fsync(file.get()) != 0) {
        const auto message = std::error_code(errno, std::generic_category()).message();
        (void)::unlinkat(parent_fd.get(), temporary_name.c_str(), 0);
        return failure("Cannot finalize native library permissions/data: " + message);
    }
    file.reset();

    if (::renameat(parent_fd.get(), temporary_name.c_str(),
                   parent_fd.get(), final_name.c_str()) != 0) {
        const auto message = std::error_code(errno, std::generic_category()).message();
        (void)::unlinkat(parent_fd.get(), temporary_name.c_str(), 0);
        return failure("Cannot atomically install native library: " + message);
    }
    (void)::fsync(parent_fd.get());

    auto library_path = root / manifest.id / version;
    for (const auto& part : parts) library_path /= part;
    return {std::move(library_path), {}};
#else
    (void)payload;
    return failure("Secure native package installation is not implemented for this platform.");
#endif
}

} // namespace dream::dmod
