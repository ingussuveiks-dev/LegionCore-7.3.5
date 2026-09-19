# Server.log kļūdu labošanas progress

Sākuma stāvoklis pēc `DBErrors.log` pabeigšanas: 116 `ERROR` ieraksti pilnā `worldserver` startā. No tiem 115 bija spell skriptu validācijas kļūdas un viens — Arena Season konfigurācijas kļūda.

## Pakete 167 — Druid Incarnation aura hooki

Fails: `src/server/scripts/Spells/spell_druid.cpp`.

Kopīgais `spell_dru_incarnation` skripts ir piesaistīts četriem specializāciju spelliem, kuru effect 0 izmanto četrus dažādus aura tipus. Iepriekš skripts katram spellam reģistrēja visus četrus tipus, tādēļ tikai viens pāris strādāja, bet pārējie seši hooki katram spellam radīja validācijas kļūdas. Apply un remove darbības tagad vienreiz piesaistītas faktiskajam effect 0 ar `SPELL_AURA_ANY`; esošā spell ID izvēle un formas uzlikšanas loģika nav mainīta.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika divi iepriekš eksistējoši, ar šo labojumu nesaistīti C5055 brīdinājumi tajā pašā failā.
- Pilns starts pabeigts 11 sekundēs; visas 24 `spell_dru_incarnation` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 116 uz 92.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Restoration, Feral, Guardian un Balance specializāciju aktivizēt attiecīgo Incarnation spēju un pārbaudīt pareizo formu/modeli gan uzlikšanas, gan auras noņemšanas brīdī.

## Pakete 168 — Desolate Host Spiritual Barrier hooki

Fails: `src/server/scripts/Legion/TombOfSargeras/boss_TheDesolateHost.cpp`.

`spell_tos_spiritual_barrier_dissonance` apkalpo četrus spellus ar divām atšķirīgām DBC uzbūvēm: vizuālajiem variantiem kontrolējošā aura ir effect 1, bet realm variantiem — effect 0. Iepriekš katram spellam tika reģistrēti visi phase, dummy un periodic-dummy varianti. Apply, remove un update loģika tagad katram spellam vienreiz piesaistīta pirmajam faktiskajam aura efektam ar `EFFECT_FIRST_FOUND` un `SPELL_AURA_ANY`.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; visas 16 `spell_tos_spiritual_barrier_dissonance` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 92 uz 76.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Desolate Host cīņā abās realm pusēs pārbaudīt Spiritual Barrier uzlikšanu un noņemšanu, spēlētāja selectability, custom-field atjaunošanu un heroic/mythic Dissonance periodisko castu.

## Pakete 169 — General Nazgrim stance noņemšanas hooks

Fails: `src/server/scripts/Pandaria/SiegeofOrgrimmar/boss_general_nazgrim.cpp`.

Trīs General Nazgrim stance spelli izmanto atšķirīgus effect 0 aura tipus, bet tiem ir viena un tā pati noņemšanas darbība — pāriet uz nākamo stance, ja aura nav noņemta nāves dēļ. Trīs savstarpēji izslēdzošie hooki aizstāti ar vienu effect 0 `SPELL_AURA_ANY` hooku.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; visas sešas `spell_generic_stance` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 76 uz 70.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- General Nazgrim cīņā sagaidīt Battle, Berserker un Defensive stance nomaiņas un pārbaudīt, ka katras auras beigās AI tieši vienreiz izvēlas nākamo stance; nāves brīdī pāreja nedrīkst notikt.

## Pakete 170 — Sisters of the Moon Eclipse absorb hooki

Fails: `src/server/scripts/Legion/TombOfSargeras/boss_SistersoftheMoon.cpp`.

Embrace of the Eclipse naidīgais variants ir heal absorb, bet draudzīgais variants ir damage absorb. Abiem ir viena effect 0 uzkrātā apjoma un detonācijas loģika. Katram spellam reģistrētie abi savstarpēji izslēdzošie aura tipi aizstāti ar vienu `SPELL_AURA_ANY` hooku amount aprēķinam, absorb notikumam un noņemšanai.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; visas sešas `spell_sistersmoon_embrace_eclipse` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 70 uz 64.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Sisters of the Moon cīņā pārbaudīt abus Embrace of the Eclipse variantus: heal absorb un damage absorb uzkrāj pareizo daudzumu un tikai pēc dabiskas auras beigšanās izsauc attiecīgi Umbra vai Lunar Detonation.

## Pakete 171 — ieroča modeļa kopēšanas aura hooki

Faili: `src/server/scripts/Spells/spell_generic.cpp` un `src/server/scripts/Kalimdor/LostCityOfTheTolvir/boss_high_prophet_barim.cpp`.

Kopīgais clone-weapon skripts ir piesaistīts sešiem spelliem, bet spell 41054 un 69893 effect 0 šajā klienta būvē vairs nav `SPELL_AURA_PERIODIC_DUMMY`. Arī Lost City of the Tol'vir lokālais melee-weapon variants izmanto spell 69893. Abos skriptos apply/remove dzīves cikls attiecas uz effect 0 neatkarīgi no aura tipa, tādēļ hooki mainīti uz `SPELL_AURA_ANY`; ieroča saglabāšanas un atjaunošanas loģika nav mainīta.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika četri iepriekš eksistējoši, ar šo labojumu nesaistīti C5055 brīdinājumi `spell_generic.cpp`.
- Pilns starts pabeigts 11 sekundēs; visas desmit abu ieroča kopēšanas skriptu validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 64 uz 54.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Lost City of the Tol'vir Repentance fāzē pārbaudīt, ka spoguļattēls saņem spēlētāja galvenā ieroča modeli un pēc auras beigām atgūst sākotnējo modeli.
- Atsevišķi pārbaudīt generic galvenā, off-hand un ranged ieroča kopēšanas spellus, īpaši 41054 un 69893, ieskaitot auras atkārtotu uzlikšanu.

## Pakete 172 — Brewfest ram ātruma aura hooki

Fails: `src/server/scripts/Spells/spell_generic.cpp`.

Brewfest Trot, Canter un Gallop spelli saglabā veco trīs efektu izkārtojumu, bet Normal un Exhausted šajā klienta būvē vairs neatbilst fiksētajiem aura tipiem. Apply un remove ir visas auras dzīves cikla notikumi, tādēļ tie piesaistīti pirmajam faktiskajam aura efektam. Periodiskais handlers paliek effect 1, bet pieņem tā faktisko aura tipu; tas jau pēc spell ID nekavējoties ignorē Exhausted variantu.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika četri iepriekš eksistējoši, ar šo labojumu nesaistīti C5055 brīdinājumi tajā pašā failā.
- Pilns starts pabeigts 12 sekundēs; visas četras `spell_brewfest_speed` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 54 uz 50.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Brewfest ram braucienā pārbaudīt Normal → Trot → Canter → Gallop pārejas, fatigue pieaugumu/samazinājumu, Exhausted uzlikšanu pie 100 stackiem un 15 stacku noņemšanu pēc Exhausted beigām.

## Pakete 173 — Skovald jaukto aura tipu nodalīšana

Faili: `src/server/scripts/Legion/HallsofValor/boss_god_king_skovald.cpp` un `sql/updates/world/2026_09_19_149_split_skovald_infernal_flames_absorb.sql`.

`spell_skovald_aegis_remove` ir piesaistīts diviem spelliem ar atšķirīgu effect 1: Aegis Override (193783) izmanto DUMMY, bet Infernal Flames Aura (193983) izmanto SCHOOL_ABSORB. Kopīgais auras beigu hooks tagad pieņem abu faktisko tipu un turpina castot 193991. Amount un absorb loģika izdalīta jaunā `spell_skovald_infernal_flames_absorb`, kuru SQL piesaista tikai spellam 193983. Atsevišķais Aegis of Aggramar absorb spells 193743 un tā skripts nav mainīts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`; spellam 193983 datubāzē ir gan kopīgais remove, gan jaunais absorb skripts.
- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; visas četras Skovald validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 50 uz 46.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- God-King Skovald cīņā pārbaudīt Aegis of Aggramar frontālo absorb (spell 193743), Infernal Flames (193983) virziena/distances absorb noteikumus un to, ka gan 193983, gan Aegis Override (193783) beigās casto spellu 193991.

## Pakete 174 — Monk aura dzīves cikla hooki

Fails: `src/server/scripts/Spells/spell_monk.cpp`.

Storm, Earth, and Fire pamata auras noņemšanas darbībai nav nepieciešams konkrētais effect 0 aura tips, tādēļ tā piesaistīta faktiskajam tipam. Trīs clone-visual spelli un Hurricane Strike šajā klienta būvē vairs neizmanto skriptā fiksētos aura tipus/indeksus; to darbības attiecas uz visas auras dzīves ciklu un tagad vienreiz piesaistās pirmajam faktiskajam aura efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika divi iepriekš eksistējoši, ar šo labojumu nesaistīti C5055 brīdinājumi tajā pašā failā.
- Pilns starts pabeigts 11 sekundēs; visas piecas šīs Monk grupas validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 46 uz 41.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Aktivizēt un atcelt Storm, Earth, and Fire, pārbaudot abu klonu vizuālos efektus, summon slotus un korektu atgriešanos mājās pēc auras noņemšanas.
- Izmantot Hurricane Strike un pārbaudīt sākotnējo un turpmāko 158221 castu aptuveni ik pēc 140 ms visā auras laikā.

## Pakete 175 — Maiden of Vigilance infusion periodiskais hooks

Fails: `src/server/scripts/Legion/TombOfSargeras/boss_MaidenofVigilance.cpp`.

Infusion spelli 235213 un 235240 izmanto vienu un to pašu effect 0 periodisko darbību, bet to faktiskie aura tipi ir savstarpēji atšķirīgi. Divu alternatīvu reģistrāciju vietā skripts tagad vienreiz piesaistās faktiskajam periodiskajam aura efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; abas `spell_tos_infusions` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 41 uz 39.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Maiden of Vigilance cīņā pārbaudīt gan Fel Infusion (235240), gan Light Infusion (235213) pirmo periodisko ticku un turpmāko pretējās infusion auras mijiedarbību.

## Pakete 176 — Eye of Azshara Roiling Storm periodiskais hooks

Fails: `src/server/scripts/Legion/EyeOfAzshara/boss_serpentrix.cpp`.

Roiling Storm spelli 196290 un 196296 izmanto vienu effect 0 periodisko apstrādi, bet vienam faktiskā aura ir `PERIODIC_TRIGGER_SPELL`, otram — `PERIODIC_DUMMY`. Divu alternatīvu reģistrāciju vietā skripts tagad vienreiz piesaistās faktiskajam periodiskajam aura efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; abas `spell_eye_of_azshara_roiling_storm` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 39 uz 37.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Eye of Azshara pārbaudīt gan 196290, gan 196296 Roiling Storm darbību: NPC izsaukšanu, kustību pa apli un periodisko efektu katrā tickā.

## Pakete 177 — Mercenary Contract auras noņemšana

Fails: `src/server/scripts/Spells/spell_generic.cpp`.

Mercenary Contract spelli 193472 un 193475 effect 0 ir `DUMMY` aura. Apply hooks jau bija piesaistīts šim faktiskajam tipam, bet remove hooks kļūdaini gaidīja `TRANSFORM`; tagad abi auras dzīves cikla hooki izmanto vienu un to pašu effect 0 tipu.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika četri iepriekš eksistējoši, ar šo labojumu nesaistīti C5055 brīdinājumi tajā pašā failā.
- Pilns starts pabeigts 11 sekundēs; abas `spell_mercenary_contract` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 37 uz 35.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Battleground vidē ar ieslēgtu cross-faction režīmu pārbaudīt abu frakciju Mercenary Contract morph uzlikšanu un to, ka atbilstošais rases morphs tiek pilnīgi noņemts reizē ar līguma auru.

## Pakete 178 — Death Knight Pillar of Frost dzīves cikls

Fails: `src/server/scripts/Spells/spell_dk.cpp`.

Pillar of Frost spellam 51271 šajā klienta būvē vairs nav skriptā pieņemtā effect 2 `DUMMY` aura. Incapacitate imunitātes ieslēgšana un izslēgšana neizmanto konkrēta aura efekta datus, tādēļ abi dzīves cikla hooki tagad vienreiz piesaistās pirmajam faktiskajam auras efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 12 sekundēs; abas `spell_dk_pillar_of_frost` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 35 uz 33.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Death Knight spēlē aktivizēt Pillar of Frost, pārbaudīt incapacitate imunitāti auras laikā un tās pilnīgu noņemšanu pēc auras beigām vai manuālas atcelšanas.

## Pakete 179 — Conclave of Wind Hurricane transporta aura

Fails: `src/server/scripts/Kalimdor/ThroneOfTheFourWinds/boss_conclave_of_wind.cpp`.

Hurricane spellam 86492 šajā klienta būvē effect 0 vairs nav skriptā pieņemtais `MOD_STUN`. Pasažiera ievietošana transportā un visu pasažieru noņemšana izmanto tikai visas auras sākumu un beigas, tādēļ abi hooki tagad vienreiz piesaistās pirmajam faktiskajam auras efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; abas `spell_hurricane` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 33 uz 31.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Conclave of Wind cīņā pārbaudīt, ka Hurricane sākumā mērķis ieiet pareizajā vehicle seat un auras beigās vai noņemšanā visi pasažieri tiek korekti izlaisti.

## Pakete 180 — Pursuing the Black Harvest Fel Energy aura

Fails: `src/server/scripts/Scenario/PursuingTheBlackHarvest/pursuing_the_black_harvest.cpp`.

Fel Energy spellam 140116 šajā klienta būvē effect 0 vairs nav skriptā pieņemtais `DUMMY`. Papildu Fel Energy auras uzlikšana un paredzamā vehicle ride stāvokļa atcelšana ir visas auras dzīves cikla darbības, tādēļ abi hooki tagad vienreiz piesaistās pirmajam faktiskajam auras efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; abas `spell_fel_enery` validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 31 uz 29.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pursuing the Black Harvest scenārijā pārbaudīt Fel Energy (140116) sākumu, papildu `SPELL_FEL_ENERGY_DUMMY_2` uzlikšanu un korektu vehicle ride stāvokļa atcelšanu auras beigās.

## Pakete 181 — Warlock Immolate noņemšanas hooks

Fails: `src/server/scripts/Spells/spell_warlock.cpp`.

Immolate spellam 157736 effect 0 faktiskā aura ir `PERIODIC_DAMAGE`, ko jau pareizi izmantoja apply hooks. Remove hooks kļūdaini gaidīja `MOD_DAMAGE_PERCENT_TAKEN`; tagad abas viena un tā paša efekta dzīves cikla darbības izmanto faktisko effect 0 tipu.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika viens iepriekš eksistējošs, ar šo labojumu nesaistīts C5055 brīdinājums tajā pašā failā.
- Pilns starts pabeigts 11 sekundēs; `spell_warl_immolate` validācijas kļūda pazuda un `Server.log` skaits samazinājās no 29 uz 28.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Warlock spēlē uzlikt, atkārtoti uzlikt un noņemt Immolate, pārbaudot, ka saistītā aura 205690 tiek noņemta visos trīs dzīves cikla gadījumos.

## Pakete 182 — Tirisfal Darnel summon auras beigas

Fails: `src/server/scripts/EasternKingdoms/zone_tirisfal_glades.cpp`.

Quest 26800 Darnel summon auras noņemšanas darbība vairs neatbilst vecajam effect 1 `DUMMY` izkārtojumam. Tā izmanto tikai visas auras beigu notikumu, lai despawnotu saistītos Darnel summon, tādēļ hooks tagad vienreiz piesaistās pirmajam faktiskajam auras efektam.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; `spell_summon_darnel_q26800` validācijas kļūda pazuda un `Server.log` skaits samazinājās no 28 uz 27.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Quest 26800 laikā izsaukt Darnel, pēc tam pabeigt vai atcelt summon auru un pārbaudīt, ka visi ar caster saistītie Darnel summon korekti despawno.

## Pakete 183 — Effect-independent aura dzīves cikla hooki

Faili: `boss_jandice_barov.cpp`, `boss_council_of_elders.cpp`, `thunder_forge.cpp`, `boss_ashgolm.cpp` un `boss_helya.cpp`.

Gravity Flux, Sandstorm, Small Sha Fixate, Brittle un Brackwater handleri izmanto tikai visas auras sākuma vai beigu notikumu un neizmanto `AuraEffect` datus. To novecojušās konkrēto tipu/indeksu piesaistes aizstātas ar vienreizēju piesaisti pirmajam faktiskajam aura efektam. Sandstorm gadījumā izlabota arī aura hookā kļūdaini izmantotā spell effect konstante.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika septiņi iepriekš eksistējoši, ar šo paketi nesaistīti C4305 brīdinājumi divos failos.
- Pilns starts pabeigts 11 sekundēs; visas piecas mērķa validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 27 uz 22.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pārbaudīt Gravity Flux papildu spellu 114038, Sandstorm visual/Living Sand aktivizēšanu, Small Sha fixate mērķa izvēli, Ash'golm Brittle dialogu un Brackwater beigu castu 201397.

## Pakete 184 — DBC precīzie periodisko auru tipi

Faili: `boss_paragons_of_the_klaxxi.cpp`, `boss_norushen.cpp`, `boss_garrosh_hellscream.cpp` un `boss_imperator_margok.cpp`.

Vienreizēja ielādētā DBC diagnostika apstiprināja faktiskos effect 0 tipus: Fiery Edge ir `PERIODIC_DUMMY`, Blind Hatred un Volatile Anomalies ir `PERIODIC_TRIGGER_SPELL`, bet Growing Power ir `PERIODIC_ENERGIZE`. Hooki piesaistīti šiem konkrētajiem tipiem; diagnostikas kods pirms gala būves pilnībā noņemts.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika viens iepriekš eksistējošs, ar šo paketi nesaistīts C4305 brīdinājums.
- Pilns starts pabeigts 12 sekundēs; visas četras mērķa validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 22 uz 18.
- Žurnālā nav pagaidu `SPELL_DIAG` ierakstu, `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pārbaudīt Fiery Edge periodisko damage castu, Blind Hatred staru, Garrosh Growing Power enerģijas slieksni un Imperator Margok Volatile Anomalies izsaukšanu abās paredzētajās fāzēs.

## Pakete 185 — DBC precīzie aura effect indeksi

Faili: `boss_wing_leader_neronok.cpp`, `spell_druid.cpp` un `antorus.cpp`.

Ielādētā DBC diagnostika apstiprināja, ka Quick-Dry Resin `ENABLE_ALT_POWER` ir effect 1, Ekowraith otrā Astral Influence korekcija pieder effect 6 (nevis atkārtoti effect 5), bet Bound by Fel periodiskais `DUMMY` ir effect 0. Attiecīgie hooki pārvietoti uz šiem faktiskajiem indeksiem, saglabājot konkrētos aura tipus.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika divi iepriekš eksistējoši, ar šo paketi nesaistīti C5055 brīdinājumi `spell_druid.cpp`.
- Pilns starts pabeigts 11 sekundēs; visas četras mērķa validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 18 uz 14.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pārbaudīt Quick-Dry Resin alternate power palielināšanu/samazināšanu, Ekowraith Astral Influence range korekcijas un Bound by Fel pāra sasaisti, attāluma pārbaudi un stack limitu.

## Pakete 186 — Argus Titanforging periodisko skriptu sadalīšana

Faili: `boss_argus.cpp` un `sql/updates/world/2026_09_19_150_split_argus_titanforging_periodic.sql`.

Spells 257213 izmanto effect 1 periodisko pārbaudi/buff aprēķinu, bet 257214 izmanto effect 0 enerģijas samazināšanu. Kopīgais skripts sadalīts divos, lai katram spellam reģistrētu tikai tā faktisko periodisko efektu un paredzēto darbību. SQL updateris 257214 piesaista jaunajam reduced-energy skriptam.

### Pārbaudes rezultāts

- SQL updateris piemērots un reģistrēts kā `RELEASED`; datubāzē abi spelli piesaistīti savam skriptam.
- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika viens iepriekš eksistējošs, ar šo paketi nesaistīts C4305 brīdinājums.
- Pilns starts pabeigts 12 sekundēs; abas Argus validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 14 uz 12.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Argus cīņā pārbaudīt Titanforging enerģijas sliekšņus un buff procentus spellam 257213, kā arī vienas enerģijas vienības samazināšanu katrā 257214 tickā.

## Pakete 187 — Echo of Sylvanas Seeping Shadows piesaiste

Fails: `sql/updates/world/2026_09_19_151_fix_echo_seeping_shadows_script.sql`.

`spell_echo_of_sylvanas_seeping_shadows` bija kļūdaini piesaistīts Death Grip AOE spellam 101397, kuram nav periodiskas auras. Encounter kods Seeping Shadows mehānikai casto periodisko dummy spellu 103175; SQL updateris pārvieto tikai šo skripta piesaisti uz pareizo spellu.

### Pārbaudes rezultāts

- SQL updateris piemērots un reģistrēts kā `RELEASED`; datubāzē skripts tagad piesaistīts spellam 103175.
- Ar iepriekš pārbaudīto Release būvi pilns starts pabeigts 11 sekundēs; abas dublētās Seeping Shadows validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 12 uz 10.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Echo of Sylvanas cīņā pārbaudīt, ka Seeping Shadows 103175 periodiski atjauno 103182 stacku skaitu līdz 20% no bosa trūkstošās veselības, netraucējot Death Grip AOE 101397.

## Pakete 188 — Emerald Nightmare jaukto aura izkārtojumu sadalīšana

Faili: `boss_nythendra.cpp`, `boss_elerethe_renferal.cpp` un `sql/updates/world/2026_09_19_152_split_emerald_nightmare_aura_scripts.sql`.

Nythendra Rot DOT 203096 un Volatile Rot 204463 beigu darbības sadalītas atsevišķos skriptos atbilstoši effect 0 `PERIODIC_TRIGGER_SPELL` un effect 2 `MOD_SCALE`; tas arī novērš Volatile Rot beigu darbības dubultu izpildi. Elerethe Web of Pain absorb daļa paliek kopīga 215300/215307, bet Mythic periodiskā Pain Lash pārbaude izdalīta atsevišķā skriptā tikai periodiskajam spellam 215300.

### Pārbaudes rezultāts

- SQL updateris piemērots un reģistrēts kā `RELEASED`; visas četras spellu piesaistes datubāzē atbilst sadalītajiem skriptiem.
- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 12 sekundēs; abas Emerald Nightmare validācijas kļūdas pazuda un `Server.log` skaits samazinājās no 10 uz 8.
- `DBErrors.log` palika tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Nythendra cīņā pārbaudīt vienu Infested Ground izveidi pēc Rot DOT un tieši četrus visual castus pēc Volatile Rot; Elerethe Mythic cīņā pārbaudīt Web of Pain absorb/atstaroto damage un Pain Lash spēlētājiem starp sasaistīto pāri.

## Pakete 189 — Neizmantojamo legacy spell loaderu noņemšana

Faili: `spell_generic.cpp` un `sql/updates/world/2026_09_19_153_remove_unusable_legacy_spell_scripts.sql`.

Seši vecie riding-skill pakāpju mount skripti atsaucās uz šajā Legion DBC neesošiem mount variantiem, bet Seaforium achievement skripts — uz neesošu credit spellu 60937. Šie skripti jau līdz šim neizturēja `Validate()` un netika izpildīti. Novecojušās DB piesaistes un tieši šo septiņu loaderu reģistrācija noņemta kopā, saglabājot pamatspellu DBC noklusējuma darbību un neatstājot nepiesaistītus loaderus.

### Pārbaudes rezultāts

- SQL updateris piemērots un reģistrēts kā `RELEASED`; datubāzē nav palikusi neviena no septiņām novecojušajām piesaistēm.
- `worldserver` Release būve pabeigta bez kompilācijas kļūdām; palika četri iepriekš eksistējoši, ar šo paketi nesaistīti C5055 brīdinājumi.
- Pilns starts pabeigts 11 sekundēs; visas septiņas `Validate()` kļūdas pazuda un `Server.log` skaits samazinājās no 8 uz 1.
- Pēc loaderu reģistrācijas noņemšanas `DBErrors.log` atkal ir tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Izsaukt Magic Broom, Headless Horseman's Mount, Big Love Rocket, Invincible, Celestial Steed un X-53 Touring Rocket ar Legion mount sistēmu; battleground vidē pārbaudīt Seaforium noklusējuma gameobject damage darbību bez novecojušā achievement-credit papildinājuma.

## Pakete 190 — Arena Season world state starta secība

Faili: `src/server/game/World/World.cpp` un `src/server/game/Events/GameEventMgr.cpp`.

Arena season starts iepriekš notika pirms sākotnējo globālo world state izveides, tādēļ trūkstošā `uint32` vērtība tika saīsināta uz 255. Sākotnējie world state tagad tiek izveidoti pirms arena starta, un sezonas ID vairs netiek saīsināts uz `uint8`. Ja konfigurētajai Legion sezonai nav izvēles legacy `game_event_arena_seasons` sasaistes, serveris korekti turpina startu ar informatīvu ierakstu, jo šī tabula satur tikai vecos 3.–8. sezonas vendor eventus.

### Pārbaudes rezultāts

- `worldserver` Release būve pabeigta bez kompilācijas kļūdām.
- Pilns starts pabeigts 11 sekundēs; ArenaSeason kļūda pazuda un `Server.log` skaits samazinājās no 1 uz 0.
- Konfigurētā sezona tiek nolasīta kā 14, nevis nederīgā 255, un legacy eventa neesamība tiek reģistrēta INFO līmenī.
- `DBErrors.log` ir tukšs (0 kļūdu), un serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pieslēgt klientu un pārbaudīt arena season/rated PvP world state vērtības; ja vēlāk tiek pievienots konkrēts Legion sezonas vendor game events, piesaistīt to `game_event_arena_seasons` konfigurētajam sezonas ID.
