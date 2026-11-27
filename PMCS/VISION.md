# Product Vision

## Goal
Build an Android-first general-purpose game runtime and modding platform where creators and modders can add major gameplay systems through a stable script-first API without rebuilding the engine or host APK for ordinary content mods.

## Goals
- Use official, unmodified Godot as the underlying engine.
- Keep Dream-specific runtime/API in a separate C++ GDExtension.
- Make Android ARM64 a first-class target.
- Support 2D and 3D game foundations.
- Allow mods to introduce systems absent from the original game: enemies, combat, crafting, vehicles, quests, inventory, magic, multiplayer, and more.
- Distribute ordinary mods as .dmod packages containing Lua, JSON, scenes, and assets.
- Keep native extensions optional for advanced trusted modders.
- Maintain durable PMCS and a generated code map so new AI sessions can orient quickly.

## Deep modding means
A mod can create new gameplay concepts, not merely reskin existing content. For example, it could add enemies/AI to a game with no enemies, or multiplayer to a game designed for single-player.

This requires both Godot primitives and Dream Runtime integration points for lifecycle, world/session access, input, saves, entity registration, events, and mod communication. A generic engine API cannot automatically provide game-specific semantics: each host game must expose the capabilities mods need.

## Non-goals and limits
- No custom Godot fork as the normal extension strategy.
- No claim of NeoForge or Minecraft Java binary/API compatibility.
- No promise that arbitrary native mods are safe or sandboxed.
- No requirement for ordinary mod authors to compile native libraries or rebuild the APK.
- A design document or successful compile does not prove a feature is complete.
