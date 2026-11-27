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


## Colony continuity and civic autonomy — game design requirement
The colony remains the central long-term purpose of exploration. Spaceflight, mining, trade, satellites, stations, and mobile habitats must feed back into the growth, survival, and development of inhabited communities.

The mayor is not appointed as a full autonomous manager at the start and autonomy is not simply a fixed manual/assisted/autonomous menu. The intended behavior is gradual:
- Early on, the mayor acts primarily as an adviser: observes the colony, identifies issues, and recommends actions to the player.
- As the player spends longer periods away or gives a colony less direct attention, the mayor progressively takes more initiative and responsibility for routine decisions.
- The player can return, inspect the colony's history and reports, and resume direct involvement. The mayor's growing role should reduce repetitive micromanagement, not take ownership of the game away from the player.
- Autonomy is bounded by real resources, infrastructure, population, production, logistics, and time. The mayor cannot create supplies or guarantee survival.
- Colonies can still decline and be destroyed through severe shortages, disasters, isolation, conflict, or future enemy attacks. The mayor is not a plot shield or immortality mechanic.

## Space settlements beyond planets
The space layer must include more than small satellites. Distinguish:
- **Satellites/probes:** functional orbital craft for communication, observation, science, navigation, and prospecting.
- **Orbital stations:** large modular structures with docking ports, habitats, power, storage, workshops, laboratories, shipyards, and player-expandable modules.
- **Mobile colony ships / megaships:** very large vessels that can house a population and sustain a settlement while travelling or remaining in orbit. They can provide a planetless colony, with life support, food production, power, storage, industry, docking, and other modules.
- **Other space settlements:** asteroid bases, orbital habitats, industrial platforms, and shipyards can extend this system.

These are game-specific host systems and should be exposed through extensible host APIs so trusted GDScript mods can add new station modules, habitat types, ship components, population systems, and related mechanics.

## Mobile-colony fleet autonomy and controlled expansion
A mobile colony is a governed settlement with its own mayor, not just a transport ship. It may plan and build a fleet of different ship types when its technology, designs, shipyard, workforce, resources, energy, fuel, and logistics support them. The mayor chooses ships according to the settlement's actual needs; no ships or materials are created for free.

Planetary expansion is permission-gated. After roughly 5–6 star systems have been explored, the mayor may ask the player whether it may autonomously establish additional colonies. The player must approve before this autonomous colonization begins. Candidate worlds must be genuinely habitable for the civilization's current technology and biology, not merely discoverable. The Sun, Mercury, Venus's surface, and gas-giant interiors are not ordinary habitable colony targets. Specialized floating habitats (such as in Venus's upper clouds), orbital settlements, and suitable moons are distinct engineered options.

Every expedition must pass habitability, capability, supply, and logistics checks. New colonies receive their own governance and remain subject to real resource and survival constraints. The player retains visibility, permission controls, and the ability to intervene.


## Reference-guided colony and flight design (D-014)
Use The Final Earth 2 as inspiration for deeply simulated citizens, worker assignment, production graphs, housing/services, happiness, research, building upgrades, policies, and open-ended colony growth. Use Spaceflight Simulator as inspiration for modular spacecraft parts with real mass/aerodynamic properties, propulsion and propellant accounting, staging, orbital planning, atmospheric flight, docking, landing, and payload operations. Adapt these principles to the project's spherical worlds, interplanetary/interstellar economy, mobile colonies, NPC automation, and modding API. Do not copy code, assets, names, distinctive UI, or exact balance values. See the dedicated PMCS system specifications.
