# 1.–20. līmeņa kvestu atkārtotais audits

Atkārtoti analizēti visi 5 459 atlases ieraksti no 30 316 pasaules veidnēm. Ieslēgti ir 4 865, bet 594 ir projekta `disables` tabulā. Atlase ietver nominālo vai minimālo 1.–20. līmeni un 289 nulles minimuma ierakstus ar atrastu piedāvājuma sasaisti. Vēl 3 304 nulles minimuma ierakstiem piedāvājums paliek neizsekots, tajā skaitā 11 ar iespējamu C++ atsauci.

Atjaunotajā pārskatā katram ierakstam ir iespējamais devējs, saņēmējs, objekti un aktieri ar pasaules novietojumiem, priekšmeta avoti, mērķa daudzums, lietošanas burvestības, cleanup metadati un piesaistītie skripti. Atrasti 154 sākotnējās pasaules piesaistīti C++ nosaukumi, un visi atrasti izmantotajā Release failā. Šie ir datu, koda sasaistes un binārā faila pierādījumi. Faktiskā secība pārbaudīta atsevišķajos regresiju auditos.

| Atlases pazīme pirms šīs partijas labojumiem | Ieslēgtie ieraksti | Ar sākuma aktiera vai objekta novietojumu |
| --- | ---: | ---: |
| Kvesta priekšmeta ieguve vēl neizsekota | 134 | 3 |
| Aktiera vai kredīta ceļš vēl neizsekots | 92 | 18 |
| Atsevišķa loot rinda neatbilst objekta atslēgai | 38 | 24 |
| Lietošanas mērķa ierobežojums jāpārbauda | 109 | 68 |
| Kvesta loot izņemšana pēc atmešanas jāpārbauda | 530 | 437 |

Pazīmes pārklājas. Aktiera novietojums vēl neapstiprina pieejamu piedāvājumu. Priekšnoteikumi, specializācija, atslēgti iepriekšējie kvesti, fāzes un aktuālais pasākums var mainīt pieejamību. Piemēram, `319` ir agrākās `317 → 318 → 319 → 320` ķēdes ieraksts. Sākums `317` ir atslēgts, `319` vajadzīgajam `1127` nav spawnu, un Blizzard kvestu izņēma agrāk par Legion. Classic spawni šai ķēdei nav pārnesti. [Klasiskās ķēdes statuss](https://www.wowhead.com/ptr/quest=319/a-favor-for-evershine)

Mathystra Relics `951` nav sākuma sasaistes vai relikviju objektu spawnu. Relikvijas un kvests izņemti 4.0.3a, tāpēc to neesamība atbilst Legion pasaulei. [Mathystra Relics vēsture](https://warcraft.wiki.gg/wiki/Mathystra_Relics)

`29433` trofeju ceļš atjaunots un pārbaudīts serverī, ieskaitot aktīvo kvestu, grupu, mantas atrašanos somā un nodošanu. [Darkmoon pārbaude](darkmoon-test-your-strength-2026-11-22.md)

Abiem `37644/37645` piedāvājumiem devēji ir tikai Draenor garnizona kartēs `1153/1159/1330/1331`. To izveides kvesti `34378/34586` sākas 90. līmenī, un Pet Menagerie būves kvesti `36483/36662` sākas 98. līmenī. Šie ieraksti ir ārpus 1.–20. līmeņa spēlētāja parastā ceļa.

`13968` atjaunots Tortusk pasaules novietojums. Servera pārbaude apstiprināja atrašanās vietu, nogalināšanu un progresu mājdzīvnieka īpašniekam. [Tortusk pārbaude](tortusk-thorn-hill-2026-11-22.md)

Eksports un atlases pārskats ir lokāli `.codex/quest-coverage-20261121/exports-clean/` un `.codex/quest-coverage-20261121/ranked-survey/`. Tajos ir `quests.jsonl`, `candidates.tsv` un kopsavilkums ar izmantoto SQL, DB2 un Release SHA-256. Pārskats tagad uzskaita faktiskās atmešanas metadatu saites, jo iepriekš labotais pamata kods vairs neizsauc noņemšanu divreiz. Abi datu avoti nolasīti tikai lasāmās transakcijās, izmantojot izolētās datubāzes iestatījumus.

`24671` novērsta Remote Control Fireworks izmantošana uz cita tipa mērķa un atkārtota viena Wild Clucker ieskaitīšana. [Cluster Cluck pārbaude](cluster-cluck-2026-11-22.md)

`24864` klikšķis un kredīts piesaistīts faktiskajam pievilinātājam, un noņemta nepabeigtajam kvestam paredzētā aura pēc pabeigšanas vai atmešanas. [Irresistible Pool Pony pārbaude](irresistible-pool-pony-2026-11-22.md)

`14021` atjaunots personīgs Frightened Miner un ratiņi katram spēlētājam. Servera pārbaudē abi spēlētāji izmantoja vienu Foreman un vienu rūdas novietojumu, un katrs saņēma tikai savu maršruta progresu. [Miner Troubles pārbaude](miner-troubles-2026-11-22.md)

Pēc šiem labojumiem 2026-11-22 aptvere pārrēķināta no atkārtota pasaules un hotfix eksporta. Izlasē joprojām ir 5 459 ieraksti, no kuriem 4 865 ir ieslēgti. Gala Release failā atrasti visi 155 sasaistītie skriptu nosaukumi. Abi ieraksti ar devēju pasaulē un statiski neizsekotu priekšmeta avotu ir Draenor garnizona `37644/37645`. Vēl jāpārskata 17 statiski neizsekoti aktiera vai kredīta ceļi un 66 priekšmetu lietošanas gadījumi starp ierakstiem ar atrastu devēja novietojumu. Šie saraksti ietver arī ceļus, ko pārskats nespēj izsekot caur C++ izsaukumiem. Jaunās datu attiecības un pārskats ir lokāli `.codex/quest-coverage-20261122/exports/` un `.codex/quest-coverage-20261122/survey/`.

`13842` atjaunota secība no Duraka sarunas līdz viņa transformācijai un faktiskai nogalināšanai. Servera pārbaude apstiprināja abas atzīmes un grupas biedra mājdzīvnieka pēdējo sitienu. [Dread Head Redemption pārbaude](dread-head-redemption-2026-11-22.md)

`13975` atjaunots Rocco kodo brauciens no aktīvā kvesta līdz ierašanās kredītam, izkāpšanai un priekšmeta nodošanai. Divu spēlētāju servera pārbaudē transportlīdzekļi, kredīts un atmešana palika piesaistīti konkrētajam braucējam. Klientā vēl jāpārbauda pilnais brauciens un kauja. [Crossroads Caravan Delivery pārbaude](crossroads-caravan-delivery-2026-11-22.md)
