# Plan de joc: colonie + rachete (titlu de lucru: TBD)

Document de planificare, versiunea 2. **Nu scriem cod până nu ești de acord cu planul.**
Fișiere însoțitoare: `MOD_API.md` (API de scripting), `TEXTURES.md` (lista de texturi).

Etichete: **[cercetat]** = din surse publice. **[de la tine]** = decis de tine. **[propunere]** = ideea mea.

## 0. Decizii luate

| Decizie | Valoare |
| --- | --- |
| Forma lumii | **Glob rotund** (planetă văzută din lateral, 2D), clădiri radiale **[de la tine]** |
| Mărimea texturilor | **32x32 pe tile**; clădirile mari = multipli (ex. 3x2 tile = 96x64 px) **[de la tine]** |
| Texturi | le faci tu; eu generez placeholdere low-poly/flat și îți dau lista completă **[de la tine]** |
| Platformă | **Doar Android**; dezvoltare și teste pe PC (headless, în sandbox-ul meu) **[de la tine]** |
| Motor | Godot 4.7, GDScript (C++ doar dacă un punct fierbinte o cere) |
| Modding | **Ca la Minecraft: scripturi, acces la absolut tot** **[de la tine]** |
| Livrare | **Tot jocul într-un singur run** **[de la tine]** |
| Dușmani | mai târziu, nu în prima versiune **[de la tine]** |
| Nume joc | nedecis |

## 1. Viziunea

Construiești o colonie pe o planetă mică și rotundă, cercetezi tehnologii, construiești rachete din piese, zbori cu fizică orbitală reală spre alte lumi, aduci resurse și fondezi colonii noi. Tot conținutul e modificabil prin scripturi și date.

Șase stâlpi: **Colonia**, **Racheta**, **Legătura** (cercetare ⇄ piese ⇄ resurse), **Frumusețea**, **Modurile**, **Mai târziu** (dușmani).

## 2. Proprietate intelectuală

Mecanicile nu sunt protejate; codul, grafica, sunetele, numele și interfața sunt. Folosim idei similare, dar **nume, artă și cod proprii** (toate numele din liste sunt originale). Nu e sfat juridic.

## 3. Referințe de design [cercetat, rezumat]

**The Final Earth 2** (city builder vertical 2D): resurse (lemn, piatră, mâncare, cunoștințe, piese, metale, cipuri, grafen, combustibil); cetățeni simulați care își găsesc joburi; **fericirea** schimbă viteza de lucru (0,25x la 0 până la 2,0x la 100, 2,5x cu festivaluri) și vine din 5 surse: casă, sens, distracție, educație, sănătate; case cu compromisuri; arbore de deblocare; îmbunătățiri pe clădire; politici (City Hall); festivaluri; facțiuni. Slăbiciuni: repetitiv, lume mică și statică.

**Spaceflight Simulator** (rachete 2D): construire din piese, fizică orbitală reală, hartă + time warp, reintrare cu încălzire, parașute, andocare, transfer de combustibil, undo și revenire. Slăbiciuni: fără scop economic, nimic nu se întoarce în bază.

## 4. Lumea pe glob [propunere, adaptat la alegerea ta]

### 4.1 Coordonate
- Poziție = **(unghi θ, altitudine h)**. Planeta-mamă are raza ~40 tile → circa 250 de sloturi pe circumferință (valoare configurabilă, mod-abilă).
- Clădirile ocupă sloturi contigue pe suprafață și se pot **stivui pe verticală** (platforme, etaje) și **săpa în jos** (adâncime limitată, ex. 12 straturi, tile-urile se îngustează spre centru).
- Sprite-ul unei clădiri e desenat drept și rotit tangent la unghiul ei. La rază ≥ 30 tile distorsiunea e mică și acceptabilă.
- **Cameră**: planeta se rotește sub cameră, cu "sus" mereu în sus pe ecran; zoom out arată globul întreg, ca în vederea de hartă.

### 4.2 Aceeași matematică pentru colonie, zbor și orbite
- Rachetele pornesc de la platforma de lansare în coordonate polare; urcarea radială devine orbită fără schimbare de scenă matematică.
- Orbite Kepler pe șine (precizie dublă în cod propriu), desen cu origine plutitoare.
- Alte planete și luni sunt globuri mai mici cu aceeași logică de colonie.

### 4.3 Zi și noapte
Soarele are un unghi; zona luminată se rotește peste glob (ferestre aprinse, umbre, fericire de noapte).

## 5. Bucla principală și progresia [propunere]

Bucla: colonie → resurse și cunoștințe → deblocări → rachetă din resurse reale → lansare → resurse și descoperiri noi → colonie mai mare.

Etape: 1) navă aterizată, lemn/piatră/mâncare; 2) case, distracție; 3) laboratoare, bibliotecă; 4) atelier + platformă; 5) rachetă mică, spațiu; 6) orbită, satelit; 7) Lună (sondă, echipaj, ruine); 8) avanpost cu gheață → apă + combustibil; 9) alte planete, rețea de colonii, transport regulat; 10) final deschis (de decis).

Adăugiri proprii: rachetă = resurse reale, misiuni și contracte, evenimente (furtuni solare, meteoriți, defecțiuni), locuitori cu nume și trăsături (astronauții sunt oameni din colonie), noduri de manevră + afișaj delta-v/TWR, mod Chill/Normal/Hard + realism de zbor separat.

### 5.1 Minerit pe asteroizi și comerț [de la tine, ideea unui prieten]

- **Tipuri de asteroizi**: de gheață (apă, combustibil), de silicați (piatră, regolit), metalici (metal rafinat, relicve rare).
- **Prospectare**: observatorul sau o sondă dezvăluie compoziția fiecărui asteroid.
- **Aterizare și extracție**: gravitație mică, deci aterizare ușoară; piese `drill` și `cargo_bay`; instalație `mining_rig` pe asteroid.
- **Două căi pentru marfă**: (a) o aduci în colonie și o prelucrezi pentru tine; (b) o **vinzi** pe piață pentru **credite**.
- **Credite**: resursă nouă, cheltuită pe piese, rachete și contracte. **Piața** (`trading_post`) are prețuri care variază în timp și după ofertă/cerere.
- **Avanpost minier**: rafinărie locală pe asteroid, fără drum înapoi (opțional).
- Totul prin registre și hook-uri, deci modurile pot adăuga tipuri de asteroizi, mărfuri și reguli de piață.

## 6. Conținut final (lista completă e în `TEXTURES.md`)

Ca să putem livra într-un run, conținutul e înghețat la:
- **13 resurse + 3 de mediu** (inclusiv credite), **13 case**, **58 clădiri** (producție, servicii, distracție, transport, unice), **51 piese de rachetă**, **10 corpuri cerești**, **~40 cercetări**, **~25 evenimente/misiuni-șablon**.
- Echilibrul (costuri, viteze) e prima țintă de corecturi după primul run.

## 7. Modding [de la tine: ca la Minecraft]

Detaliile sunt în `MOD_API.md`. Pe scurt:
- **Jocul de bază e un mod** (`base`) scris cu exact același API.
- Moduri = ZIP sau dosar cu `mod.json`, scripturi `.gd`, date, texturi, traduceri.
- Scripturi cu **acces complet** la starea jocului, registre pentru orice, evenimente (unele anulabile), **suprascrieri și învelitori** de funcții ale sistemelor interne, UI propriu, comenzi de consolă, date salvate per mod.
- **Fără sandbox**; avertisment la activare. Codul GDScript e oricum decompilabil, deci nu pretindem secretul codului.
- Texturile: un dosar `textures/` dintr-un mod are prioritate peste cele implicite, fără cod.
- Android: dosar privat `mods/` + import prin selector de fișiere. Scripturile se compilează la rulare din ZIP (fără nevoie de PCK).
- Pe PC (dev): reîncărcare la cald a modurilor.

## 8. Arhitectură tehnică (Godot 4.7)

Simularea e separată de desen; modelele nu știu de ecran; testăm fără ecran.

| Modul | Rol |
| --- | --- |
| Globul coloniei | coordonate polare, sloturi, clădiri, straturi subterane |
| Cetățeni | agenți în tablouri de date, joburi, fericire, nivel de detaliu după zoom |
| Economie | resurse, producție, lanțuri, cercetare, politici |
| Constructor rachete | piese, masă, centru de greutate, etaje, simetrie |
| Zbor | corp rigid compus, propulsie, drag, încălzire, aterizare, andocare |
| Motor orbital | corpuri, sfere de influență, Kepler pe șine, warp |
| Univers | corpuri, resurse, pericole, hartă |
| Misiuni și evenimente | contracte, recompense, evenimente aleatorii |
| UI și sunet | interfață tactilă, zone sigure, muzică |
| Salvare | JSON versionat, notează modurile folosite; mod lipsă = marcaje, nu salvare stricată |
| Modding | încărcător, registre, evenimente, suprascrieri, manager de moduri |

Puncte tehnice: precizie dublă în cod propriu pentru poziții mari; orbite = matematică proprie, nu motorul de fizică; zbor cu fizică proprie; 30/60 FPS, grafică redusă opțională, salvare automată la pauză.

## 9. Testare (în sandbox-ul meu, pe PC)

- Godot rulat **headless**: teste de logică (economie, fericire, orbite, fizica rachetei, încărcarea modurilor, salvare/încărcare).
- Simulări de mulți ani de joc rulate rapid pentru verificat stabilitatea și echilibrul grosier.
- Capturi de ecran din scene reale (dacă se poate randa în sandbox) ca verificare vizuală.
- **Nu pot testa pe telefon**: atingeri, performanță reală, notch, baterie. Asta intră în runda ta de corecturi.

## 10. Foaie de parcurs (într-un singur run, în ordinea construirii)

1. Fundație: proiect, coordonate polare, registre, încărcător de moduri, modul `base`, salvare.
2. Colonia: globul, clădiri, cetățeni, resurse, fericire, cercetare, politici.
3. Racheta: constructor, zbor, aterizare.
4. Orbite: motor orbital, hartă, warp, corpuri.
5. Legătura: platformă, costuri reale, misiuni, evenimente, avanposturi, asteroizi, piață și credite.
6. UI, sunet, texturi placeholder, tutorial, localizare (EN + RO).
7. Modding complet: manager în joc, exemple de moduri, documentație.
8. Teste, export Android, lista "ce merge / ce nu".

Dușmani, provocări zilnice și alte moduri: **mai târziu**.

## 11. Riscuri

1. **Scop uriaș într-un singur run**: probabil bug-uri și dezechilibru; planificăm o rundă de corecturi.
2. **Fără test pe device**: performanța pe telefon rămâne neverificată de mine.
3. **Globul**: camera, rotația și UI-ul pe cerc sunt mai complexe decât o fâșie; le fac, dar sunt sursa probabilă de bug-uri vizuale.
4. **Scripturi fără sandbox**: un mod rău intenționat poate face orice; avertisment clar și activare explicită.
5. **Nume joc**: de ales înainte de export.

## 12. Întrebări rămase

1. Numele jocului (poate rămâne `TBD` până la export).
2. Final deschis: refacerea Pământului sau o casă nouă? (default: sandbox fără final obligatoriu)
3. Monetizare: fără (default).
