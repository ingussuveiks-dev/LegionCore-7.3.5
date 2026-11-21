# 1.–20. līmeņa kvestu loot audita turpinājums

No sākotnējā audita atlasīti 69 ieslēgti kvestu ieraksti ar vismaz vienu neatrisinātu loot atslēgu vai objektā minētu priekšmetu bez tiešas loot saites. No tiem 27 bija atrasta piedāvājuma sasaiste un 42 tāda vēl nebija izsekota. Piedāvājuma saite viena pati nepierāda, ka devējs ir pieejams. Saraksta skaitļi raksturo sākotnējo eksportu pirms Serpentbloom un šīs partijas labojumiem.

## Piemērotie labojumi

| Kvests | Objekts un priekšmets | Izmaiņa |
| --- | --- | --- |
| **866 · Root Samples** | Ziemeļu Barrens augi `1620/1622/3724/3725/3726/3727/3729`, Root Sample `5056` | Trīs objektiem atjaunotas saites uz pareizā parastā auga loot. Septiņās sasaistītajās tabulās sakni dod tikai spēlētājam ar aktīvu kvestu un trūkstošu vienību Ziemeļu Barrens. Parastās augu rindas un jau esošo 5056 rindu izkrišanas varbūtības saglabātas. |
| **13526 · The Bear’s Paw** | `194100/44850` | Kvesta loot nosacījums tagad ir arī gathering-node avotā. |
| **25110 · Kaja’Cola Gives You IDEAS! (TM)** | `202552/202553/202554` un `52483` | Kvesta loot nosacījums visām trim kārbām. Otra Kaja’Cola priekšmeta un pārējā loot rindas saglabātas. |
| **25136 · Galgar’s Cactus Apple Surprise** | `171938/11583` | Kaktusa ābols tiek filtrēts pēc aktīvā kvesta. Tās pašas vienības vecā 4402 sasaiste saglabāta. |
| **25735 · Incendicite Ore** | `203188/55241` | Rūdai pievienota kvesta prasība. Esošais rīks 55240, burvestība 77020 un slēdzene 1896 saglabāti. |
| **25937 · Priceless Treasures** | `203385/56225` | Artefaktam pievienota kvesta prasība. Esošais rīks 56226, burvestība 77821 un slēdzene 1912 saglabāti. |

Root Samples lokālajā 7.3.5 datubāzē ir astoņu sakņu mērķis ar Herbalism prasību. Atjaunotie avoti ir 64 esošie spawni kartē 1, zonā 17. Slēdzene 29 joprojām izmanto parasto zāļu vākšanas ceļu. Vecākais TrinityCore labojums apstiprina Barrens augu saimi un 100% atsevišķu kvesta pilienu. No tā pārņemta šī varbūtība tikai trim iztrūkstošām rindām. Novietojums, loot atslēgas un lauki pārbaudīti šā projekta datos. [TrinityCore 3.3.5 atsauce](https://github.com/TrinityCore/TrinityCore/commit/2420b174e3955e13382977303350b2250a752211)

Abas migrācijas izmanto esošo `GetLootId`, `HasQuestForItem` un item loot nosacījumu ceļu. Loot pārbaude attiecas uz katru saņēmēju, arī grupā, neatkarīgi no tā, kurš uzsāka mijiedarbību. Koplietojamo objektu fiziska redzamība un parastā zāļu vākšana turpinās. Priekšmetu pieejamība pirms pieņemšanas, pie pilna skaitītāja, pēc atmešanas un nodošanas izmanto servera standarta nepabeigta kvesta pārbaudi.

## Pārējie sākuma atradumi

- **13796, 13909, 25668, 28325, 28724, 29360, 29418 un 29518** ir vismaz viens tiešs objekta avots ar atbilstošu veidnes loot atslēgu. Piemēram, abas Greymist Debris versijas pamatoti lasa `195080`, un trīs Darkmoon Tonk Scrap varianti lasa `40164`. Atsevišķas vecās rindas, kas glabātas zem objekta ID, pašas par sevi nepārtrauc šos ceļus.
- Parastajām piegādēm un profesiju materiāliem atrasti tirgotāji, skinning vai NPC loot. Zvejas uzdevumos **8193, 8194 un 29513** vēl atsevišķi jāpārbauda reālais baseina un ūdens loot, pasākuma laiks un uzdevuma nodošana.
- **29777 · Tools of the Enemy** ir tieša 74615 corpse loot rinda Fe-Feng Wiseman 55601, kura eksemplāri ir kartē 860. Parastajā negrupētajā loot ceļā `LootMode=0` ir derīgs. Ar nulles masku vien nepietiek, lai konstatētu defektu šajā kvestā. Īstā NPC laupīšana un skaitītājs vēl jāapstiprina.
- **29402 un 29426 · Taking Precautions** abām frakcijām trūkst attiecīgo devēju 54021/54146 spawnu, 209059 auga spawnu un loot rindas 39847/71035. Vecās Shopping Around pārejas norāda uz tiem pašiem devējiem. Paliek atjaunot visu sezonālo ķēdi ar tās abu frakciju izvietojumu un aktivizāciju. [Blizzard Legion laika Hallow’s End norise](https://worldofwarcraft.blizzard.com/en-us/news/21088694/the-horseman-rises-hallow-s-end-is-nigh)

## Pārbaude un izvietošana

- `Test-LowLevelQuestObjectLoot.ps1` salīdzināja izolētas MariaDB sākotnējās, labotās un atkārtoti migrētās rindas. Pārbaudītas visas septiņas saknes un zonas nosacījumu saites, septiņas pārējo priekšmetu rindas, objektu tips un slēdzenes, nemainīti spawnu skaiti, mērķa daudzumi, parastie augi un cita loot tajās pašās tabulās.
- `LevelQuestCatalog.cpp` ar servera DB2 lasītāju apstiprināja zāļu un parasto atvēršanu, kā arī precīzos 55240/77020/1896 un 56226/77821/1912 rīku ceļus no 7.3.5 datiem.
- Kanoniskais Release izpildfails `c4306b8a8ef64ee4254b98c6a523392f63c73a5988472a57087313535f0f3e98` ar labotajiem izolētajiem world un hotfix datiem sasniedza gatavību un korekti izslēdzās. Nebija jaunu crash, `DBErrors.log` ierakstu vai neizskaidrotu servera brīdinājumu.
- Migrācijas 29 un 30 vienā InnoDB transakcijā piemērotas faktiskajai `legion_world`. Atkārtotais nolasījums precīzi sakrita ar izolēto pārbaudi. Updater SHA-1 ir `ddf6b6f91b1fb73114f45a4702a3e9f4bb41b76b` un `16a7d05839150aac2b0c139e6cff6e2a81b0b156`. Abi uzdevuma MariaDB procesi korekti izslēdzās.
- Pierādījumi lokāli `.codex/quest-object-loot-audit-20261121/`. Pēc pārbaudes izņemti 402 748 baiti zondes binārā un kompilācijas materiāla saskaņā ar `cleanup-manifest.csv`. Iepriekšējā apturētā DB kopija tika izmantota atkārtoti, un tās dzēšana paliek atlikta iepriekšējā atļauju pārbaudes noraidījuma dēļ.

7.3.5 klientā vēl jāpārbauda vākšana pirms un pēc pieņemšanas, divi spēlētāji pie viena avota, pēdējā prasītā vienība, atmešana un faktiskā nodošana. Abām rīku ķēdēm atsevišķi jāpārbauda lietošana uz pareizā un cita objekta.
