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

Tipuri (`kind`): `resource`, `building`, `house`, `rocket_part`, `body` (corp ceresc), `asteroid_type`, `market_good` (marfă cu preț de bază și volatilitate), `research`, `policy`, `faction`, `event`, `mission`, `job`, `trait` (trăsătură de cetățean), `biome`, `item_tag`, `ui_theme`, `sound`, `music`, `command`.

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
- `UI` — vezi §7.
- `Audio` — `play(sound_id)`, `music(track_id)`.

Orice câmp intern poate fi citit; modificarea directă a stării interne e permisă și **nu e garantată stabilă între versiuni** (doar fațada `ModAPI` e garantată).

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
