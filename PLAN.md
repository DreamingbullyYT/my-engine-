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
| Platformă | **Android-first, ARM64**; build și teste CI în GitHub Actions, validare pe telefonul utilizatorului **[de la tine]** |
| Motor | Godot 4.7, GDScript (C++ doar dacă un punct fierbinte o cere) |
| Modding | **Ca la Minecraft: scripturi, acces la absolut tot** **[de la tine]** |
| Livrare | **Joc complet, nu prototip**; dezvoltare incrementală în etape testabile, fără a elimina sistemele agreate **[clarificare a planului]** |
| Dușmani | mai târziu, nu în prima versiune **[de la tine]** |
| Piloți NPC | **Both**: coloniști la bord, piloți la distanță și sisteme autonome pentru vehicule fără echipaj **[de la tine]** |
| Autopilot NPC | misiuni complete, cu aceeași fizică orbitală, combustibil și limite ca navele jucătorului **[de la tine]** |
| Guvernarea coloniilor | primarul începe ca sfătuitor și preia gradual responsabilități în funcție de atenția jucătorului; fără trei moduri fixe **[de la tine]** |
| Nume joc | nedecis |

## 1. Viziunea

Construiești o colonie pe o planetă mică și rotundă, cercetezi tehnologii, construiești rachete din piese, zbori cu fizică orbitală reală spre alte lumi, aduci resurse și fondezi colonii noi. Tot conținutul e modificabil prin scripturi și date.

Stâlpi: **Colonia și oamenii** (centrul experienței), **Racheta și explorarea**, **Legătura** (cercetare ⇄ piese ⇄ resurse), **Automatizarea** (primari, piloți NPC și logistică), **Sateliții și infrastructura spațială**, **Modurile**, **Mai târziu** (dușmani).

Colonizarea rămâne nucleul jocului pe tot parcursul progresiei. Explorarea spațiului trebuie să furnizeze resurse, informații, infrastructură și oportunități pentru colonii; nu înlocuiește construirea și administrarea lor. Automatizarea reduce micro-managementul, dar nu elimină populația, economia, nevoile și consecințele deciziilor.

## 2. Proprietate intelectuală

Mecanicile nu sunt protejate; codul, grafica, sunetele, numele și interfața sunt. Folosim idei similare, dar **nume, artă și cod proprii** (toate numele din liste sunt originale). Nu e sfat juridic.

## 3. Referințe de design [cercetat, rezumat]

**The Final Earth 2** (referință de design pentru simularea coloniilor; adaptare, nu copiere): resurse (lemn, piatră, mâncare, cunoștințe, piese, metale, cipuri, grafen, combustibil); cetățeni simulați care își găsesc joburi; **fericirea** schimbă viteza de lucru (0,25x la 0 până la 2,0x la 100, 2,5x cu festivaluri) și vine din 5 surse: casă, sens, distracție, educație, sănătate; case cu compromisuri; arbore de deblocare; îmbunătățiri pe clădire; politici (City Hall); festivaluri; facțiuni. Slăbiciuni: repetitiv, lume mică și statică. Valorile numerice de fericire sunt doar date de referință din jocul analizat, nu valori adoptate pentru Dream Engine.

**Spaceflight Simulator** (referință de design pentru construcția și zborul navelor; adaptare, nu copiere): construire din piese, fizică orbitală reală, hartă + time warp, reintrare cu încălzire, parașute, andocare, transfer de combustibil, undo și revenire. Slăbiciuni: fără scop economic, nimic nu se întoarce în bază.

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


### 5.2 Sateliți, stații orbitale și infrastructură spațială modulară [de la tine]

**Sateliți și sonde**
- Sateliții se construiesc din piese: structură/bus, panouri solare, baterii, antene, propulsoare, rezervoare, camere, senzori și încărcături științifice sau de navigație.
- Au orbite reale și funcționează în limitele energiei, combustibilului, orientării, comunicațiilor și echipamentelor instalate.
- Roluri: comunicații, observație, meteorologie, navigație, prospectare și cercetare. Acoperirea depinde de orbită, linia de vizibilitate, putere și echipament.

**Stații orbitale mari**
- Sunt structuri construibile și extensibile, nu sateliți decorativi: module de locuit, docuri de andocare, depozite, ateliere, laboratoare, șantiere navale, generatoare, panouri solare și sisteme de suport vital.
- Jucătorul poate andoca nave, construi/extinde stația și adăuga sau înlocui module. Modulele au funcții și limite reale.
- Stațiile pot deveni noduri pentru comerț, reparații, cercetare, transfer de combustibil, construcția navelor și logistică.

**Nave-colonie / megaships locuibile**
- Nave foarte mari, modulare, care pot găzdui populație și funcționa ca așezări mobile sau permanente, fără să fie nevoie de o planetă.
- Pentru a susține o colonie au nevoie de module reale: locuințe, suport vital, hrană/agricultură, apă, energie, depozitare, producție, reciclare, andocare și propulsie, după rolul navei.
- Pot călători între corpuri cerești sau pot rămâne într-o orbită/zonă ca habitat. Dacă sunt echipate corespunzător, reprezintă o **colonie fără planetă**, cu populație, economie și administrație proprie.
- Alte forme posibile: habitate orbitale, baze pe asteroizi, platforme industriale și șantiere navale.
- Toate aceste structuri trebuie să fie modulare și extensibile prin registre și API, astfel încât modurile să poată adăuga module și tipuri noi de habitat.

### 5.3 Piloți NPC și autopilot realist [de la tine]

- Există trei forme compatibile: **pilot-colonist la bord**, **pilot-colonist la distanță** și **control autonom** pentru sonde/vehicule fără echipaj. Decizia pentru piloții NPC este BOTH.
- Un colonist pilot trebuie să existe fizic la bordul navei. Pilotul la distanță trebuie să aibă legătură de comunicații funcțională; întreruperea comunicației poate limita sau opri comenzile la distanță, în funcție de echipamente și proceduri.
- Orice navă poate primi un pilot NPC dacă interfața sa de control este compatibilă. Nu impunem o piesă specială obligatorie de autopilot. Navele modificate pot expune o interfață de control compatibilă.
- NPC-urile pot executa misiuni complete: lansare, inserție orbitală, transferuri, corecții, andocare și aterizare.
- Se folosesc aceleași legi fizice și aceleași constrângeri ca pentru jucător: masă, tracțiune, combustibil, delta-v, orientare, control, comunicații și capabilități de andocare/aterizare. Fără teleportare sau combustibil gratuit.
- Înainte de plecare, plannerul evaluează misiunea ca **fezabilă**, **fezabilă cu risc** sau **imposibilă**, explicând lipsurile și riscurile. Un NPC poate refuza sau eșua dacă nava nu poate îndeplini misiunea.
- Jucătorul poate prelua controlul oricând. La predarea controlului către NPC, planul se recalculează din starea fizică actuală a navei.

### 5.4 Primarul și autonomia care crește treptat [de la tine]

Primarul nu începe ca administrator complet și nu trebuie numit obligatoriu de jucător. La început este în principal un **consilier al coloniei**: observă situația, oferă sfaturi, avertizează despre lipsuri și recomandă construcții sau schimbări.

- **Autonomie graduală:** când jucătorul se ocupă frecvent de colonie, primarul rămâne în principal consultativ. Dacă jucătorul petrece mult timp în joc departe de acea colonie sau nu îi mai acordă atenție, primarul începe treptat să ia inițiativa și să gestioneze mai multe activități de rutină.
- **Inițiativă progresivă:** poate începe cu ajustări mici (priorități de producție, redistribuirea muncii), apoi poate propune și executa proiecte locale, extinderi și măsuri de stabilizare, în funcție de resurse și infrastructură.
- **Controlul jucătorului:** jucătorul poate reveni oricând, vedea ce s-a întâmplat, inspecta recomandările și acțiunile primarului și poate interveni sau schimba prioritățile. Primarul nu confiscă controlul permanent.
- **Fără magie:** primarul nu creează hrană, combustibil, materiale, energie, locuri de muncă sau infrastructură. Nu poate construi dacă lipsesc resursele și nu poate depăși capacitățile reale ale coloniei.
- **Colonia poate muri:** lipsa gravă de resurse, izolarea, dezastrele, conflictele și viitorii inamici pot duce la declin, evacuare sau distrugerea coloniei. Primarul poate reduce riscul, nu garantează supraviețuirea.
- **Coloniștii rămân fundamentali:** populația, nevoile, fericirea, profesiile, sănătatea și infrastructura determină ce poate face colonia. Primarul coordonează, nu înlocuiește societatea.
- **Rapoarte de revenire:** la întoarcere, jucătorul poate vedea timpul trecut, evoluția populației, construcțiile, stocurile, crizele și explicația deciziilor primarului. Rapoartele nu trebuie să devină spam sau obligații zilnice.

### 5.5 Timpul coloniilor îndepărtate și rapoarte de revenire [propunere]

- Coloniile active pe ecran folosesc simularea detaliată. Coloniile îndepărtate folosesc simulare economică agregată, dar păstrează aceleași reguli, costuri, stocuri, populație și limite de producție.
- Timpul jocului avansează când jucătorul explorează în altă parte. Când aplicația este închisă sau suspendată, ceasul jocului se oprește implicit; nu există pierderi punitive offline. Time warp este controlat de jucător în timpul jocului.
- La revenire sau la consultarea hărții, jucătorul poate primi un raport: timp trecut în joc, schimbări de populație, clădiri, economie, evenimente și mesajul primarului.
- Rapoartele sunt informative, nu sarcini obligatorii. Fără spam repetitiv; jucătorul poate configura frecvența și tipurile de notificări.
- Exemplu de mesaj: „Nu ai mai vizitat Eos de 86 de zile. Colonia a crescut; primarul a extins cartierele și a identificat o lipsă de metal.”

### 5.6 Autonomia flotelor navelor-colonie și colonizare automată [de la tine]

- O navă-colonie/megaship are propriul primar și poate deveni o așezare mobilă administrată independent.
- Primarul poate planifica și construi tipuri diferite de nave pentru nevoile coloniei: transport, explorare, minerit, cargo, construcție sau alte roluri disponibile. Construcția este posibilă numai dacă există proiectele/deblocările, șantierul naval, forța de muncă, materialele, componentele, energia, combustibilul și timpul necesare.
- După explorarea a aproximativ **5–6 sisteme solare**, primarul poate întreba jucătorul dacă îi permite să înceapă colonizarea autonomă a altor lumi. Pragul este o țintă de progresie și echilibrare, configurabilă; nu este o permisiune implicită.
- Colonizarea autonomă începe numai după aprobarea explicită a jucătorului. Jucătorul poate inspecta rapoartele, schimba sau revoca permisiunea și poate interveni oricând.
- Sunt eligibile doar planetele evaluate ca **locuibile în mod real** pentru biologia și tehnologia curentă. Evaluarea verifică temperatura, presiunea, atmosfera, gravitația, apa, radiația, resursele și pericolele.
- Soarele nu poate fi colonizat; Mercur nu este o țintă obișnuită locuibilă; suprafața lui Venus și interiorul giganților gazoși precum Jupiter nu sunt colonii planetare obișnuite. Habitatele plutitoare în straturi atmosferice potrivite (de exemplu, norii superiori ai lui Venus), stațiile orbitale și coloniile pe luni potrivite sunt soluții inginerești separate.
- Înainte de expediție se verifică populația, navele, capacitatea de transport, suportul vital, hrana, apa, energia, materialele, aterizarea/habitatul și logistica de aprovizionare. Coloniile noi primesc propriul primar și pot eșua în continuare din motive reale.

### 5.7 Transport automat și nave cargo [propunere]

- Rute între colonii, stații, avanposturi și puncte de extracție: origine, destinație, marfă, cantitate, capacitate, încărcare/descărcare, întoarcere și repetare.
- Navele cargo pot fi pilotate de NPC-uri de la bord sau de la distanță, ori pot folosi sisteme autonome, cu aceleași constrângeri de zbor.
- Complexitate inițială medie: rute și livrări fiabile; simularea uzurii detaliate, reparațiilor și echipajelor de mentenanță rămâne pentru mai târziu.
- Lanț integrat: satelit prospectează asteroidul → instalație minieră extrage → navă cargo transportă minereul la stație → alt transport îl duce la colonie → colonia îl folosește pentru extindere și nave.

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
| Guvernare locală | primari, consilii, politici, bugete și autonomie pe colonie |
| Simulare la distanță | progres economic agregat pentru colonii și nave îndepărtate |
| Sateliți | construcție modulară, orbită, energie, comunicații și senzori |
| Planificare de zbor | misiuni NPC, evaluarea fezabilității, control manual/asistat/autonom |
| Logistică și flotă | rute cargo, încărcare/descărcare, livrări și managementul navelor |
| UI și sunet | interfață tactilă, zone sigure, muzică |
| Salvare | JSON versionat, notează modurile folosite; mod lipsă = marcaje, nu salvare stricată |
| Modding | încărcător, registre, evenimente, suprascrieri, manager de moduri |

Puncte tehnice: precizie dublă în cod propriu pentru poziții mari; orbite = matematică proprie, nu motorul de fizică; zbor cu fizică proprie; 30/60 FPS, grafică redusă opțională, salvare automată la pauză.

## 9. Testare (GitHub Actions + telefon)

- GitHub Actions: teste headless pentru economie, fericire, orbite, fizica rachetei, încărcarea modurilor și salvare/încărcare.
- GitHub Actions: simulări accelerate de perioade lungi pentru stabilitate și echilibru, plus build Android ARM64 și verificarea artefactelor.
- Utilizatorul validează APK-ul pe telefon: lansare, randare, touch, notch/zone sigure, pauză/reluare, performanță, baterie și utilizabilitatea interfeței.
- Nu presupunem existența unui PC local; orice test automat trebuie să poată rula în CI.

## 10. Foaie de parcurs (într-un singur run, în ordinea construirii)

1. Fundație: proiect, coordonate polare, registre, încărcător de moduri, modul `base`, salvare.
2. Colonia: globul, clădiri, cetățeni, resurse, fericire, cercetare, politici și primar cu autonomie graduală conform D-011.
3. Racheta: constructor, zbor, aterizare.
4. Orbite: motor orbital, hartă, warp, corpuri.
5. Legătura: platformă, costuri reale, misiuni, evenimente, avanposturi, asteroizi, piață și credite.
6. Sateliți modulari și infrastructură orbitală.
7. Planificator de zbor și piloți NPC (la bord, la distanță, autonomi), cu verificări de fezabilitate.
8. Logistică cargo și simularea coloniilor îndepărtate; rapoarte de revenire.
9. UI mobil cu complexitate progresivă, sunet, texturi placeholder, tutorial opțional, localizare (EN + RO), asistență configurabilă și verificări de accesibilitate.
10. Modding complet: manager în joc, exemple de moduri, documentație.
11. Teste, export Android, lista "ce merge / ce nu".

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


## Specificații detaliate PMCS (D-014)
Pentru regulile complete, PLAN.md se citește împreună cu documentele PMCS dedicate:
- `PMCS/GAMEPLAY.md`: cetățeni, nevoi, fericire, joburi, clădiri, economie, cercetare și politici.
- `PMCS/ROCKETS_AND_PHYSICS.md`: piese modulare, propulsie, combustibil, staging, orbite, atmosferă, andocare și aterizare.
- `PMCS/UNIVERSE.md`: stele, planete, luni, asteroizi, descoperire și călătorii interstelare.
- `PMCS/AUTOMATION.md`: primari, piloți NPC, flote, cargo și simulare la distanță.
- `PMCS/SPACE_INFRASTRUCTURE.md`: sateliți, stații, megaships și habitate.
- `PMCS/SPACE_EXPANSION.md`: extindere autonomă și locuibilitate.
- `PMCS/PERFORMANCE_AND_SIMULATION.md`: niveluri de simulare, salvări, timp și Android.

Politica de timp de bază: ceasul jocului se oprește când aplicația este închisă; time warp este controlat de jucător în timpul jocului. Aceasta evită progresia offline punitivă.


## UX mobil și complexitate
Interfața trebuie să păstreze profunzimea simulării fără a afișa toate sistemele simultan. Detaliile avansate sunt disponibile la cerere; acțiunile de bază au valori implicite și explicații clare. Automatizarea reduce micro-managementul, dar nu ascunde costurile sau riscurile. Vezi `PMCS/MOBILE_UX.md` (propunere în review, încă neacceptată ca decizie separată).
