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
