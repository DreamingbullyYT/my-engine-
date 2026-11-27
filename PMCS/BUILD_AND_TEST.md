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
3. Integration: Godot + GDExtension, Lua bindings, mod lifecycle.
4. Android build: ARM64 compile/link/package.
5. Device: launch, rendering, touch, lifecycle, mod install/execution, networking.

## Completion evidence
Record commit SHA, workflow run ID/URL, artifact name/size, test results, user-confirmed device outcome, and known limitations. A green compile proves only that the build succeeded; it does not prove runtime correctness, API completeness, performance, or device compatibility.
