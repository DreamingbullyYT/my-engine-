# Modding API Specification

Status: requirements/proposal only. No runtime implementation exists in the current branch.

## Accepted direction
- GDScript is the primary language for ordinary .dmod mods.
- .dmod is a ZIP package containing GDScript, JSON data, scenes, and assets; unpacked folders are supported for development.
- The base game is itself a mod and uses the same API wherever practical.
- Mods may directly access Godot APIs, game autoloads, and internal state. This is intentionally trusted-code execution, not a sandbox.
- Godot remains official and unmodified (4.7.x). Dream-specific native integration stays in a separate C++ GDExtension.
- C++ native extensions are optional advanced features. Lua is not part of the planned runtime.

## Package layout
A .dmod archive contains mod.json, optional main.gd, scripts/, data/, scenes/, textures/, sounds/, lang/, optional config.json, and optional LICENSE. The manifest defines stable id, name, semantic version, integer api_version, entry script, authors, description, dependencies/version constraints, optional dependencies, conflicts, and load-order hints.

On Android, discover mods in user://mods/ and support importing a .dmod through the system file picker into app-private storage. Validate manifest, archive paths, and resource sizes before loading. Reject path traversal and malformed/oversized entries. Never execute scripts during package validation.

## Script execution and trust
GDScript is loaded/compiled by Godot at runtime from package source. Ordinary script/data/assets mods should not require PCK or APK rebuild. This must be proven in an exported Android build before it is considered implemented.

This is not a security sandbox. Mod scripts can use Godot APIs and objects available to the project, including direct access to game autoloads and internal state. Internal fields are unstable across versions; the documented Dream Mod API is the stable compatibility contract.

The manager must display a clear trusted-code warning and require explicit user activation. Do not automatically download or activate executable mods. Attribute errors to mod ID and source file; provide disable/quarantine controls.

## Lifecycle and loading
Provisional callbacks: on_load(api), on_enable(), on_disable(), on_unload(), on_save(), on_world_ready(), and optional migration callbacks. Finalize signatures through executable tests.

Resolve dependencies and load order deterministically; load base first. Detect missing dependencies, version mismatches, conflicts, duplicate IDs, and cycles. A mod failure should be logged and isolated where possible.

## API concepts
### Events
Stable event IDs, documented/typed payloads, subscribe/unsubscribe, deterministic priority/order, cancellation semantics, and mod-attributed error handling. Support game-defined and mod-defined namespaced events.

### Registries
Namespaced IDs, duplicate detection, validation, registration phases, patch/remove/query operations, and extensible registry types. Preserve unknown data fields for compatible mod-to-mod data exchange.

### Hooks
Before, Around/Wrap, Replace/Override, and After. Stable target IDs, documented signatures, priority/order, owner ID, diagnostics, and recursion protection. Only explicitly registered targets are hookable. Multiple wrappers compose deterministically.

### Game access
Mods can directly access game autoloads and internal state by design. The stable ModAPI facade should provide versioned helpers for common operations: world/colony, resources, citizens, buildings, research, rockets, universe, markets, missions, UI, audio, assets, commands, and mod-owned save/config data.

### UI, assets, commands
Allow panels, tabs, buttons, dialogs, themes, commands, texture/sound overrides, and custom assets. Define deterministic override priority and Android touch behavior.

### Mod communication
Support optional dependencies, public service registration/lookup, namespaced events/messages, and documented contracts. Direct access to another mod's private globals is not a stable compatibility promise.

### Entirely new systems
Mods must be able to implement systems absent from the host, such as enemy AI, multiplayer, vehicles, crafting, quests, magic, or inventory. This requires usable Godot/game primitives and integration points. Multiplayer additionally requires a host-provided session, authority, and networking path; the mod API cannot invent a network transport.

### Networking
When host networking exists, define mod-owned namespaced messages, schema validation, authority, size limits, version negotiation, and entity/state sync helpers. Do not claim multiplayer support until two real clients are tested.

## Compatibility
Version Godot integration, Dream Runtime, and Dream Mod API independently. Use integer api_version or a documented manifest range, semantic versioning for public API releases, explicit migration behavior, and deprecation policy. Never silently reinterpret old fields.

## Acceptance tests
1. Install and execute a GDScript-only .dmod on Android without APK rebuild.
2. Reject malformed manifests, unsafe ZIP paths, oversized entries, and duplicate IDs.
3. Resolve dependencies and detect cycles/conflicts deterministically.
4. Register/query/patch/remove a custom registry entry.
5. Verify event subscription, cancellation, and priority order.
6. Test Before/Around/Replace/After on exposed hook targets.
7. Add enemy AI using documented game APIs.
8. Let two mods communicate through public contracts.
9. Reload a script mod with defined state/save semantics.
10. Connect two clients and sync a mod-owned entity in a host with networking.


## Colony governance and remote simulation integration (planned host API)
The host may expose a colony-governance facade to trusted mods:
- Observe colony oversight state and mayor advice/actions.
- Register mayor roles, advice providers, priorities, and policy types.
- Subscribe to recommendation, delegation, colony-report, crisis, and mayor-action events.
- Extend decision rules through documented hooks while respecting host resource/workforce constraints unless a mod intentionally overrides those rules.
- Read an auditable history of mayor decisions so the player can understand what happened while away.

The intended base-game mayor behavior is gradual: adviser first, then increasing routine responsibility as player oversight decreases. Do not model this as an obligatory mayor appointment or assume all colonies are permanently autonomous.

## Space settlement API surface (planned host API)
Expose extensible registries and runtime interfaces for:
- satellite/probe and payload definitions — functional orbital craft and scientific/communication equipment.
- orbital stations, station modules, and docking ports — large modular stations, docking, expansion, and installed systems.
- mobile colony ships, habitat modules, life support, food production, power, storage, and shipyard modules — mobile settlements and their required capabilities.
- asteroid bases, orbital habitats, and industrial platforms — additional non-planetary settlement types.

The API should let mods add modules, components, station types, and mobile habitats without modifying Godot source or rebuilding the host for ordinary GDScript content mods. A station or colony ship must only provide capabilities supported by its installed modules and available resources. Docking, population capacity, power, food, life support, and propulsion must be modeled as explicit capabilities/constraints.

Do not collapse all these entities into a cosmetic satellite category. Satellites, stations, and mobile colonies have different gameplay roles and lifecycle needs.

## Autonomous mobile-colony expansion API (planned host API)
Expose mod-extensible contracts for:
- Mobile-colony mayor fleet preferences, ship-design selection, construction requests, and shipyard/production capability checks.
- Exploration progress and the configurable expansion-permission prompt after approximately 5–6 explored star systems.
- Explicit player permission to autonomously found new planetary colonies, including inspection and revocation.
- Habitability assessment and world classification. Ordinary automatic colonization must exclude the Sun, Mercury, Venus's surface, and gas-giant interiors. Mods may add specialized settlement capabilities such as floating atmospheric habitats, orbital habitats, or moon settlements.
- Expedition feasibility: population, transport, life support, food, water, power, materials, landing/habitat systems, and logistics.
- Colony-founding events, new-colony governance initialization, and auditable reports.

Suggested registry/event surface (names provisional until implemented): ship_design, shipyard_capability, habitat_environment, habitability_profile, colony_expansion_policy, star_system_explored, expansion_permission_requested, expansion_permission_changed, habitable_world_assessed, colonization_expedition_planned, colonization_expedition_failed, and autonomous_colony_founded.

The host must not imply that a planet is habitable merely because a mod registers it. The base evaluator and any modded evaluator must expose reasons, hazards, missing capabilities, and required technology. All construction and colonization consume real resources and obey travel/physics constraints.
