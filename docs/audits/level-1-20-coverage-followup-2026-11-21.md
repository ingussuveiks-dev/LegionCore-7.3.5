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

`29433` prasītajām Grisly Trophies nav atrasta iegūšanas saite. Kvesta teksts prasa Darkmoon Adventurer’s Guide un nogalināšanu, kas dod XP vai godu. Jāpārbauda pasākuma un grupas nogalināšanas ceļš. `37644/37645` abas devēju versijas ir tikai Draenor garnizona kartēs `1153/1159/1330/1331`. Nulles minimuma vērtība prasa papildus izsekot garnizona pieejamību.

Eksports un atlases pārskats ir lokāli `.codex/quest-coverage-20261121/exports-clean/` un `.codex/quest-coverage-20261121/ranked-survey/`. Tajos ir `quests.jsonl`, `candidates.tsv` un kopsavilkums ar izmantoto SQL, DB2 un Release SHA-256. Pārskats tagad uzskaita faktiskās atmešanas metadatu saites, jo iepriekš labotais pamata kods vairs neizsauc noņemšanu divreiz. Abi datu avoti nolasīti tikai lasāmās transakcijās, izmantojot izolētās datubāzes iestatījumus.
