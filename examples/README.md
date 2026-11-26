# Headless sample games

These are small, executable integration projects for the engine API. They deliberately use a headless render backend so the Linux CI can run the exact same application loop without a GPU or display server. They validate runtime startup/shutdown, fixed-step simulation, physics, frame construction, and the retained UI-to-render bridge.

- `2d_platformer`: dynamic player circle landing on a static floor; sprite and UI panel are submitted every frame.
- `3d_physics_lab`: dynamic 3D sphere, static floor, registered triangle mesh/material, 3D and 2D cameras, plus a retained UI panel.

Build and run with:

```sh
cmake -S . -B build -DDREAM_BUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The headless backend checks command submission, not GPU pixels. For visual validation, these sample scenes still need to be attached to a real platform host and a configured graphics backend (and compatible shader binaries).
