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
