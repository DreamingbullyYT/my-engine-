# Build and Test Policy

## Constraints
- User has no PC development environment.
- GitHub Actions is the primary build/test environment.
- Primary Android ABI: arm64-v8a.
- User validates installable APKs on a physical phone.
- CI success is not equivalent to device success.

## Workflow requirements
- Pin action versions and external dependencies.
- Pin Godot release/tag and commit SHA.
- Build from a clean checkout in release/scheduled validation.
- Upload APKs, native libraries, test reports, and logs separately.
- Artifact names include commit SHA and ABI.
- Fail if expected output is missing or zero bytes.
- Never expose signing keys or secrets in logs/artifacts.
- Use least-privilege workflow permissions.
- Download game assets only when explicitly required and legally permitted.

## Test layers
1. Static checks: formatting, lint, manifest/schema validation.
2. Unit tests: package parser, dependency solver, registries, event dispatch, hook ordering.
3. Integration: Godot + GDExtension, runtime GDScript loading, mod lifecycle and API calls.
4. Android build: ARM64 compile/link/package.
5. Device: launch, rendering, touch, lifecycle, .dmod install/execution, networking.

## GDScript mod validation
- Verify script source can be loaded/compiled at runtime in the exact exported Godot configuration.
- Test app-private user://mods/ access and Android file-picker import.
- Test .dmod ZIP validation, path traversal rejection, resource limits, dependency ordering, and error attribution.
- Confirm mods can be installed and executed without rebuilding the APK.
- Verify disabling a mod prevents its entrypoint from executing.
- Do not claim script mods are sandboxed; the design is trusted-code execution.

## Completion evidence
Record commit SHA, workflow run ID/URL, artifact name/size, test results, user-confirmed device outcome, and known limitations. A green compile proves only that the build succeeded; it does not prove runtime correctness, API completeness, performance, or device compatibility.


## Android build trigger coverage
The M1 Android workflow must run when Godot project code, scenes, resources, native extension sources, tests, or game assets change, including files added under future modular domains such as world, terrain, colony, vehicles, physics, rendering, and infrastructure. It should not run for documentation-only PMCS edits. Keep the workflow path filters broad enough to cover new modules without requiring a manual workflow edit for every new gameplay directory.

## Branch and integration policy — 2026-11-27
- The user requires all future project work to happen directly on `main`; do not create feature branches or ordinary PRs.
- Make small commits to `main`, with focused tests and PMCS updates. Inspect latest main before each change.
- One-time transition exception: the M2 GDExtension PR already open on 2026-11-27 is to be merged after all required CI checks pass. Do not merge with pending/failing checks; do not bypass protections. If blocked, report the exact blocker.
- The user's physical Android test remains distinct from CI and must still be recorded separately.
