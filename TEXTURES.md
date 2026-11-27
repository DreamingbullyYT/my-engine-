# Lista de texturi (v1) — pentru tine să le desenezi

- **1 tile = 32x32 px.** Clădirile și piesele mari sunt multipli (ex. 3x2 tile = 96x64 px).
- Format: PNG cu transparență, **filtru "nearest"** (fără anti-aliasing). Stil: pixel art simplu / low-poly flat.
- Calea implicită: `textures/<cale>.png`. Eu generez **placeholdere** pentru toate (culoare plată + contur + text cu id-ul). Tu înlocuiești fișierul cu același nume; **un dosar `textures/` are prioritate peste cele implicite, fără alt cod**.
- Desenezi clădirile **drepte, cu baza jos**; jocul le rotește după unghiul de pe glob.
- Opțional pe orice clădire: `<id>_night.png` (aceeași mărime, cu ferestrele aprinse). Dacă lipsește, se folosește imaginea normală.
- Spritesheet-uri: cadre egale, pe orizontală, de la stânga la dreapta.
- Total: **≈ 341 de fișiere**. Poți începe cu cele marcate ★ (esențiale pentru primele minute de joc).

---

## A. Resurse (icoane 32x32) — `textures/resources/` — 16

| Id | Descriere |
| --- | --- |
| wood ★ | buștean |
| stone ★ | bloc de piatră |
| food ★ | coș / măr |
| knowledge ★ | carte sau bec |
| machine_parts | roți dințate |
| refined_metal | lingou |
| chips | cip electronic |
| graphene | hexagoane / foaie neagră |
| rocket_fuel | butelie / picătură portocalie |
| water | picătură |
| ice | cristal de gheață |
| regolith | grămadă de praf gri |
| oxygen | bule / balon "O2" |
| energy | fulger |
| relics | fragment străin strălucitor |
| credits | monedă sau jeton |

## B. Case — `textures/buildings/` — 13

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| house_hut ★ | 2x2 | 64x64 | colibă simplă din lemn/piatră |
| house_modular ★ | 2x2 | 64x64 | casă din module prefabricate |
| house_comfort | 3x2 | 96x64 | casă confortabilă cu verandă |
| house_tower | 2x4 | 64x128 | bloc de apartamente |
| house_garden | 3x2 | 96x64 | casă comunitară cu seră pe acoperiș (produce mâncare) |
| house_party | 3x2 | 96x64 | casă colorată cu luminițe |
| house_dome | 3x2 | 96x64 | cupolă eco cu plante |
| house_pressurized | 3x2 | 96x64 | modul presurizat (spațiu), tuburi și sas |
| house_burrow | 2x2 | 64x64 | locuință subterană (intrare + cupolă mică) |
| house_villa | 4x2 | 128x64 | vilă modernă |
| house_mansion | 4x3 | 128x96 | conac mare |
| house_skypod | 2x3 | 64x96 | locuință futuristă, formă de capsulă |
| house_spooky | 3x2 | 96x64 | casă bântuită (lilieci, ferestre verzi) |

## C. Producție — `textures/buildings/` — 25

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| lumber_camp ★ | 3x2 | 96x64 | tabără de tăiat lemn, stivă de bușteni |
| quarry ★ | 3x2 | 96x64 | carieră, macara mică |
| hydro_farm ★ | 3x2 | 96x64 | fermă interioară cu rafturi verzi |
| eco_farm | 4x2 | 128x64 | fermă eco în aer liber |
| alien_farm | 4x2 | 128x64 | fermă cu plante străine violete |
| tree_plantation | 4x2 | 128x64 | plantație de arbori |
| ruin_explorer | 3x2 | 96x64 | centru de explorare ruine, antene |
| lab ★ | 3x2 | 96x64 | laborator mic, bec |
| adv_lab | 4x3 | 128x96 | laborator avansat, cupolă și antenă |
| parts_factory | 4x3 | 128x96 | fabrică de piese, coș de fum |
| metal_refinery | 4x3 | 128x96 | rafinărie de metale, cuptor |
| chip_factory | 4x3 | 128x96 | fabrică de cipuri, curat și alb |
| graphene_lab | 4x3 | 128x96 | laborator de grafen, negru-albastru |
| fuel_plant | 4x3 | 128x96 | fabrică de combustibil, rezervoare |
| super_refinery | 4x3 | 128x96 | rafinărie supraîncălzită, țevi roșii |
| ice_extractor | 3x2 | 96x64 | extractor de gheață |
| water_pump | 2x2 | 64x64 | pompă de apă |
| solar_field | 4x1 | 128x32 | rând de panouri solare |
| battery_bank | 2x2 | 64x64 | baterii mari |
| oxygen_plant | 3x2 | 96x64 | generator de oxigen |
| storehouse ★ | 3x2 | 96x64 | depozit |
| volunteers_center | 3x2 | 96x64 | centru de voluntari |
| rock_teleporter | 2x3 | 64x96 | teleporter de roci, inel |
| trading_post | 4x2 | 128x64 | piață / port de comerț, tejghea și ecran de prețuri |
| mining_rig | 3x2 | 96x64 | instalație minieră portabilă (se montează pe asteroizi), burghiu și container |

## D. Servicii — `textures/buildings/` — 4

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| school ★ | 4x2 | 128x64 | școală cu steag |
| library | 4x2 | 128x64 | bibliotecă cu coloane |
| clinic ★ | 3x2 | 96x64 | clinică, cruce |
| hospital | 4x3 | 128x96 | spital |

## E. Distracție — `textures/buildings/` — 10

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| pub ★ | 3x2 | 96x64 | pub cu felinar |
| restaurant | 3x2 | 96x64 | restaurant cu terasă |
| factory_pub | 3x2 | 96x64 | pub industrial |
| park ★ | 4x1 | 128x32 | parc cu bănci |
| botanical | 4x2 | 128x64 | grădină botanică cu sere |
| nightclub | 4x2 | 128x64 | club cu neon |
| arcade | 3x2 | 96x64 | sală de jocuri |
| holo_hall | 4x2 | 128x64 | sală holografică |
| art_museum | 4x3 | 128x96 | muzeu de artă modernă |
| aquarium | 4x3 | 128x96 | acvariu cu pești |

## F. Transport — `textures/buildings/` — 5

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| landing_pad ★ | 4x1 | 128x32 | platformă de aterizare |
| elevator | 2x5 | 64x160 | lift hiper-rapid, turn |
| teleporter | 2x3 | 64x96 | teleporter, arc luminos |
| rail_station | 3x2 | 96x64 | stație de tren |
| rail_segment | 1x1 | 32x32 | șină (se repetă) |

## G. Clădiri unice — `textures/buildings/` — 14

| Id | Tile | px | Descriere |
| --- | --- | --- | --- |
| workshop ★ | 4x3 | 128x96 | atelier mare |
| observatory | 3x4 | 96x128 | observator cu telescop |
| town_hall ★ | 4x3 | 128x96 | primărie cu ceas |
| res_mining | 4x3 | 128x96 | centru de cercetare minieră |
| res_computing | 4x3 | 128x96 | centru de cercetare calculatoare |
| res_life | 4x3 | 128x96 | centru de cercetare viață/longevitate |
| festival_hq | 4x3 | 128x96 | sediu festival, steaguri |
| monument | 3x5 | 96x160 | statuie a fericirii |
| space_museum | 4x3 | 128x96 | muzeu cu schelet de navă |
| galactic_library | 5x3 | 160x96 | bibliotecă intergalactică |
| launch_platform ★ | 6x2 | 192x64 | platformă de lansare, turn de serviciu |
| assembly_hangar ★ | 6x5 | 192x160 | hangar de asamblare rachete |
| mission_control | 4x3 | 128x96 | control misiuni, antene |
| comms_array | 3x5 | 96x160 | rețea de comunicații |

## H. Structură, teren, natură — `textures/world/` — 36

**Structură (32x32), 5:** `platform`, `pillar`, `stairs`, `shaft` (puț/lift vertical), `tunnel`.

**Teren (32x32 fiecare; trebuie să se potrivească pe lateral), 15 + 6:**

| Bioma | Fișiere |
| --- | --- |
| Temperat (planeta-mamă) ★ | `ground_temperate_top`, `ground_temperate_mid`, `ground_temperate_deep` |
| Praf (stil Marte) | `ground_dust_top`, `ground_dust_mid`, `ground_dust_deep` |
| Gheață | `ground_ice_top`, `ground_ice_mid`, `ground_ice_deep` |
| Regolit (Lună) | `ground_regolith_top`, `ground_regolith_mid`, `ground_regolith_deep` |
| Vulcanic | `ground_volcanic_top`, `ground_volcanic_mid`, `ground_volcanic_deep` |
| Minereuri (suprapuse) | `ore_metal`, `ore_ice`, `ore_crystal`, `ore_relic` |
| Alte | `water_tile`, `ruin_tile` |

**Natură, decor (px în paranteză), 10:** `tree_a` (32x64), `tree_b` (32x64), `bush` (32x32), `rock_a` (32x32), `rock_b` (32x32), `rock_c` (64x32), `grass_tuft` (32x32), `flower` (32x32), `cactus` (32x64), `ice_spike` (32x64).

## I. Cetățeni — `textures/citizens/` — 14

**Spritesheet-uri 192x32** (6 cadre de 32x32: 4 de mers + 2 de repaus), 6 fișiere: `adult_a`, `adult_b`, `adult_c`, `adult_d`, `child`, `astronaut`.
**Portrete 32x32**, 8 fișiere: `portrait_01` … `portrait_08`.

## J. Piese de rachetă — `textures/parts/` — 51

Desenate cu **vârful în sus**. Pe lățime folosim 32/64/96 px (1/2/3 tile).

**Rezervoare (7):**
| Id | px | Descriere |
| --- | --- | --- |
| tank_xs | 32x32 | mic |
| tank_s | 32x64 | |
| tank_m | 32x96 | |
| tank_l | 64x96 | |
| tank_xl | 64x160 | |
| tank_xxl | 96x192 | |
| tank_wide_s | 64x32 | lat și scund |

**Motoare (9):**
| Id | px | Descriere |
| --- | --- | --- |
| eng_spark | 32x32 | motor mic |
| eng_ember | 32x48 | motor mediu |
| eng_falcon | 64x64 | motor mare |
| eng_titan | 96x80 | motor foarte mare |
| eng_vacuum | 64x64 | duză lungă pentru vid |
| eng_ion | 32x32 | motor ionic |
| solid_s | 32x96 | rachetă solidă mică |
| solid_l | 64x160 | rachetă solidă mare |
| rcs_thruster | 16x16 | duză RCS |

**Control și protecție (8):** `capsule_crew` (64x64), `probe_core` (32x32), `hab_module` (64x64), `heat_shield_s` (32x16), `heat_shield_m` (64x16), `heat_shield_l` (96x16), `parachute_packed` (32x32), `parachute_open` (96x64).

**Aerodinamică (7):** `fairing_s` (64x96), `fairing_m` (64x128), `fairing_l` (96x160), `nose_cone_s` (32x32), `nose_cone_m` (64x48), `nose_cone_l` (96x80), `side_cone` (16x32).

**Separare, legare, andocare (9):** `decoupler_s` (32x16), `decoupler_m` (64x16), `decoupler_l` (96x16), `side_separator` (16x32), `strut` (16x16), `docking_s` (32x16), `docking_m` (64x16), `docking_l` (96x16), `fuel_line` (16x16).

**Aterizare (3):** `leg_s` (32x32), `leg_l` (64x64), `wheel` (32x32).

**Energie și echipament (8):** `solar_s` (64x16), `solar_l` (128x16), `battery_s` (32x32), `battery_l` (64x32), `antenna` (16x32), `cargo_bay` (64x64), `drill` (32x64), `science_pod` (32x32).

**Flăcări de motor (4, spritesheet 4 cadre):** `flame_small` (128x64), `flame_medium` (128x96), `flame_large` (256x128), `flame_ion` (128x64) — `textures/effects/`.

## K. Corpuri cerești, cer, hartă — `textures/space/` — 33

**Pictograme pe hartă, 64x64 (12):** `sun`, `home`, `moon`, `ember` (stâncă fierbinte), `dust` (stil Marte), `gasgiant`, `gasmoon`, `icedwarf`, `asteroid_ice` (albăstrui), `asteroid_silicate` (gri-maro), `asteroid_metal` (strălucitor), `comet`.
**Alte (21):**
| Id | px | Descriere |
| --- | --- | --- |
| ring_gasgiant | 192x64 | inele |
| sky_temperate_day / _dusk / _night | 8x256 fiecare | gradient de cer (se repetă pe orizontală) |
| sky_dust_day / _dusk / _night | 8x256 | |
| sky_space_day / _dusk / _night | 8x256 | fără atmosferă (aproape negru) |
| starfield | 256x256 | stele, se repetă |
| cloud_a, cloud_b | 96x48 | nori |
| sun_disc | 64x64 | soare pe cer |
| moon_disc | 32x32 | lună pe cer |
| marker_ship | 16x16 | rachetă pe hartă |
| marker_apoapsis | 16x16 | |
| marker_periapsis | 16x16 | |
| marker_node | 16x16 | nod de manevră |
| marker_target | 16x16 | țintă |
| marker_soi | 16x16 | |

## L. Icoane de interfață (32x32) — `textures/ui/icons/` — 67

- **Fericire (6):** `ico_home`, `ico_work`, `ico_fun`, `ico_edu`, `ico_health`, `ico_happy`
- **Timp (6):** `ico_pause`, `ico_play`, `ico_fast`, `ico_faster`, `ico_warp_up`, `ico_warp_down`
- **Meniuri (17):** `ico_market`, `ico_build`, `ico_demolish`, `ico_upgrade`, `ico_research`, `ico_rockets`, `ico_map`, `ico_settings`, `ico_mods`, `ico_save`, `ico_load`, `ico_camera`, `ico_missions`, `ico_policies`, `ico_population`, `ico_stats`, `ico_help`
- **Editor/zbor (14):** `ico_undo`, `ico_redo`, `ico_symmetry`, `ico_rotate`, `ico_stage`, `ico_fuel`, `ico_thrust`, `ico_delta_v`, `ico_altitude`, `ico_speed`, `ico_launch`, `ico_crew`, `ico_save_design`, `ico_load_design`
- **Generale (18):** `ico_close`, `ico_check`, `ico_warning`, `ico_info`, `ico_lock`, `ico_star`, `ico_plus`, `ico_minus`, `ico_arrow_up`, `ico_arrow_down`, `ico_arrow_left`, `ico_arrow_right`, `ico_trash`, `ico_zoom_in`, `ico_zoom_out`, `ico_alert_event`, `ico_search`, `ico_filter`
- **Categorii construire (6):** `cat_house`, `cat_production`, `cat_service`, `cat_fun`, `cat_transport`, `cat_unique`

## M. Cadre de interfață — `textures/ui/` — 20

Cele marcate "9-slice" au colțuri de 8 px, centrul se întinde.

| Id | px | Descriere |
| --- | --- | --- |
| panel_9 | 48x48 | panou, 9-slice |
| panel_dark_9 | 48x48 | panou închis, 9-slice |
| button_9_normal | 48x48 | 9-slice |
| button_9_pressed | 48x48 | 9-slice |
| button_9_disabled | 48x48 | 9-slice |
| tab_9_active | 48x32 | 9-slice |
| tab_9_inactive | 48x32 | 9-slice |
| tooltip_9 | 32x32 | 9-slice |
| slot_9 | 48x48 | casetă pentru obiect, 9-slice |
| slot_9_selected | 48x48 | selectată, 9-slice |
| selection_outline_9 | 32x32 | contur pe clădire selectată, 9-slice |
| bar_bg_9 | 32x8 | fundal bară progres, 9-slice |
| bar_fill_9 | 32x8 | umplere, 9-slice |
| toggle_on | 32x16 | |
| toggle_off | 32x16 | |
| slider_track | 32x8 | |
| slider_knob | 16x16 | |
| build_ghost_ok | 32x32 | peste clădirea fantomă, verde |
| build_ghost_bad | 32x32 | roșu |
| construction_scaffold | 32x32 | schelă (se repetă pe clădiri în construcție) |

## N. Efecte — `textures/effects/` — 14 (+ flăcările din J)

`particle_dot` (8x8), `particle_spark` (8x8), `particle_smoke_sheet` (64x16, 4 cadre), `explosion_sheet` (192x32, 6 cadre), `dust_puff_sheet` (96x16, 6 cadre), `heart_float` (16x16), `confetti_a` (8x8), `confetti_b` (8x8), `confetti_c` (8x8), `firework_sheet` (192x32, 6 cadre), `reentry_glow` (64x64), `rain_drop` (4x8), `snow_flake` (8x8), `meteor_trail` (32x64).

## P. Module de stație orbitală și megaship — `textures/station/` — 10

Modulele sunt piese vizuale independente, conectate prin puncte de atașare standardizate. Aceleași definiții de modul pot fi folosite în stații și, la scară mai mare, în megaships. Pentru v1 sunt necesare doar modulele de bază pentru o stație simplă; restul pot fi introduse ulterior.

| Id | px | Descriere |
| --- | --- | --- |
| station_hub | 64x64 | nod central cu porturi de conectare |
| station_habitat | 64x64 | modul presurizat pentru echipaj/locuitori |
| station_docking | 64x64 | modul cu port de andocare |
| station_cargo | 64x64 | modul de depozitare marfă |
| station_power | 64x64 | modul cu generatoare și baterii |
| station_solar | 128x32 | aripă de panouri solare |
| station_life_support | 64x64 | reciclare apă/aer și suport vital |
| station_lab | 64x64 | laborator/echipament științific |
| station_greenhouse | 64x64 | seră presurizată pentru hrană |
| station_engine | 64x64 | modul de propulsie și control orbital |

## O. Aplicație (Android) — `textures/app/` — 5

`icon_foreground` (432x432, în zona centrală de 288x288), `icon_background` (432x432), `icon_legacy` (192x192), `splash` (288x288), `logo` (256x64).

---

### Rezumat

| Secțiune | Fișiere |
| --- | --- |
| A Resurse | 16 |
| B–G Clădiri (case 13 + producție 25 + servicii 4 + distracție 10 + transport 5 + unice 14) | 71 |
| H Teren, structură, natură | 36 |
| I Cetățeni | 14 |
| J Piese de rachetă | 51 |
| J Flăcări | 4 |
| K Spațiu | 33 |
| L Icoane UI | 67 |
| M Cadre UI | 20 |
| N Efecte | 14 |
| O Aplicație | 5 |
| P Module stație/megaship | 10 |
| **Total** | **341** |

Sunete și muzică nu sunt incluse; le listez separat dacă vrei.
