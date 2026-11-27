# API de scripting (modding) — specificație v0.1

Țintă: **putere ca la Minecraft**. Un mod poate adăuga, schimba sau elimina orice. Jocul de bază (`base`) e un mod și folosește exact acest API.

## 1. Structura unui mod

```
mymod/                (dosar sau mymod.zip)
  mod.json            manifest
  main.gd             punct de intrare (opțional dacă mod.json declară alt script)
  scripts/            scripturi .gd suplimentare
  data/               definiții .json (nivel simplu, fără cod)
  textures/           PNG-uri; înlocuiesc texturile implicite după id
  sounds/             .ogg / .wav
  lang/               en.json, ro.json ...
  config.json         setări implicite ale modului (opțional)
```

### mod.json
```json
{
  "id": "mymod",
  "name": "My Mod",
  "version": "1.0.0",
  "author": "Tu",
  "api_version": "1",
  "entry": "main.gd",
  "dependencies": [{"id": "base", "min": "1.0.0"}],
  "load_after": ["othermod"],
  "load_before": [],
  "conflicts": [],
  "description": "..."
}
```

### Încărcare
- Locuri: `user://mods/` (privat pe Android) și import prin selector de fișiere (copiază în `user://mods/`).
- ZIP-urile se citesc cu `ZIPReader`; scripturile se compilează la rulare (`GDScript.new()` + `source_code` + `reload()`), deci **nu e nevoie de PCK** și nu apare problema autoload-urilor.
- Ordine: sortare topologică după `dependencies`/`load_after`/`load_before`; `base` primul. Ciclurile sunt raportate și modurile implicate dezactivate.
- Erori într-un mod: logate în `user://logs/mods.log`, modul se dezactivează, jocul continuă.
- PC (dev): comanda `mods.reload` reîncarcă la cald.

## 2. Punctul de intrare

```gdscript
extends ModBase

func on_load(api: ModAPI) -> void:
    # rulează o dată, după ce toate modurile de care depinde sunt încărcate
    api.registry.register("building", "mymod:gold_mine", {
        "name": {"en": "Gold Mine", "ro": "Mină de aur"},
        "texture": "buildings/gold_mine",    # caută textures/buildings/gold_mine.png
        "size": Vector2i(3, 2),
        "cost": {"wood": 10, "stone": 20},
        "workers": 5,
        "produces": {"gold": 0.4},
        "requires_research": "base:basic_mining",
    })
    api.events.on("day_started", _on_day)

func on_unload() -> void: pass          # curățenie la dezactivare
func _on_day(e: Event) -> void: pass
```

`ModAPI` este fațada stabilă. În plus, scripturile pot accesa **direct** orice autoload al jocului (`Game`, `World`, `Colony`, `Economy`, `Research`, `Rockets`, `Universe`, `Missions`) — fără restricții.

## 3. Registre (adaugă conținut)

`api.registry.register(kind, id, def)` / `.patch(kind, id, changes)` / `.remove(kind, id)` / `.get(kind, id)` / `.all(kind)`

Tipuri (`kind`): `resource`, `building`, `house`, `rocket_part`, `spacecraft_part`, `satellite`, `satellite_part`, `satellite_payload`, `orbital_station`, `station_module`, `docking_port`, `mobile_colony_ship`, `habitat_module`, `life_support`, `food_production`, `power_system`, `storage_module`, `shipyard_module`, `asteroid_base`, `orbital_habitat`, `industrial_platform`, `engine`, `fuel_type`, `body` (corp ceresc), `asteroid_type`, `market_good` (marfă cu preț de bază și volatilitate), `research`, `policy`, `faction`, `event`, `mission`, `job`, `trait` (trăsătură de cetățean), `mayor_role`, `colony_policy`, `autopilot`, `flight_plan`, `cargo_route`, `cargo_vehicle`, `ai_behavior`, `navigation_algorithm`, `biome`, `item_tag`, `ui_theme`, `sound`, `music`, `command`.

- ID-uri cu spațiu de nume: `base:stone_mine`, `mymod:gold_mine`. Fără prefix = al modului curent.
- `def` e un dicționar; câmpurile necunoscute sunt păstrate și accesibile (un mod poate adăuga câmpuri proprii citite de alt mod).
- Formulele pot fi lambda-uri GDScript direct în `def` (ex. `"cost_fn": func(level): return {"stone": 10 * level}`) sau expresii text în fișiere de date.
- **Patch-uri** (modifică fără a copia): `patch("building", "base:stone_mine", {"produces": {"stone": 1.0}})`. Operatori: set (implicit), `{"$add": n}`, `{"$mul": n}`, `{"$del": true}`. Conflictele între moduri sunt jurnalizate; câștigă ultimul în ordinea de încărcare.
- Fișiere în `data/*.json` cu aceeași formă fac același lucru fără cod (`{"kind": "building", "id": "...", "def": {...}}` sau `{"patch": ...}`).

## 4. Evenimente

`api.events.on(name, callable, priority := 0)` · `.off(name, callable)` · `.emit(name, data)` (moduri își pot defini evenimente proprii, prefix cu id-ul modului).

Evenimentele marcate **[cancel]** pot fi anulate (`e.cancel()`) și permit modificarea datelor (`e.data`) înainte de efect. Prioritatea mai mare rulează prima.

| Categorie | Evenimente |
| --- | --- |
| Joc | `game_started`, `game_loaded`, `game_saving`, `tick`, `hour_started`, `day_started`, `season_started`, `paused`, `resumed` |
| Construire | `building_place` [cancel], `building_built`, `building_upgrade` [cancel], `building_upgraded`, `building_demolish` [cancel], `building_demolished`, `building_work_tick`, `building_produced` |
| Cetățeni | `citizen_born`, `citizen_died` [cancel], `citizen_arrived`, `citizen_job_assigned`, `citizen_happiness_changed`, `citizen_trait_gained` |
| Economie | `resource_changed` [cancel], `resource_spent` [cancel], `production_calc` (modifici multiplicatorii) |
| Cercetare | `research_started`, `research_completed`, `policy_enacted` [cancel], `festival_started`, `festival_ended` |
| Rachete | `rocket_designed`, `rocket_part_placed` [cancel], `rocket_built`, `rocket_launch` [cancel], `rocket_stage_separated`, `rocket_docked`, `rocket_crashed`, `rocket_landed`, `rocket_reentry_heat`, `rocket_soi_changed`, `rocket_orbit_reached` |
| Univers | `body_discovered`, `outpost_founded`, `colony_founded`, `asteroid_prospected`, `asteroid_mined` |
| Guvernare | `mayor_assigned`, `mayor_policy_changed`, `colony_autonomy_changed`, `colony_report_ready`, `colony_crisis_started`, `colony_crisis_resolved` |
| Sateliți | `satellite_built`, `satellite_launched`, `satellite_orbit_changed`, `satellite_payload_activated`, `satellite_power_low`, `satellite_coverage_changed` |
| Piloți și zbor NPC | `npc_pilot_assigned`, `npc_pilot_unassigned`, `flight_plan_created`, `flight_plan_evaluated`, `autopilot_started`, `autopilot_interrupted`, `autopilot_completed`, `autopilot_failed`, `remote_link_lost`, `remote_link_restored`, `player_takeover` |
| Logistică | `cargo_route_created`, `cargo_loaded`, `cargo_unloaded`, `cargo_route_paused`, `cargo_route_completed`, `cargo_delivery_failed` |
| Comerț | `market_price_update` (modifici prețul), `cargo_sold` [cancel], `cargo_delivered` |
| Misiuni/evenimente | `mission_offered`, `mission_accepted`, `mission_completed`, `random_event_roll` [cancel], `random_event_started`, `random_event_ended` |
| UI | `ui_opened`, `ui_closed`, `tile_clicked`, `building_selected` |
| Moduri | `mod_loaded`, `mods_all_loaded`, `mod_unloaded` |

## 5. Acces la starea jocului

Obiecte de nivel înalt, toate citibile și scriibile:

- `Game.state` — rădăcina stării salvabile (dicționar/obiecte); scrii direct valori.
- `World.globe(body_id)` — sloturi, clădiri, straturi subterane; `place_building(id, angle_slot, level)`, `remove_building(ref)`, `find_buildings(filter)`.
- `Colony.citizens` — tablou de cetățeni (nume, trăsături, fericire pe cele 5 surse, job, acasă); `spawn_citizen(...)`, `kill_citizen(...)`.
- `Economy` — `get(res)`, `add(res, n)`, `set_cap(res, n)`, `multipliers`.
- `Research` — `unlock(id)`, `is_unlocked(id)`, `add_knowledge(n)`.
- `Rockets` — rachete existente, designuri salvate, `spawn_rocket(design, body, angle)`.
- `Universe` — corpuri, orbite, `add_body(def)`, `time_warp`, ora jocului.
- `Market` — `price(good)`, `sell(good, amount)`, `set_price(good, value)`, `credits` (în `Economy`).
- `Missions`, `Events` (aleatorii), `Policies`.
- `Colonies` — colonii, primari, consilii, nivelul de autonomie, politici, bugete/rezerve și rapoarte.
- `Satellites` — sateliți, piese, încărcături utile, energie, acoperire și comunicații.
- `Autopilot` — asignare pilot, evaluarea navei, executarea planului și preluarea controlului.
- `FlightPlanner` — lansare, orbită, transfer, corecții, andocare și aterizare.
- `Logistics` — rute cargo, inventare, încărcare/descărcare și livrări.
- `Fleet` și `Stations` — nave, flote și module de stație.
- `UI` — vezi §7.
- `Audio` — `play(sound_id)`, `music(track_id)`.

Orice câmp intern poate fi citit; modificarea directă a stării interne e permisă și **nu e garantată stabilă între versiuni** (doar fațada `ModAPI` e garantată).

### 5.1 Contracte de comportament pentru sisteme NPC, guvernare și automatizare

Modelul oficial pentru piloți este **BOTH**: API-ul trebuie să suporte simultan piloți-coloniști aflați la bord, piloți-coloniști care controlează de la distanță și control autonom pentru vehicule fără echipaj.

- Primarii și piloții sunt extensibili prin registre, evenimente și hook-uri; modurile pot adăuga roluri, priorități, algoritmi de navigație și comportamente AI.
- **Primarul începe ca sfătuitor**, oferind recomandări și avertismente. Pe măsură ce jucătorul petrece mai mult timp în joc departe de colonie sau îi acordă mai puțină atenție, primarul preia treptat mai multe sarcini de rutină. Nu este obligatoriu să fie numit ca administrator complet de la început și nu este un simplu comutator permanent de autonomie.
- API-ul trebuie să expună nivelul de supraveghere/atenție, recomandările, delegarea progresivă, politicile și istoricul acțiunilor primarului. Jucătorul poate reveni și interveni.
- Primarul respectă resursele, populația, forța de muncă, infrastructura, producția și logistica. Nu poate garanta supraviețuirea: coloniile pot intra în declin sau pot fi distruse de lipsuri, dezastre, conflicte și viitori inamici.
- API-ul de control al vehiculelor este generic: un vehicul de bază sau adăugat de un mod poate fi pilotat dacă expune capabilitățile necesare. Nu există o piesă universală obligatorie de autopilot.
- `Autopilot.evaluate(vehicle, plan)` întoarce `feasible`, `risky` sau `impossible`, cu motive și cerințe lipsă. Execuția folosește aceleași reguli fizice și resurse ca pentru jucător.
- Piloții la bord trebuie să fie coloniști existenți și alocați navei. Piloții la distanță necesită comunicații funcționale; vehiculele fără echipaj pot folosi control autonom.
- Simularea coloniilor îndepărtate poate agrega tick-urile, dar păstrează aceleași formule și limite. Rapoartele de revenire sunt informative și explică schimbările și deciziile primarului.

### 5.2 Contracte pentru stații și colonii mobile

- API-ul trebuie să distingă sateliți/sonde, stații orbitale modulare și nave-colonie mobile. Nu le reduce la o singură categorie decorativă de satelit.
- Registre propuse: satellite, satellite_payload, orbital_station, station_module, docking_port, mobile_colony_ship, habitat_module, life_support, food_production, power_system, storage_module, shipyard_module, asteroid_base, orbital_habitat și industrial_platform.
- Stațiile trebuie să suporte andocare și extindere prin module instalabile. Modulele pot adăuga locuințe, energie, depozitare, ateliere, laboratoare, rafinare, șantiere navale și suport vital.
- O navă-colonie poate funcționa ca așezare fără planetă numai dacă are populație și module/capabilități suficiente pentru hrană, apă, aer/suport vital, energie, depozitare și producție. Aceste funcții consumă resurse și pot eșua.
- Modurile pot adăuga noi module, componente, tipuri de stații și nave-habitat prin API-ul public.

## 6. Suprascrieri și învelitori (pentru a schimba "orice")

Sistemele interne expun puncte de intrare numite. Un mod le poate înlocui sau înconjura:

```gdscript
# înlocuiește complet calculul fericirii
api.hooks.override("happiness.compute", func(citizen): return 80.0)

# învelitoare: primești funcția originală
api.hooks.wrap("rocket.thrust", func(original: Callable, rocket, dt):
    var f = original.call(rocket, dt)
    return f * 1.5)

# înainte / după (nu schimbă fluxul)
api.hooks.before("economy.produce", func(args): ...)
api.hooks.after("economy.produce", func(args, result): ...)
```

- Toate funcțiile "mari" (producție, fericire, cost, gravitație, tracțiune, drag, încălzire, alegere job, rezultat eveniment, formulă delta-v etc.) sunt înregistrate cu nume stabile (listă în `HOOKS.md`, generată din cod).
- Mai multe suprascrieri pe același nume: ultima încărcată câștigă, `wrap` se înlănțuie.
- Pentru ceva fără hook: poți înlocui o resursă `.gd` a jocului prin `api.scripts.replace("res://core/xyz.gd", "scripts/my_xyz.gd")` (la încărcare, înainte de instanțiere).

## 7. UI

- `api.ui.add_panel(id, scene_or_callable, slot)` — slot: `side_left`, `side_right`, `top_bar`, `build_menu`, `rocket_editor`, `map_view`.
- `api.ui.add_button(slot, icon, tooltip, callable)`.
- `api.ui.add_tab(menu_id, name, scene)`.
- `api.ui.show_dialog(title, text, buttons)` → semnal cu alegerea.
- `api.ui.remove(id)`.
- Teme: registru `ui_theme`.
- Toate panourile primesc zone sigure și mărime tactilă minimă din cadru.
- Interfața modurilor trebuie să respecte principiile mobile din `PMCS/MOBILE_UX.md`: ținte tactile adecvate, informație progresivă și fără controale esențiale bazate exclusiv pe drag precis.

## 8. Comenzi de consolă

`api.commands.register("give", func(args): Economy.add(args[0], int(args[1])))` — consolă în joc (PC: tasta `~`; Android: buton). Comenzi de bază: `give`, `unlock`, `spawn`, `time`, `warp`, `tp`, `mods.reload`, `mods.list`, `god`.

## 9. Date salvate per mod

- `api.save.get(key, default)` / `api.save.set(key, value)` — intră în salvare, sub spațiul modului.
- `api.config.get(key)` — din `config.json` + setări în manager.
- La încărcarea unei salvări cu moduri lipsă: clădirile/piesele necunoscute devin marcaje; datele modului lipsă sunt păstrate și restaurate dacă modul revine.
- Eveniment `game_loaded` pentru migrări; `mod.json: version` comparată cu cea din salvare, callback opțional `on_migrate(old_version, saved)`.

## 10. Texturi și sunete

- Id de textură = cale fără extensie sub `textures/`, ex. `buildings/stone_mine`.
- Ordinea de prioritate: ultimul mod încărcat care are fișierul → mod anterior → `base`. Un dosar `textures/` dintr-un mod are deci prioritate peste cele implicite, **fără alt cod**.
- Dimensiune: multiplu de 32 (tile); dacă alta, se scalează cu filtru "nearest" și se loghează un avertisment.
- `api.assets.texture(id)`, `.sound(id)`, `.register_texture(id, image)` (generate din cod).
- Variante opționale: `<id>_night.png` (aprins noaptea), `<id>_anim.png` (bandă de cadre), `<id>_ruined.png`.

## 11. Manager de moduri în joc

Listă cu activare/dezactivare, ordine (drag), versiuni, dependențe lipsă, conflicte, jurnal de erori, profile (seturi de moduri), setări per mod, import ZIP. La activarea unui mod cu scripturi: **avertisment** că rulează cod fără restricții.

## 12. Securitate (declarată deschis)

Scripturile pot citi/scrie fișiere accesibile aplicației și pot face orice permite GDScript. Nu există sandbox. Se activează doar explicit de utilizator, pe propriul risc; nu se descarcă moduri automat.

## 13. Versionare

`api_version` întreg. Schimbările care strică compatibilitatea cresc versiunea; funcțiile depreciate se anunță cu o versiune înainte, cu avertisment în jurnal.

## 14. Exemple incluse (în pachetul final)

1. `example_building` — clădire nouă prin date.
2. `example_script` — eveniment + UI + comandă.
3. `example_override` — schimbă formula fericirii și tracțiunea.
4. `example_textures` — pachet de texturi care înlocuiește jocul de bază.
5. `example_body` — planetă nouă cu resurse proprii.

### 5.3 Contracte pentru flote autonome și colonizare cu permisiune

- O navă-colonie are propriul primar și poate planifica nave de tipuri diferite în funcție de necesități. Construirea cere proiecte/deblocări, șantier naval, forță de muncă, materiale, componente, energie, combustibil și timp.
- După aproximativ 5–6 sisteme solare explorate, primarul poate cere permisiunea jucătorului pentru colonizare autonomă. Pragul trebuie să fie configurabil pentru echilibrare.
- Colonizarea automată nu pornește fără aprobarea explicită a jucătorului; permisiunea poate fi inspectată, modificată sau revocată.
- Evaluarea locuibilității trebuie să explice condițiile și riscurile. Soarele, Mercur, suprafața lui Venus și interiorul giganților gazoși nu sunt ținte planetare obișnuite. Habitatele plutitoare atmosferice, stațiile orbitale și așezările pe luni sunt tipuri distincte de infrastructură specializată.
- Planificatorul verifică populația, transportul, suportul vital, hrana, apa, energia, materialele, aterizarea/habitatul și aprovizionarea pe termen lung.
- Evenimente/registre propuse: star_system_explored, expansion_permission_requested, expansion_permission_changed, habitable_world_assessed, colonization_expedition_planned, colonization_expedition_failed, autonomous_colony_founded, ship_design și shipyard_capability.


## Host API additions for reference-guided systems (planned)
Expose mod-extensible definitions and runtime contracts for citizen needs/traits/skills, job roles, building footprints and connection networks, production recipes, service coverage, research prerequisites, policies, factions, events, spacecraft part properties/attachment nodes, propulsion and propellant, staging actions, flight environments, maneuver plans, celestial body parameters, survey instruments, procedural generation, station module connection types, simulation fidelity tiers, save migrations, and performance diagnostics. These additions extend the registries and event surface already listed above; exact API signatures must be versioned and tested before release.
