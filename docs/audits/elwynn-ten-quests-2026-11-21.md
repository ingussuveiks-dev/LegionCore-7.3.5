# Desmit Elwynn Forest kvesti

Desmit nākamie ieraksti atlasīti no 1.–20. līmeņa audita. Tiem ir dzīvi piedāvājuma ceļi ar 7. minimālo līmeni. Izmantoti projekta Legion 7.3.5 dati, MariaDB pasaules rindas un standarta servera mijiedarbības un atlīdzības ceļi.

| Kvests | Piedāvājums un mērķis | Nodošana un priekšmetu apstrāde |
| --- | --- | --- |
| **40 A Fishy Peril** | Remy `241` Goldshire | Marshal Dughan `240`. Nodošana atver 35. |
| **35 Further Concerns** | Dughan pēc 40 | Guard Thomas `261`. Nodošana atver 37. |
| **37 Find the Lost Guards** | Thomas pēc 35 | Malakai ķermeņa objekts `55`. Kvestam nav nogalināšanas, loot vai izpētes skaitītāja. Ķermenis ir īstais kvesta saņēmējs un pēc atlīdzības piedāvā 45. |
| **45 Discover Rolf’s Fate** | Malakai objekts pēc 37 | Rolf ķermeņa objekts `56`, kas pēc atlīdzības piedāvā 71. |
| **71 Report to Thomas** | Rolf objekts izsniedz vienu `735` pēc 45 | Thomas patērē vienu medaļu. Atmešana to izņem un atkārtota pieņemšana atjauno tieši vienu vienību. |
| **239 Westbrook Garrison Needs Help!** | Dughan | Deputy Rainer `963`. Nodošana atver 11. |
| **11 Riverpaw Gnoll Bounty** | Rainer pēc 239. `97` un `478` dod līdz astoņiem aktīvā kvesta apsējiem `782`. | Rainer paņem astoņus. Atjaunota savāktā daudzuma izņemšana, atmetot kvestu. |
| **46 Bounty on Murlocs** | Bounty Board `203733`. Murloki `46` un `732` dod līdz astoņām kvesta spurām `780`. | Thomas paņem astoņas. Atjaunota savāktā daudzuma izņemšana, atmetot kvestu. |
| **52 Protect the Frontier** | Thomas. Prowler `118` un Gray Forest Wolf `1922` izmanto vienu astoņu vilku mērķi. Young Forest Bear `822` dod atsevišķus piecus kredītus. | Thomas. Kvesta priekšmeta nav. |
| **5545 A Bundle of Trouble** | Supervisor Raelen `10616`. 15 koplietojami koksnes objekti `176793` apgabalos `86/88`, katrs dod vienu `13872` ar standarta slēdzeni `43` un burvestību `6478`. | Raelen paņem astoņas vienības. Skripts bloķē nepieejama kvesta vai aizņemtas kastes atvēršanu. Atjaunota jau savāktās koksnes izņemšana pēc atmešanas. |

Koksnes modelis ir redzams kā koplietojams pasaules objekts. Aktīvais mirdzums un atvēršana ir pieejami, kamēr spēlētājam vajag priekšmetu. Saglabāts sākotnējais 120 sekunžu atjaunošanās laiks un loot daudzums. Izmantots arī Ashenvale pārbaudītais atvēršanas kontrolieris, saglabājot atsevišķus skriptu nosaukumus un kvesta un priekšmeta ID. Apsēju, spuru un koksnes jaunos atmešanas datus apstrādā standarta kvesta tīrīšana. Vienādu loot un mērķa ID daudzumu atlīdzībā neskaita dubultā.

Atsevišķs agrākās datu pārcelšanas atlikums ir koksnes rinda zem Mathystra Relics loot atslēgas `2767`. Abām veidnēm, kuras uz to rāda, šajā pasaulē nav atrastu spawnu. Elwynn koksnes objekti jau izmanto citu atslēgu `176793`. Mathystra piedāvājums un īstā relikvija `5273` paliek atsevišķā auditā.

## Servera pārbaudes

- `Test-QuestObjectLootNative.ps1 -ElwynnQuestsBaseline` ar sākotnējiem datiem atkārtoja tukšas koksnes kastes atvēršanu un patērēšanu bez kvesta. Tā arī apstiprināja, ka atmešana atstāja savākto koksni, apsējus un spuras somā.
- `-ElwynnQuests` izmantoja abas faktiskās kvesta paketes pieņemšanai un atlīdzībai. Pārbaudīti pieci pāreju kvesti, divi ķermeņu objekti, priekšnoteikumu atteikumi un īstā `735` medaļa, tostarp tās izņemšana atmetot un nododot kvestu. Koksnes pārbaudē lietoti visi 15 izliktie objekti, ieskaitot atvēršanas atteikumu bez kvesta, aizņemtu kasti divām sesijām, 8/8, atkārtotu atjaunošanu un atlīdzību. Gaidīšana paātrināta līdz faktiskā GO taimera iztecēšanai.
- Gnoll un Murloc pārbaudēs standarta spēlētāja un peta nogalināšanas ceļš deva prasītos loot priekšmetus no sākotnējiem avotiem. Pārbaudīti atteikumi bez kvesta, pie pilna skaitītāja un pēc nodošanas, astoņu priekšmetu atmešana, atgūšana un nodošana. *Protect the Frontier* pārbaudīti abi vilki, lāči, spēlētāja un peta pēdējais sitiens, divi tiesīgi tuvumā esoši grupas dalībnieki un precīzi griesti 8/5. Saglabātā `-AshenvaleFelWood` pārbaude izgāja ar kopīgo kastes kontrolieri.
- Agrīnajās zondēs tika precizēts reālais `6478` atvēršanas veids un palielināts GO testa laika solis atbilstoši objekta atjaunināšanas intervālam. Pirmajā grupas pārbaudē zonde nebija ielādējusi parasto vilku un lāču loot, tāpēc radās 42 `DBErrors.log` rindas. Ielādējot 128 sākotnējās loot rindas, novērsts kļūdu cēlonis. Abas gala izpildes bija tīras.

## Izvietošana

- Izolētās datubāzes 86 rindu salīdzinājumā mainījās tikai vienas objekta veidnes skripts un trīs kvestu atmešanas metadati. Atkārtots imports neko nemainīja. Pēc izvietošanas `legion_world` 86 rindu nolasījums sakrita ar testu. Migrācija `2026_11_21_47_elwynn_quest_gathering.sql` reģistrēta kā `RELEASED` ar SHA-1 `C93C600F35E5169F5BEEC0A8E490C2170A9DD11D`.
- Pārbaudītas faktiskās `scripts.lib` un `game.lib` linkera ievades. Gala PE failā abu zonu kastes izmanto kopīgo atvēršanas pārbaudi ar saviem kvesta un priekšmeta ID. Kanoniskā `worldserver.exe` SHA-256 ir `E7A3CC6618C9941502396CF92ACA201FA90539B61FDD8F4369AB2640B96AE660`. Gan pagaidu, gan kanoniskais Release sasniedza gatavību ar migrēto izolēto pasauli un korekti apstājās. Pēc gala pārbaudēm un importa nebija jaunu avāriju, datubāzes kļūdu vai neizskaidrotu brīdinājumu.
- Ar pārbaudītiem SHA-256 saglabāti 212 diagnostikas faili zem `build-extractors/bin/Release/fixed/elwynn-ten-quests-2026-11-21/`. Izņemti 42 pagaidu faili jeb 1 734 717 278 baiti. Saglabāta iepriekšējā Release atjaunošanas kopija un koplietotā izolētā audita datubāze. Uzdevuma procesi apstājušies.

7.3.5 klientā paliek faktiskie pieņemšanas un atlīdzības logi, objektu mirdzums un animācija, gājiens starp vietām, dzīva cīņa un reālais atjaunošanās intervāls.
