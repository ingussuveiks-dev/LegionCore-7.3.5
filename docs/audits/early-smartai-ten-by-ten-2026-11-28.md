# 10×10 SmartAI pāreju audits

2026. gada 28. novembrī pārbaudītas 100 unikālas SmartAI pārejas 32 tēliem. Atlasē priekšroka dota kļūdainām pārejām un tēliem, kuri ir tiešie kvestu mērķi. Pārbaudīti tēli ar agrīna līmeņa šablonu un vismaz vienu novietojumu Austrumu karaļvalstīs, Kalimdorā vai atsevišķā sākuma apgabala kartē. Šablona līmenis var atšķirties no kvesta pieejamības līmeņa. Katra tabulas rinda ir viena pāreja, un vairāki ieraksti var piederēt vienam kvestam vai tēlam.

Pārbaude seko `WORLD_SEL_SMART_SCRIPTS` ielādes kārtībai pēc `entryorguid`, `source_type`, `id`, `link`. No pirmās atbilstošās rindas `SmartScript::FindLinkedEvent` turpina uz `SMART_EVENT_LINK` ar nenulles darbību. Pārejas izsekotas līdz pēdējai darbībai, trūkstošam mērķim vai neatbilstošam notikuma tipam. Ielādes kārtība ir svarīga, jo datubāzes primārajā atslēgā ietilpst arī `link`. Captured Razormane `34523` blakus pārbaudē parādīja, ka divas rindas ar vienādu ID var dot nepareizu slēdzienu no vienkārša SQL savienojuma. Pirmais ID 4 tā pašreizējā secībā ir derīgais `LINK` uz Saintly, un otra rinda ir patstāvīgs dialoga notikums. Daio `107716` abi vecajos žurnāla ierakstos redzamie gadījumi jau bija sakārtoti pirms šā audita.

## Labojumi

- Northwatch Ranger `39261` agresijas ķēde atkal iet secībā 0 → 1 → 2 → 3. Tā apstādina kustību, vienreiz izpilda `6660` šāvienu un ieslēdz distances kaujas fāzi. Noņemts identisks otrais kustības apturēšanas notikums. Darbību secību apstiprina tā paša servera Palemane Poacher `2951` ķēde. Distances un kaujas atiestatīšanas notikumu rindas palika ārpus labojuma.
- Frozen Mountaineer `41763` izsaukšanas emocija beidzas pēc savas darbības. Atsevišķais trīs sekunžu taimeris turpina virzību un ieplāno tēla pazušanu vēl pēc trim sekundēm. Esošais taimeris un tā `LINK` tika pārbaudīti kopā ar izsaukšanas notikumu. Salīdzinājumam TrinityCore pašreizējā realizācija arī vada kustību ar atliktu notikumu. [TrinityCore Frozen Mountaineer](https://main.trinitycore.net/de/d08/zone__dun__morogh_8cpp_source)
- Cowering Trooper `45910` klikšķis saglabā esošo `27547` kredītu un pirmās teksta grupas runu. Pazušanu pēc runas vada esošais `TEXT_OVER` ar četru sekunžu aizkavi. Dzīvajos datos pārbaudīts klikšķa nosacījums un šīs runas teksta ieraksts. Saites uz neeksistējošu starpposmu vietā runas notikums beidzas pēc savas darbības.

Scarlet Corpse `49340` jāizmeklē kā viena `26800` secība, jo klikšķis, izsauktā kopija, kredīts un Darnell transportlīdzeklis izmanto kopīgus ID. Darkshore savvaļas dzīvnieku klikšķu avots ir kvests `13566`. Iepriekš remontētais `13885` lidojums ir atsevišķs gadījums ar citiem mērķu identifikatoriem.

## Pārbaudes un uzstādīšana

**Noslēguma stāvoklis** — pārbaudītas visas 100 atlasītās pārejas. Sākotnēji derīgas bija 86, bet 14 atradnes salabotas. Spēles klienta iziešanas pārbaudes norādītas pie katra labojuma.

- **86 pārejas** atrisinās līdz derīgam gala notikumam. **3 pārejas** salabotas un pārbaudītas dzīvajā datubāzē. **11 pārejas** atstātas atvērtas ar nākamo izpētes punktu. Tabulās parādīts struktūras rezultāts. Tēlu parādīšanos, fāzes, redzamību, runu, uzvedību un kvestu iziešanu spēles klientā šis audits nepārbaudīja.
- MariaDB pārbaudē izmantoti pagaidu galdi un lietotājs ar lasīšanas tiesībām. Pārbaudītas precīzas rindas pirms un pēc labojuma, atkārtota migrācija, nesaistīts avota tips un trīs atšķirīgu lokālo variantu saudzēšana. Pirmajā pārbaudes palaišanā MariaDB noraidīja pagaidu tabulas izveidi ar tādu pašu nosaukumu kā shēmas avotam. Shēmas kopija pārdēvēta. Pēc labojuma gan izolētā pārbaude, gan dzīvo datu salīdzinājums izdevās.
- Kanoniskais `worldserver` ielādēja `2026_11_28_10_early_smartai_continuations.sql`. Pārbaudīta precīza `RELEASED` rindas un migrācijas kontrolsummas atbilstība. Serveris sasniedza gatavību un izslēdzās ar kodu 0. Nebija jaunu avāriju, pievienotu DB kļūdu, brīdinājumu vai assert ierakstu. Vecais jauktais 1347 baitu `DBErrors.log` ir saglabāts. Diagnostikas oriģinālie baiti un SHA-256 kontrolsummas glabājas lokāli `build-extractors/bin/Release/fixed/2026-11-28-early-smartai-continuations/`.
- Labojums ir tikai pasaules datos. Izmantotā servera binārā faila SHA-256 `5FD82540768E75FD3409D8E7FFB044147764824334C42C0AAD849E94DE2F7010`. Pārbaudei nebija pagaidu servera būvējuma. Pārbaudes servera process un pagaidu datubāzes lietotājs ir aizvērti.

## Pārbaudītās pārejas

Turpinājumā salabota Skittering Spiderling `42689` savākšana. Tālāk redzamās tabulas saglabā sākotnējās 10×10 partijas stāvokli. [All That Skitters pārbaude](skittering-spiderling-2026-11-28.md)

Scarlet Corpse `49340` izmeklēšana noveda pie labojuma aktīvajā C++ savākšanas un Darnell izsaukšanas ceļā. [Recruitment pārbaude](recruitment-darnell-2026-11-28.md)

Darkshore trīs savākšanas pārejas salabotas kopā ar `Ritual Materials` priekšmetu saņemšanas apstrādi. [Ritual Materials pārbaude](darkshore-ritual-materials-2026-11-28.md)

Abas Greymist pārejas salabotas kopā ar kvesta `13560` robota izsaukšanu, detonāciju un atgriešanos pie konsoles. Pēc šā labojuma sākotnējā atlasē atvērtas četras pārejas, tostarp divas Archmage Ataeric un pa vienai Kolkar Marauder un Bluegill Representative. [Greymist pārbaude](darkshore-greymist-decoy-2026-11-28.md)

Kolkar Marauder pāreja novērsta, izņemot pirms Cataclysm izmantoto nejaušo Verog izsaukšanas ceļu un pārbaudot pašreizējo Wyneth ceļu. Vēl atvērtas divas Archmage Ataeric pārejas un viena Bluegill Representative pāreja. [Verog pārbaude](verog-wyneth-source-2026-11-28.md)

Abas Archmage Ataeric pārejas aizstātas ar sākotnējo vairoga un Arcane Fiend burvestību secību, kas ieskaita `27518` pēc faktiskas vairoga pārslodzes. Pēc šā labojuma atvērta Bluegill Representative pāreja. [Ataeric pārbaude](ataeric-shield-2026-11-28.md)

Pēdējā Bluegill Representative pāreja atjaunota ar vadītāja kliedzienu pirms uzbrukuma. `26868` ieskaita maskētā spēlētāja faktisko feromonu trāpījumu. [Axis of Awful pārbaude](axis-of-awful-bluegill-2026-11-28.md)

Tabulā `⇒` rāda mainīto ceļu, karte un zona ir norādītas ar to datubāzes ID. Kvesta kolonnā uzskaitīti mērķi, kas tieši atsaucas uz attiecīgā tēla ID. Papildu kredīta un burvestību saites atvērtajiem gadījumiem norādītas rezultātā.

### Partija 1

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 45910 Cowering Trooper | 0:130 | 27547 | 1 → 2 ⇒ 1 | Salabota |
| 49340 Scarlet Corpse | 0:6454 | 26800 | 0 → 1 | Atvērta. Notikuma tips 38, darbība 41. Jāizseko klikšķis, izsauktā kopija un Darnell sēdvieta kvestā 26800. |
| 2120 Archmage Ataeric | 0:130 | — | 0 → 1 | Atvērta. Notikuma tips 0, darbība 11. Jānodala vairogs, palīgi, kaujas termiņš un 45769 kredīts. |
| 2120 Archmage Ataeric | 0:130 | — | 1 → 2 | Atvērta. Notikuma tips 0, darbība 37. Jānodala vairogs, palīgi, kaujas termiņš un 45769 kredīts. |
| 3275 Kolkar Marauder | 1:17,1:4709 | — | 1 → 2 | Atvērta. Notikuma tips 0, darbība 11. Mērķa ID pieder atsevišķam Tetanus taimerim. |
| 33127 Moonstalker | 1:148 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 13566 klikšķa burvestība 62512 un sekošana. |
| 33262 Greymist Tidehunter | 1:148 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 62682 trāpījums un 62683 kredīts. |
| 33277 Greymist Warrior | 1:148 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 62682 trāpījums un 62683 kredīts. |
| 33313 Mottled Doe | 1:148 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 13566 klikšķa burvestība 62788 un sekošana. |
| 33978 Hungry Thistle Bear | 1:148 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 13566 klikšķa burvestība 64116 un sekošana. |

### Partija 2

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 39261 Northwatch Ranger | 1:14 | — | 0 → 1 ⇒ 0 → 1 → 2 → 3 | Salabota |
| 41763 Frozen Mountaineer | 0:1 | — | 0 → 1 ⇒ 0 | Salabota |
| 42689 Skittering Spiderling | 0:47 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. Jāizseko 79591 abiem kvestiem 26310 un 26523. |
| 44263 Bluegill Representative | 0:38 | — | 0 → 1 | Atvērta. Trūkst mērķa rindas. 4426300 jau vada aizkavētu uzbrukumu. Tūlītējā darbība nav identificēta. |
| 448 Hogger | 0:12 | 176 | 0 → 1 | Derīga |
| 1538 Scarlet Friar | 0:85 | 371,25009 | 2 → 3 | Derīga |
| 2269 Hillsbrad Miner | 0:267 | 539 | 0 → 1 → 2 | Derīga |
| 2269 Hillsbrad Miner | 0:267 | 539 | 1 → 2 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 0 → 1 → 2 → 3 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 1 → 2 → 3 | Derīga |

### Partija 3

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 2 → 3 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 4 → 5 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 6 → 7 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 8 → 9 → 10 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 9 → 10 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 11 → 12 → 13 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 12 → 13 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 14 → 15 | Derīga |
| 2951 Palemane Poacher | 1:215,1:357 | 745 | 16 → 5 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 0 → 1 → 2 → 3 | Derīga |

### Partija 4

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 3265 Razormane Hunter | 1:17 | 871 | 1 → 2 → 3 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 2 → 3 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 4 → 5 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 6 → 7 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 8 → 9 → 10 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 9 → 10 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 11 → 12 → 13 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 12 → 13 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 14 → 15 | Derīga |
| 3265 Razormane Hunter | 1:17 | 871 | 18 → 19 | Derīga |

### Partija 5

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 6190 Spitelash Warrior | 1:16 | 24435 | 0 → 1 → 2 → 3 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 1 → 2 → 3 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 2 → 3 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 4 → 5 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 6 → 7 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 8 → 9 → 10 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 9 → 10 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 11 → 12 → 13 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 12 → 13 | Derīga |
| 6190 Spitelash Warrior | 1:16 | 24435 | 14 → 15 | Derīga |

### Partija 6

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 6190 Spitelash Warrior | 1:16 | 24435 | 16 → 17 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 0 → 1 → 2 → 3 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 1 → 2 → 3 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 2 → 3 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 4 → 5 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 6 → 7 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 8 → 9 → 10 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 9 → 10 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 11 → 12 → 13 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 12 → 13 | Derīga |

### Partija 7

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 14 → 15 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 18 → 19 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 24 → 5 | Derīga |
| 12856 Ashenvale Outrunner | 1:331 | 6503 | 25 → 5 | Derīga |
| 23507 Elwynn Pink Elekk | 0:12 | 11118 | 0 → 1 | Derīga |
| 23527 Teldrassil Pink Elekk | 1:1657 | 11118 | 0 → 1 | Derīga |
| 32911 Gershala Nightwhisper | 1:148 | 13518 | 0 → 1 | Derīga |
| 32964 Sentinel Aynasha | 1:148 | 13510 | 0 → 1 → 2 | Derīga |
| 32964 Sentinel Aynasha | 1:148 | 13510 | 1 → 2 | Derīga |
| 32964 Sentinel Aynasha | 1:148 | 13510 | 4 → 5 | Derīga |

### Partija 8

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 32964 Sentinel Aynasha | 1:148 | 13510 | 6 → 7 | Derīga |
| 32964 Sentinel Aynasha | 1:148 | 13510 | 8 → 9 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 0 → 1 → 2 → 3 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 1 → 2 → 3 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 2 → 3 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 4 → 5 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 6 → 7 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 8 → 9 → 10 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 9 → 10 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 11 → 12 → 13 | Derīga |

### Partija 9

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 32970 Rit'ko | 1:148 | 13511 | 12 → 13 | Derīga |
| 32970 Rit'ko | 1:148 | 13511 | 14 → 15 | Derīga |
| 33024 Uncorrupted Thistle Bear | 1:148 | 13557 | 0 → 1 | Derīga |
| 33056 Shatterspear Supplies Credit Bunny | 1:148 | 13509 | 0 → 1 | Derīga |
| 33093 Cerellean Whiteclaw | 1:148 | 13518 | 0 → 1 | Derīga |
| 33094 Volcor | 1:148 | 13518 | 0 → 1 | Derīga |
| 33095 Shaldyn | 1:148 | 13518 | 0 → 1 | Derīga |
| 33178 Huntress Sandrya Moonfall | 1:148 | 13590 | 1 → 2 | Derīga |
| 33178 Huntress Sandrya Moonfall | 1:148 | 13590 | 4 → 5 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 0 → 1 → 2 → 3 | Derīga |

### Partija 10

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 33207 Lady Janira | 1:148 | 13565 | 1 → 2 → 3 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 2 → 3 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 4 → 5 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 6 → 7 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 8 → 9 → 10 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 9 → 10 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 11 → 12 → 13 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 12 → 13 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 14 → 15 | Derīga |
| 33207 Lady Janira | 1:148 | 13565 | 18 → 19 | Derīga |
