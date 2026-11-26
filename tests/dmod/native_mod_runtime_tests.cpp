#include <dream/dmod/native_mod_host.hpp>
#include <dream/dmod/native_mod_manager.hpp>
#include <cstdint>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
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
    require(manager.valid_host_api(), "host ABI valid");

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
    require(static_cast<bool>(manager.disable("test.native")), "disable mod");
    require(counter == 1110 && !manager.is_enabled("test.native"), "disable callback");
    require(static_cast<bool>(manager.unload("test.native")), "unload module");
    require(counter == 11110 && manager.loaded_count() == 0, "destroy before dlclose");
    require(!manager.enable("missing.mod"), "reject unknown module");
    manager.shutdown();
    require(!manager.update(0.01), "reject updates after shutdown");
    std::cout << "Dream Engine native DMod runtime tests passed." << std::endl;
}
