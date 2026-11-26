#include <dream/dmod/native_mod_host.hpp>
#include <dream/dmod/native_mod_manager.hpp>
#include <dream/dmod/package_reader.hpp>
#include <miniz.h>
#include <cstdint>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <string>
#include <vector>
#include <utility>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstring>
#ifndef DREAM_TEST_NATIVE_MOD_PATH
#define DREAM_TEST_NATIVE_MOD_PATH ""
#endif
namespace {
void require(bool ok, std::string_view what) {
    if (!ok) { std::cerr << "FAILED: " << what << std::endl; std::exit(EXIT_FAILURE); }
}
}
int main() {
    using namespace dream::dmod;
    NativeModHost host;
    std::int32_t counter = 0;
    require(host.expose("dream.test.counter", 1, counter), "expose host object");
    NativeModManager manager(host.api());
    bool unload_on_update = false;
    require(host.expose("dream.test.manager", 1, manager), "expose manager to trusted native mod");
    require(host.expose("dream.test.unload_on_update", 1, unload_on_update),
            "expose native mod self-unload switch");
    require(manager.valid_host_api(), "host ABI valid");

    struct InterfaceSnapshot final {
        std::vector<std::pair<std::string, std::uint32_t>> entries;
    } interface_snapshot;
    const auto collect_interface = [](
        void* context, const char* id, std::uint32_t version) noexcept -> std::uint8_t {
        if (!context || !id) return 0;
        auto& snapshot = *static_cast<InterfaceSnapshot*>(context);
        try {
            snapshot.entries.emplace_back(id, version);
            return 1;
        } catch (...) {
            return 0;
        }
    };
    const auto host_api = host.api();
    require(host_api.enumerate_interfaces != nullptr, "native host exposes interface enumeration");
    require(host_api.enumerate_interfaces(
        host_api.host_context, collect_interface, &interface_snapshot) != 0,
        "enumerate native host interfaces");
    require(interface_snapshot.entries.size() == 3 &&
            interface_snapshot.entries[0].first == "dream.test.counter" &&
            interface_snapshot.entries[1].first == "dream.test.manager" &&
            interface_snapshot.entries[2].first == "dream.test.unload_on_update" &&
            interface_snapshot.entries[0].second == 1,
            "interfaces are enumerated in deterministic ID order with versions");

    Manifest manifest;
    manifest.id = "test.native";
    manifest.name = "Native Test Mod";
    manifest.version = {1, 2, 3};
    manifest.engine_api.minimum_inclusive = {0, 1, 0};
    manifest.runtime_kind = ModRuntimeKind::Native;
    manifest.native_library = "lib/android-arm64-v8a/libtest.so";
    require(validate_manifest(manifest).valid(), "native manifest validates");

    Manifest dependent = manifest;
    dependent.id = "test.native.dependent";
    dependent.name = "Dependent Native Test Mod";
    dependent.dependencies.push_back({"test.native", {{1, 0, 0}, std::nullopt}, false});
    const std::array<NativeModPackage, 2> invalid_batch{{
        {manifest, DREAM_TEST_NATIVE_MOD_PATH},
        {dependent, "/definitely/missing/dream-test-native.so"}
    }};
    const auto rolled_back = manager.load_all(invalid_batch, {0, 1, 0});
    require(!rolled_back && manager.loaded_count() == 0,
            "failed batch unloads all modules loaded earlier in dependency order");
    require(counter == 10001, "failed batch destroys the already-created dependency");

    counter = 0;
    const std::array<NativeModPackage, 1> valid_batch{{
        {manifest, DREAM_TEST_NATIVE_MOD_PATH}
    }};
    const auto batch = manager.load_all(valid_batch, {0, 1, 0});
    require(static_cast<bool>(batch) && batch.load_order.size() == 1 &&
            batch.load_order.front() == "test.native",
            "batch load returns deterministic dependency order");
    require(manager.is_loaded("test.native") && manager.loaded_count() == 1,
            "track loaded native mod");
    require(manager.is_loaded("test.native") && manager.loaded_count() == 1, "track loaded mod");
    require(counter == 1, "mod receives a raw host interface");
    const auto enabled_batch = manager.enable_all();
    require(static_cast<bool>(enabled_batch) && manager.is_enabled("test.native"),
            "enable all runs modules in dependency order");
    require(counter == 10 && manager.is_enabled("test.native"), "enable callback");
    require(manager.update(0.25), "run update");
    require(counter == 110, "update callback");
    require(!manager.update(-1.0), "reject negative delta");
    unload_on_update = true;
    require(manager.update(0.25), "run update which requests self-unload");
    require(!manager.is_loaded("test.native") && manager.loaded_count() == 0,
            "self-unload is deferred until the native callback returns");
    require(counter == 11211,
            "deferred self-unload calls disable and destroy after update returns");
    require(!manager.enable("missing.mod"), "reject unknown module");
    manager.shutdown();
    require(!manager.update(0.01), "reject updates after shutdown");

#if defined(__unix__) || defined(__APPLE__)
    std::ifstream plugin_file(DREAM_TEST_NATIVE_MOD_PATH, std::ios::binary);
    require(static_cast<bool>(plugin_file), "read the compiled native test module");
    const std::string plugin_bytes(
        (std::istreambuf_iterator<char>(plugin_file)), std::istreambuf_iterator<char>());
    require(!plugin_bytes.empty(), "native test module has payload bytes");

    const std::string package_manifest =
        "format_version = 1\n"
        "runtime = \"native\"\n"
        "id = \"test.native\"\n"
        "name = \"Native Test Mod\"\n"
        "version = \"1.2.3\"\n"
        "engine_api_min = \"0.1.0\"\n"
        "native_library = \"lib/linux-x86_64/libtest.so\"\n";

    mz_zip_archive writer{};
    require(mz_zip_writer_init_heap(&writer, 0, 0) != 0,
            "create an in-memory native .dmod package");
    require(mz_zip_writer_add_mem(&writer, "manifest.toml",
        package_manifest.data(), package_manifest.size(), MZ_DEFAULT_COMPRESSION) != 0,
        "add native manifest to .dmod");
    require(mz_zip_writer_add_mem(&writer, "lib/linux-x86_64/libtest.so",
        plugin_bytes.data(), plugin_bytes.size(), MZ_DEFAULT_COMPRESSION) != 0,
        "add native library payload to .dmod");
    void* package_memory = nullptr;
    std::size_t package_size = 0;
    require(mz_zip_writer_finalize_heap_archive(&writer, &package_memory, &package_size) != 0,
            "finalize native .dmod archive");
    require(mz_zip_writer_end(&writer) != 0, "finish native .dmod writer");
    std::vector<std::byte> package_bytes(package_size);
    std::memcpy(package_bytes.data(), package_memory, package_size);
    mz_free(package_memory);

    std::vector<std::string> package_errors;
    auto package = PackageReader::open(package_bytes, package_errors);
    require(package != nullptr && package_errors.empty(),
            "parse native .dmod package from its archive");
    const auto install_root = std::filesystem::temp_directory_path() /
        "dream-native-mod-end-to-end";
    std::filesystem::remove_all(install_root);
    std::filesystem::create_directories(install_root);

    NativeModHost installed_host;
    std::int32_t installed_counter = 0;
    bool installed_unload_request = false;
    require(installed_host.expose("dream.test.counter", 1, installed_counter),
            "expose package test counter");
    NativeModManager installed_manager(installed_host.api());
    require(installed_host.expose("dream.test.manager", 1, installed_manager),
            "expose package test manager");
    require(installed_host.expose("dream.test.unload_on_update", 1, installed_unload_request),
            "expose package test unload flag");
    const auto package_load = installed_manager.install_and_load(*package, install_root);
    require(static_cast<bool>(package_load),
            "install library from .dmod and load it without rebuilding engine");
    require(installed_counter == 1 && installed_manager.is_loaded("test.native"),
            "installed native module can reach live engine host interface");
    require(static_cast<bool>(installed_manager.enable("test.native")),
            "enable installed .dmod module");
    require(installed_manager.update(0.016), "update installed .dmod module");
    require(static_cast<bool>(installed_manager.unload("test.native")),
            "unload installed .dmod module");
    require(installed_counter == 11110,
            "installed package executes create, enable, update, disable and destroy lifecycle");
    installed_manager.shutdown();
    std::filesystem::remove_all(install_root);
#endif




    std::cout << "Dream Engine native DMod runtime tests passed." << std::endl;
}
