# Ritual Materials Darkshore

10×10 SmartAI audita trīs atradnes bija Moonstalker `33127`, Mottled Doe `33313` un Hungry Thistle Bear `33978`. Visām trim `SMART_EVENT_ON_SPELLCLICK` rindām bija piespiedu pazušana un atsauce uz neesošu `LINK 1`. `Unit::HandleSpellClick` var izsaukt AI arī pēc noraidītas burvestības vai tad, ja priekšmetu nevar ievietot somā. Sākotnējo datu pārbaude atkārtoja pazušanu pie pilnas somas un visus trīs gaidītos `LINK` ierakstus.

Legion 26972 datos burvestības `62512`, `62788` un `64116` lieto esošo `SPELL_EFFECT_CREATE_ITEM` un veido attiecīgi `44969`, `45027` un `45885`. Kvests `13566` prasa trīs no katra. Sākotnējās klikšķa burvestības, izpildītāji, priekšmetu saņemšana un kvesta skaitītāji paliek standarta apstrādē. [Ritual Materials kvesta apraksts](https://www.wowhead.com/quest=13566/ritual-materials)

2016. gada Legion ieraksta pilnā 5 minūšu un 52 sekunžu secība pārskatīta divu sekunžu kadros no pieņemšanas līdz nodošanai. Izšķirošie fragmenti pārskatīti vienas sekundes kadros. Ap `0:13`, `0:43` un `0:50` ūsas ieskaitītas pirms Moonstalker uzbrukuma. Ap `1:32–1:37` saņemtas spalvas, kamēr Mottled Doe paliek redzamas. Ap `3:58` un `4:11–4:13` saņemta lāča spalva, kamēr dzīvnieks paliek dzīvs. Cīņas, ko ierakstā uzsāka vai turpināja spēlētājs, pārbaudītas atsevišķi no priekšmeta saņemšanas. [Legion ieraksts](https://www.youtube.com/watch?v=o5bK2mx2gRE)

## Labojums

- Jaunā apstrāde darbojas tikai trim nemainīgajiem Darkshore dzīvnieku avotiem kartē `1`, zonā `148`. Klikšķis saglabā jau esošo nepabeigtā kvesta nosacījumu. Tuvuma un redzamības pārbaude notiek burvestības izpildē. Veiksmīga savākšana tiek reģistrēta tikai pēc priekšmeta skaita pieauguma spēlētāja līdzi nēsātajā inventārā.
- Pēc veiksmīgas savākšanas tas pats dzīvnieks šajā kvesta mēģinājumā vairs nepiedāvā paraugu tam pašam spēlētājam. Citiem spēlētājiem paraugs paliek pieejams ārpus kaujas un Moonstalker pamošanās brīža. Aktīvais kvesta mēģinājums saistīts ar esošo atcelšanas apliecinājumu, tādēļ iepriekšējais mēģinājums neaiztur atkārtotu pieņemšanu. Dabiska dzīvnieka atjaunošanās atiestata tā paraugus.
- Moonstalker pēc veiksmīgas savākšanas pamostas ar vienas sekundes aizkavi un uzbrūk tuvumā esošajam vācējam. Aizkave atbilst `62512` blakusefekta ilgumam un ierakstā novērotajai secībai. Briežu mātīte un lācis paliek dzīvi. Atsevišķs lāča makšķerēšanas logs un papildu reakciju vizuālais noformējums vēl jāizpēta. Esošais `13885` lidojums izmanto citus tēlu ID.

## Regresijas pārbaude

- `Test-QuestObjectLootNative.ps1 -RitualMaterialsBaseline` ar sākotnējiem servera datiem apstiprināja, ka pilna soma nesaņem priekšmetu un SmartAI tomēr noņem katru no trim dzīvniekiem. Katram atkārtojās tieši viens jau iepriekš atrastais `LINK` ieraksts. Šis bija sagaidāmais kļūdas atkārtojums.
- `-RitualMaterials` izmantoja sākotnējos novietojumus un spēles servera klikšķa paketi. Pārbaudīja visas trīs pilnas somas situācijas, atkārtotu mēģinājumu pēc vietas atbrīvošanas, klikšķi bez kvesta, otro spēlētāju ar savu paraugu no tās pašas briežu mātītes, kvesta atmešanu un atkārtotu pieņemšanu, Moonstalker pamošanos, 3/3 katram priekšmetam un nodošanas atļauju pie Keeper Karithus. Spēles klients, konkrēta lāča makšķerēšanas secība un pati kvesta balvas saņemšana šajā pārbaudē vēl paliek atvērtas.
- `-DarkshoreWildlifeFlight` pārbaudīja iepriekšējo `13885` plūsmu ar divām sēdvietām, lidojumu, manuāliem burvestības mērķiem, trim 8/8 skaitītājiem, nolaišanos, nodošanu un atmešanu ar atkārtotu mēģinājumu. Gan šī, gan `-RitualMaterials` pārbaude beidzās ar kodu 0 un bez jaunām avārijām, `DBErrors` ierakstiem, brīdinājumiem vai assert ierakstiem.

## Izvietošana

- Gala saistītājs iekļāva `zone_darkshore.obj` no pārbūvētā `scripts.lib` ar SHA-256 `0AE8B1E4C02E561862E0E0D1A1D0702EBE2B81E035387CA84EE6B953F15C7236`. Sagatavotais un kanoniskais `worldserver.exe` sakrīt ar SHA-256 `755CF844C7DEEF7D9355FC406CE031A8171BF90DDE6DD2EAE057AA98D86FCB02`.
- Migrācija `2026_11_28_13_ritual_materials_pickup.sql` privātajā datubāzē izpildīta divreiz. Kanoniskajā datubāzē apstiprināta `RELEASED` rinda ar SHA-1 `BC9A546320E3F669135E48D27C11CAC712FEFABD`, trīs NPC piesaistes, trīs burvestību piesaistes, sākotnējās klikšķa prasības ar jauno parauga pārbaudi un trīs veco rindu noņemšana. Kanoniskais serveris sasniedza gatavību un korekti izslēdzās ar kodu 0. Jaunu avāriju, `DBErrors` ierakstu vai brīdinājumu nebija. Privātā MariaDB izslēdzās ar kodu 0.
- Arhīvā `build-extractors/bin/Release/fixed/2026-11-28-ritual-materials/` saglabāti diagnostikas oriģināli, SHA-256 manifests, dzēsto failu saraksts un atgriešanas kopija. Izdzēsti 1 277 870 999 baiti atjaunojamu failu. Esošais 1347 baitu jauktais `DBErrors.log` saglabāts, un kanoniskā avāriju mape ir tukša.
