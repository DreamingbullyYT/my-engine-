# Accepted Decisions

Change a decision only through a new entry explaining the reason, migration impact, and compatibility consequences.

## D-001 — Official unmodified Godot
Status: ACCEPTED. Use official Godot 4.7.x; do not maintain a custom source fork by default. Keep Dream code in a separate GDExtension. Reconsider source patches only if a required capability cannot be achieved through public extension points and alternatives are documented.

## D-002 — Separate Dream Runtime
Status: ACCEPTED. Dream runtime/API code lives outside Godot's source tree to keep upstream clean and upgrades manageable.

## D-003 — Android ARM64 first
Status: ACCEPTED. Primary ABI is arm64-v8a. CI must produce Android ARM64 artifacts before Android support is claimed.

## D-004 — GitHub Actions as primary build environment
Status: ACCEPTED. The user has no PC development environment. CI must be reproducible, produce useful logs/artifacts, and fail loudly. Physical-device validation is performed by the user.

## D-005 — Script-first .dmod (SUPERSEDED by D-010)
Historical decision: ordinary mods were planned to use Lua, JSON, scenes, and assets. Superseded by D-010 before runtime implementation. No Lua runtime is required by the accepted design.

## D-006 — NeoForge-like extensibility, not compatibility
Status: ACCEPTED. Aim for registries, events, hooks, lifecycle, dependencies, networking, and data-driven content. Do not claim compatibility with NeoForge/Minecraft Java mods.

## D-007 — Native mods are trusted code
Status: ACCEPTED. Native code can crash, corrupt memory, access files, or compromise the process. Do not promise safety or sandboxing without a tested threat model.

## D-008 — Small modular changes
Status: ACCEPTED. Prefer separate modules and focused patches. Avoid invasive upstream changes.

## D-009 — PMCS is mandatory
Status: ACCEPTED. Update project memory in the same change as meaningful architecture, API, status, or workflow changes.

## D-010 — GDScript is the primary .dmod language
Status: ACCEPTED — 2026-11-27. Ordinary .dmod mods use Godot GDScript as their primary language, alongside JSON, scenes, and assets. This resolves the conflict with the earlier Lua-first PMCS proposal and aligns PMCS with the game's MOD_API.md and PLAN.md.

Rationale:
- GDScript is native to Godot and integrates with its objects, scenes, signals, resources, and APIs.
- It avoids maintaining and binding a second scripting runtime for ordinary mods.
- It supports deep game-specific integration while retaining a documented stable Mod API.
- C++ remains available through Dream Runtime/GDExtension for native integration and performance-critical code.

Consequences:
- Remove Lua as a planned runtime dependency; do not build a Lua VM for standard mods.
- Use .dmod packages containing mod.json, GDScript, JSON, scenes, and assets.
- Mods are trusted code, not sandboxed. The manager warns and requires explicit activation.
- Direct access to game internals is permitted by design but is not a stable compatibility guarantee.
- Historical Lua experiments and CodeGraphContext Lua smoke fixtures remain historical only.
- Runtime script loading/execution must be verified on exported Android ARM64 before claiming support.


## D-011 — Gradual mayor autonomy based on player oversight
Status: ACCEPTED — 2026-11-27.
The mayor begins mainly as an adviser, recommending improvements and warning about problems. As the player spends more in-game time away from a colony or gives it less direct attention, the mayor progressively assumes more routine management. This is a gradual response to reduced oversight, not a mandatory mayor appointment or a simple permanent autonomy toggle.

The mayor must obey actual resource, workforce, infrastructure, production, and logistics constraints. The player can return and intervene. Reports should explain what the mayor recommended, what they did, and why. A colony can still fail or be destroyed; future enemies and other severe threats must remain capable of causing loss. The mayor is not an invulnerability system.

## D-012 — Modular orbital settlements and mobile colony ships
Status: ACCEPTED — 2026-11-27.
The space-settlement design includes small functional satellites/probes, large modular orbital stations, and very large mobile colony ships/megaships. Stations support docking and player-installed modules. A mobile colony ship can function as a planetless settlement if equipped with sufficient life support, power, food production, storage, population facilities, and other required modules. Asteroid bases, orbital habitats, industrial platforms, and shipyards may extend this family.

Rationale: space infrastructure should support more than observation satellites and should allow civilization to exist and expand without requiring every settlement to be founded on a planet.

Consequences:
- Keep satellites, stations, and mobile colony ships as distinct entity classes or capability profiles, not one decorative satellite type.
- Make stations and colony ships modular and extensible through the game host and mod API.
- Their survival and operation depend on real installed systems, resources, and logistics; no free power, supplies, or guaranteed safety.
- These are design requirements only; no implementation is implied.
