# Pirmie trīs 1.–20. līmeņa kvestu labojumi

Šie labojumi izriet no [1.–20. līmeņa kvestu audita](level-1-20-quests-2026-11-21.md). Uzstādītais Release binārais fails un abas migrācijas ir pārbaudīti. Faktiska izspēle 7.3.5 klientā vēl jāapstiprina.

## 26873 · Preemptive Methods

Commit `ce57ae5d9`. Wailing Caverns `13874` un `13891` tagad lasa esošo loot tabulu `13891` ar priekšmetu `5339`, `QuestRequired=1` un daudzumu 1. Koplietojamās tabulas `2772` Test Chest `204960`, objektu slēdzene, NPC loot un mērķa prasītās piecas vienības saglabātas. Abu augu atvēršanai serveris pārbauda, vai spēlētājam vēl vajag 5339. Gathering-node variantam atjaunoti kvesta aktivizācijas un mirdzuma karogi.

Izolētajā MariaDB atveidotas sākotnējās saites abiem augiem. Pēc migrācijas abas rāda uz to pašu pareizo ierakstu, un atkārtota izpilde datus nemaina. Faktiskajā `legion_world` pārbaudītas abas saites un nemainītā blakus lādes rinda. Klientā vēl jāpārbauda abas versijas, cits spēlētājs bez kvesta, piektā vienība un priekšmetu nodošana.

## 8490 · Powering our Defenses

Commit `68035dc60`. Sākotnējais priekšmets 22693, burvestība 28247, runestone fokuss 1363 un summon properties 64 saglabāti. Pirms publicēšanas kristāls saņem tieši izsaucēja aktīvā kvesta tvērumu. Otra dzīva kristāla izsaukšana tam pašam mēģinājumam ir liegta. Kristāls sāk divus trīs wraith viļņus pēc vienas un 31 sekundes. Tam 60 sekundes jāpārdzīvo ar savu dzīvu, tuvumā esošu izsaucēju, lai konkrētais spēlētājs saņemtu dzinēja 16364 kredītu. Saņemt atļauts vienu kredītu. Nodošanai vajag arī līdzi esošo kristālu.

Kristāla bojāeja, neatrisināts izsaucējs, pārtraukts kvesta tvērums un kļūme viļņa izsaukšanā atstāj iespēju sākt no jauna. Taimera beigās tiek izsaukta parastā mērķa uzskaite, kas pārbauda aktīvo kvestu un pabeigšanas prasības. SQL noņēma tieši 8490 novecojušo eksplorācijas karogu un divas mantotās SmartAI darbības, kas piešķīra visu kvestu vai izgāza to kristāla nāves brīdī. Saglabātam `COMPLETE` bez 16364 kredīta jāizspēlē aizsardzība ar esošo priekšmetu. Vecais statuss nav pietiekams atlīdzībai.

`Test-PoweringOurDefenses.ps1` kompilē tieši izmantoto C++ izsaukšanas atlasi un AI kodu ar kontrolētu spēlētāja, spawnu un laika modeli. Pārbaudītas precīzās viļņu robežas, liels laika solis, dublēts izsaukums, īpašnieks, atmešana, nederīgs tvērums, aiziešana, kristāla un īpašnieka nāve, neveiksmīgs wraith spawns un mēģinājums ar veco pabeigto statusu. Klientā vēl atsevišķi jāpārbauda atrašanās vieta, redzamība diviem spēlētājiem, kustība, īsta cīņa un peta vai guardian sitieni, runestone efekts un atlīdzība.

## Kvesta priekšmetu noņemšana

Commit `4cb238dbd`. Atmešanas pakete izsauc source-item tīrīšanu vienreiz pirms aktīvā kvesta noņemšanas. Ja nodošanā vai neveiksmē vairākas kvesta lomas norāda uz vienu priekšmeta ID, noņem lielāko attiecīgās lomas daudzumu. Kvesta daudzumu vairs nesaskaita vairākas reizes par vienu un to pašu priekšmeta kopumu. Saglabāti parastie item-starter un unequip izņēmumi, norādītais visu papildu vienību tīrīšanas limits un saglabāšanas karogu nozīme.

`Test-QuestAttemptCleanup.ps1` pārbaudīja vienreizēju atmešanas izsaukumu, sakrītošus mērķa, source un loot daudzumus, atlikušo priekšmetu skaitu, abus saglabāšanas karogus, item-starter izņēmumu, neaizskartu citu priekšmetu un aprīkojumu, kuru spēlētājs nevar noņemt. Pārbaudīta arī veca atlikta callback anulēšana pirms atkārtotas kvesta pieņemšanas.

## Izvietošana un diagnostika

- MSBuild pārbūvēja pastāvīgajās CMake mapēs esošos `game.lib` un `scripts.lib`. Final link komanda un linker karte pārbaudītas. Uzstādītā `worldserver.exe` SHA-256 ir `c4306b8a8ef64ee4254b98c6a523392f63c73a5988472a57087313535f0f3e98`.
- Gan kandidāts, gan kanoniskais binārais fails sasniedza gatavību, izmantojot izolētus world un hotfix datus ar testa auth un character shēmām. Abi serveri korekti izslēdzās ar kodu 0. Jaunu crash failu, DBErrors ierakstu vai neizskaidrotu worldserver brīdinājumu nebija. Izmantotā Release konfigurācija un bnetserver binārais fails saglabāti.
- Pārbaudītās migrācijas uzstādītas faktiskajā `legion_world` vienā InnoDB transakcijā. Nolasījums sakrita ar izolēto pārbaudi. Updater ierakstos `27` SHA-1 ir `b841554ea8451675963b359a5e822197a7ff4337` un `28` SHA-1 ir `3dfb41f15bee65345f73d48e410440ddaeebd04c`. Uzdevuma MariaDB process korekti izslēdzās. Kanoniskā crash inventarizācija un žurnāli pēc importa sakrita ar pirmsimporta stāvokli.
- Lokāli saglabāti nolasījumi un diagnostika `.codex/level-quest-fixes-20261121/`. Saskaņā ar `cleanup-manifest.csv` izņemti 166 607 301 baiti pagaidu bināro un būvēšanas failu. Saglabāts iepriekšējais binārais fails un linker karte atkopšanai. Iepriekšējā audita apturētā MariaDB kopija tika izmantota atkārtoti. Tās tīrīšana joprojām atlikta iepriekšējā automātiskās atļauju pārbaudes noraidījuma dēļ.
