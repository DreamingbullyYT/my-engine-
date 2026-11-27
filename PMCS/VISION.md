# Product Vision

## Goal
Build an Android-first game and modding platform where creators can add major gameplay systems through a stable, powerful script-first API without rebuilding the engine or host APK for ordinary script/content mods.

## Goals
- Use official, unmodified Godot 4.7.x.
- Keep Dream-specific native runtime/API code in a separate C++ GDExtension.
- Make Android ARM64 a first-class target.
- Use GDScript as the primary language for ordinary .dmod mods.
- Allow mods to introduce systems absent from the original game: enemies, combat, crafting, vehicles, quests, inventory, magic, multiplayer, and more, when host primitives exist.
- Distribute ordinary mods as .dmod packages containing GDScript, JSON, scenes, and assets.
- Keep native extensions optional for advanced trusted modders.
- Maintain durable PMCS and a generated code map.

## Deep modding
A mod can create new gameplay concepts, not merely reskin existing content. For example, it could add enemies/AI to a game with no enemies, or multiplayer to a single-player game.

This requires Godot primitives and game integration points for lifecycle, world/session access, input, saves, entity registration, events, and mod communication. A generic engine API cannot automatically provide game-specific semantics: each host game must expose the capabilities mods need.

GDScript mods run as trusted code with access to Godot APIs and game autoloads. There is intentionally no script sandbox. The documented Dream Mod API is the stable compatibility surface; direct access to internal state may break across versions.

## Non-goals and limits
- No custom Godot fork as normal extension strategy.
- No NeoForge or Minecraft Java binary/API compatibility claim.
- No promise arbitrary script/native mods are safe or sandboxed.
- No requirement for ordinary mod authors to compile native libraries or rebuild the APK.
- Lua is not part of the planned mod runtime.
- A design document or successful compile does not prove a feature is complete.
