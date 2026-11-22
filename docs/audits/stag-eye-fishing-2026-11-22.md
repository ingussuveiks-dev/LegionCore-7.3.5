# A Staggering Effort (26557)

Razgar `43239/264856` piedāvā kvestu Orgrimmar makšķerēšanas dienas rotācijā `5680`. Vajadzīgo Stag Eye `58949` dod Weakened Mosshoof Stag `35096` Azsharā. Pārbaudītajā pasaules datubāzē ir 50 šo briežu novietojumi. Brieža laupījuma rinda prasa aktīvu kvestu. Stag Eye sāk spēles burvestību `80868` un patērē vienu priekšmetu.

Valley of Wisdom laupījuma tabulai `5171` jau bija ēsmas auras nosacījums. Orgrimmar rezerves tabula `1637` deva Sandy Carp `58946` ar 2,8042% iespēju arī bez šīs auras. Pārbaude ar sākotnējiem datiem atkārtoja nepareizo lomu, izmantojot īstu Valley of Honor ūdens novietojumu un servera pludiņa laupījuma ceļu.

Migrācija izņem Sandy Carp no pilsētas rezerves tabulas. Valley of Wisdom loms prasa aktīvo kvestu un Stag Eye auru. Ēsmu lieto Orgrimmar, kamēr kvests ir nepabeigts. Tās iedarbība beidzas, kad spēlētājs pamet pilsētu, atmet kvestu vai saņem prasīto zivi. Kvesta priekšmetu noņemšanas metadati tagad aptver gan Stag Eye, gan jau noķertu Sandy Carp.

## Servera pārbaude

- `Test-QuestObjectLootNative.ps1 -StaggeringEffortBaseline` sākotnējos datos reproducēja zivs iegūšanu Valley of Honor bez ēsmas. `-StaggeringEffort` pārbaudīja 640 rezerves laupījuma ģenerēšanas reizes pēc labojuma. Parastās zivis joprojām parādījās, un Sandy Carp rezerves lomā neparādījās.
- Pārbaudīta Razgar pieņemšana, atteikums lietot Stag Eye bez kvesta, viena ēsmas priekšmeta patēriņš un nepareizās apakšzonas noraidīšana ar aktīvu auru. Atmetot kvestu Valley of Honor, aura beidzās. Atkārtotā mēģinājumā bija vajadzīga jauna ēsma.
- Ar īstu Valley of Wisdom pludiņa laupījumu un laupījuma paņemšanas paketi iegūta viena Sandy Carp. Atmešanas pārbaude izņēma zivi un rezerves Stag Eye. Pēc atkārtotas pieņemšanas Razgar pieņēma jaunu lomu un kvestu ieskaitīja dienas rotācijā.
- Brieža laupījuma avotu un tā nosacījumu pārbaudīja datos. Ēsmas lietošanas pārbaudei priekšmetu sagatavoja spēlētāja somā. Faktiska brieža nogalināšana, pilns makšķerēšanas kanāls un klienta izspēle paliek atsevišķi pārbaudāmi.

## Izvietošana un diagnostika

Migrācija `2026_11_22_51_stag_eye_fishing.sql` divreiz pārbaudīta izolētajā MariaDB. Darba datubāzē 45 salīdzinātās rindas sakrita ar pārbaudītajiem datiem. Updater ieraksts ir `RELEASED`, SHA-1 `CE95F4C5FA3DCAEE43544CCD5DC87BCF37930963`.

Kanoniskais Release ar šiem datiem sasniedza gatavību un korekti izslēdzās. Binārā faila SHA-256 ir `C2892B81E56DA543C03566B421AFF2237A51039162EA39C59C6EF0B42B0DE052`. Jaunu avāriju, `DBErrors.log` ierakstu, brīdinājumu vai assertion kļūdu nebija. Ar pārbaudītām kontrolsummām arhivēti 156 faili mapē `build-extractors/bin/Release/fixed/stag-eye-fishing-2026-11-22/`. Pagaidu testu failu tīrīšana atbrīvoja 749 761 210 baitus. Koplietotā izolētā datubāzes kopija saglabāta nākamajiem audita gadījumiem.
