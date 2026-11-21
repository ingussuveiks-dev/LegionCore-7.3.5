# Kvesta priekšmetu krājumi Darkshore, Hillsbrad, Undercity un Wandering Isle

Četriem kvestiem vispirms atradu īsto loot atslēgu pēc šablona. Katras iesaistītās atslēgas tabulā ir tikai viens kvesta priekšmets. Vecās nesaistītās loot rindas un jauktā satura kastes saglabātas.

| Kvests | Pasaules avots | Nodošana |
| --- | --- | --- |
| **13909 Got Some Flotsam?** | 26 Greymist Debris `195042/195080` Darkshore. Abi varianti izmanto tabulu `195080`. | 5 Salvageable Greymist Wreckage `46384` pie Archaeologist Groff `34340`. |
| **28325 Green Living** | 16 Used Blight Canister `207089/207090` Hillsbrad ar tabulu `35870`. | 8 Used Blight Canisters `63432` pie Serge Hinott `2391`. |
| **29360 Would You Like Some Flies With That?** | 31 Fly Covered Meat `208874/208876` Undercity ar tabulu `39791`. Vienā reizē izkrīt 1–5. | 25 Bloated Flies `69989` pie Eunice Burch `4552`. Nodošana izņem arī atlikumu, ja pēdējā porcijā bija vairāk nekā prasīts. |
| **29418 Kindling the Fire** | 17 Loose Dogwood Root `209326/209327` Wandering Isle ar tabulu `40266`. | 5 Dry Dogwood Roots `72111` pie Ji Firepaw `54568`. |

Sākotnējā native pārbaude visiem četriem kvestiem atkārtoja tukšas kastes atvēršanu un patērēšanu bez kvesta. Pēc atmešanas iepriekš savāktais priekšmets palika somā. Labojums izmanto kopīgo kastes skriptu ar katras veidnes autorēto kvesta un priekšmeta ID. Standarta slēdzene, loot iespēja un attiecīgā spawna atjaunošanās laiks saglabāti. Kastes modelis un tā atjaunošanās ir koplietojami. Mirdzums un faktiskā atvēršana seko konkrētā spēlētāja trūkstošajam mērķim.

`Test-QuestObjectLootNative.ps1 -SupplyChestsBaseline` apstiprināja abas sākotnējās kļūdas. `-SupplyChests` pārbaudīja 90 esošus novietojumus, visu astoņu veidņu standarta atvēršanas burvestības, atteikumu bez kvesta un pēc pabeigšanas vai nodošanas, loot saņemšanu, atmešanu, atkārtotu pieņemšanu un priekšmetu izņemšanu pēc native atlīdzības. Undercity pārbaudē pēdējā kaste palielināja skaitu no 24 uz 29. Atlīdzība izņēma visas 29 mušas. Testa laika solis paātrināja gaidīšanu līdz faktiskā GO taimera iztecēšanai. NPC un piedāvājumi pārbaudīti datos, zonde izsauca servera atlīdzības metodi. Ji Firepaw piesaistītais `npc_panda_announcer` apstrādā apkārtnes uzrunas, savukārt kastu atvēršana un priekšmeta uzskaite iet pa loot ceļu.

Migrācijas `2026_11_21_48_single_quest_supply_chests.sql` SHA-1 ir `AC12EFDB5796035AE65F18BDF5994F911A389856`. Izolētajā un faktiskajā pasaulē sakrita 52 nolasītās rindas. Atkārtota migrācija neko nemainīja. Izvietotajā `worldserver.exe` ar SHA-256 `C9B571148A381A670260833B21DB8BDE372A805D43721B55461B7D060C8916F2` pārbaudīti abi faktiskie linker bibliotēku ceļi un jaunās kastes saite uz kvesta, priekšmeta un loot loga pārbaudēm. Pagaidu un kanoniskais Release startēja ar migrēto izolēto pasauli un korekti apstājās. Kanoniskie diagnostikas faili pēc testa un importa palika bez jaunām kļūdām vai avārijām.

Gala zondē ielādēja piegādes karogu pēc addon, kā to dara pamata ielādētājs, un ikdienas atlīdzību pārbaudīja tās faktiskajā uzskaitē. Ar pārbaudītām SHA-256 saglabāti 168 diagnostikas faili zem `build-extractors/bin/Release/fixed/single-quest-supply-chests-2026-11-21/`. Izņemti 24 pagaidu faili jeb 1 012 205 455 baiti. Iepriekšējais Release un koplietotā izolētā pasaules kopija saglabāti atkopšanai un pārbaudēm.

7.3.5 klientā vēl jāpārbauda faktiskais NPC piedāvājums un nodošanas logs, mirdzums, peldēšana pie Greymist Debris un atjaunošanās dabiskā laikā. Undercity veidnēm ir arī četri Draenor spawni ar 180 sekunžu iestatījumu, kuru kustība un vizuālā uzvedība šajā izpildē nav pārbaudīta.
