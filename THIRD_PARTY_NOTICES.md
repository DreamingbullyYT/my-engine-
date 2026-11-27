# Third-Party Notices

Dream Engine's root project license remains MIT. The following separately licensed upstream components are bundled or used by the Android wgpu-native build. Keep their notices with source and binary distributions.

## wgpu-native

- Project: https://github.com/gfx-rs/wgpu-native
- Release: `v29.0.1.1`
- Android ARM64 asset: `wgpu-android-aarch64-release.zip`
- Release archive SHA-256: `721741f1b05a20c1738166bedf7a5efb2ba4b382da689526d3fc33de22bdd573`
- License: dual-licensed `MIT OR Apache-2.0`; copies are retained in [LICENSE.MIT](third_party/wgpu-native/LICENSE.MIT) and [LICENSE.APACHE](third_party/wgpu-native/LICENSE.APACHE).

## WebGPU native C API headers

- Project: https://github.com/webgpu-native/webgpu-headers
- Pinned submodule commit used by wgpu-native v29.0.1.1: `673658bc2bd70ec39fc55ebe6bb0173cf6d0a603`
- License: BSD 3-Clause; copy retained in [LICENSE](third_party/webgpu-headers/LICENSE).

The Android host uses wgpu-native as a prebuilt native dependency through its C API. Dream Engine's own implementation and public engine interfaces remain C++20. This file records direct package/header notices; consult the upstream projects for complete transitive dependency attribution.
