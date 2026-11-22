# The Grand Magus Doane (26694)

Redridge priekšnoteikums ir `26693`. Keeshan `43611/260620` pieņem `26694` un nodrošina atkārtoti lietojamo Ilgalar Key `59522`. Īstās Ward of Ilgalar durvis `204582/472` izmanto slēdzeni `1931` un atvēršanas burvestību `81776`. Grand Magus Doane `397/39198` ir kvesta faktiskais nogalināšanas mērķis.

Sākotnējā servera pārbaudē pietika ar parastu klikšķi, lai atvērtu durvis bez atslēgas. Tagad durvis prasa īsto līdzi nesto atslēgu un tās burvestību. Pārbaudīts īstais novietojums, fāze, sasniedzamība un aktīvs kvesta mēģinājums. Atslēga darbojas arī pēc cīņas līdz atlīdzības saņemšanai, lai spēlētājs varētu iziet no torņa. Durvju lietošana nemaina Doane ieskaiti.

`Test-QuestObjectLootNative.ps1 -IlgalarWard` pārbaudīja kvesta priekšnoteikumu, atslēgas saņemšanu, atteikumu bez atslēgas, svešu durvju novietojumu, atmešanu un atkārtotu pieņemšanu, īsto atvēršanu, mājdzīvnieka pēdējo sitienu, vienu ieskaiti īpašniekam un atkārtotu atvēršanu pēc kaujas. Keeshan pieņēma īstās atlīdzības paketes izvēli, un atslēga tika izņemta. Sākotnējai zondei bija jāielādē arī īstās Doane laupījuma rindas un jāpabeidz spēlētāja aizkavēto notikumu rinda. Atkārtoti pārbaudīti `BlackrockPrisoners` un `RedridgeSeaforium`.

Darba DB rindas šim labojumam jau bija pieejamas. Kanoniskā `worldserver.exe` SHA-256 ir `F04B6447FC5D2A59A674CE1D84B034DC434F02212E0F594D67FBC6ACF9B439E8`. Klientā jāpārbauda torņa telpas, Doane kaujas un animāciju secība un visa Redridge ķēde.

Kanoniskais un sagatavotais Release izolētajā MariaDB sasniedza gatavību un korekti izslēdzās. Galīgajās pārbaudēs netika konstatētas jaunas avārijas, `DBErrors.log` ieraksti, brīdinājumi vai assertion kļūdas. Ar kontrolsummām arhivēti 167 diagnostikas faili mapē `build-extractors/bin/Release/fixed/ilgalar-ward-2026-11-22/`. Izņemti 3 256 162 517 baiti pagaidu failu, un lokāli paturēta iepriekšējā kanoniskā versija.
