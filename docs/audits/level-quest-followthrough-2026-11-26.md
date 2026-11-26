# 1.–20. līmeņa kvestu turpinājums

Labotas Hallow’s End `39657` un `43055` nodošanas, Aysa pēdējais `30767` eskorta posms, `47430` trīs novērošanas un karte, kā arī `32470` un `32471` kartes punkti. Native pārbaudes un darba datubāzes nolasījumi dokumentēti katra labojuma auditā. [Hallow’s End](hallows-end-candy-buckets-2026-11-26.md) · [Risking It All](wandering-isle-thorn-deck-descent-2026-11-26.md) · [Moonkin Monitoring](moonkin-monitoring-observation-2026-11-26.md) · [Moonglade atzīmes](moonkin-monitoring-markers-2026-11-26.md) · [Light Camera Action](light-camera-action-markers-2026-11-26.md)

Darba eksports aptver 5 459 ierakstus, tostarp 4 865 ieslēgtus un 594 atslēgtus. Visi 174 sasaistītie skriptu nosaukumi atrasti darba EXE `AF0CA95838BDC4F24D1FF1EC7E818FE60A5A946AC19BAEB43BCA656D9B4EEDE6`. Statiskais pārskats joprojām atzīmē 17 aktiera vai progresa un 27 priekšmeta lietošanas ceļus ar novietotu piedāvājumu. Daudzus no tiem izpilda C++ skripti, kuru konkrētās pārbaudes uzskaitītas pārklājuma auditā. Pilna 4 865 kvestu izspēle un vizuālās pārbaudes WoW klientā paliek atvērtas. [Pārklājuma audits](level-1-20-coverage-followup-2026-11-21.md) · [Atkārtotā darba atlase](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/final-survey/summary.json)

## Atlikušās kartes rindas

| Kvests | Pārbaudes rezultāts |
| --- | --- |
| `31035` Enemies Below | Vietējais teksts un nākamais kvests `26858` sakrīt ar veco Cataclysm variantu pie Stone Guard Kurjack `44217`. Šo variantu izņēma 5.0.4, un darba Ragefire Chasm ir jaunās ķēdes saņēmējs Commander Bagran. [Kurjack ķēdes vēsture](https://warcraft.wiki.gg/wiki/Enemies_Below_%28Cataclysm_Undercity%29) |
| `41016` BC TL | Nomināli 100. līmeņa ieraksts ar `133955` objektīvu, bez piedāvājuma un ārpus pasaules esoša T.Q. Bret `102735`. Parasts Legion ceļš vēl nav apstiprināts. [Kvesta kataloga ieraksts](https://www.wowhead.com/quest=41016/bc-tl) |
| `47251/47252` Interesting Times | `147881/147880` ir priekšmetu sākuma saites. Projektā jāatjauno 2017. gada jubilejas pasta piegāde un abu vēsturnieku novietojumi un fāzes. Darba datubāzē nav jubilejas pasākuma vai aktieru novietojumu. [Blizzard 2017. gada jubilejas apraksts](https://worldofwarcraft.blizzard.com/en-us/news/21187747/comemore-13-anos-de-world-of-warcraft) |
| `47895` Bert's Bots | Palikusi saņēmēja saite uz `124617` un petu kaujas mērķis. Darba pasaulē nav izpildāmās veidnes, sākuma saites, novietojuma vai `battlepet_npc_team_member` ierakstu. Jāatgūst atbilstošie 26972 aktiera un komandas dati. [Bert's Bots kataloga ieraksts](https://www.wowhead.com/ptr-2/quest=47895/berts-bots) |

Vietējā TrinityCore vēsturē atrasti `28153` jubilejas un Bert kartes punkti. Jubilejas kartes lapu ID tajos atšķiras no vietējā 26972 ieraksta, un Bert papildu grupai mainīta lapa un stāvs. Šie ieraksti palīdz atrast iespējamos novietojumus. Tajos nav apstiprināta 26972 aktieru publicēšana, jubilejas pasta secība vai Bert petu komanda.

Foreman Oslow pamatpunkti darbojas. Papildu grupu `2/3/4` 26972 koordinātu avots joprojām nav apstiprināts. `319` pieder izņemtai Evershine ķēdei. `46064` mērķis pieder Legion 110. līmeņa March of the Tadpoles, un `37644/37645` izpilde prasa Draenor garnizona un Pet Menagerie priekšnoteikumus. [Iepriekšējā kartes pārbaude](level-native-poi-markers-2026-11-26.md) · [Tvēruma pārbaude](level-1-20-coverage-followup-2026-11-21.md)

## Diagnostika un tīrīšana

Darba serveris un izolētie serveri sasniedza gatavību un korekti apstājās. Gala pārbaudēs nav jaunu avāriju, DBErrors vai servera brīdinājumu. Izolētās MariaDB žurnālā 12 pārtraukta savienojuma brīdinājumi sakrīt ar divām agrīnām Hallow’s End zondes avārijām. Labotajās zondēs tie neatkārtojās. MariaDB žurnāls apstiprina parastu izslēgšanu, un sākotnējais brīdinājumu fails ir arhivēts. Iepriekšējais SmartScript ieraksts palika darba `DBErrors.log`. [Diagnostikas arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/manifest.json)

Pēdējā darba palaišanā automātiski reģistrēta arī koplietotajā SQL mapē paralēli pievienotā `2026_11_26_33_dreadlords_prize_portal.sql`. Tās spēles ceļš šajā uzdevumā netika pārbaudīts. [Updater nolasījums](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/same-date-world-updater.tsv)

Pārskatīto veco būvējumu un šī uzdevuma ģenerētie faili bija 228 faili ar kopējo loģisko izmēru 11 008 793 561 baits. Jaunākie atkopšanas pāri un tagadējā darba versija paliek pieejami. [Veco testa būvējumu tīrīšana](old-level-testservers-cleanup-2026-11-26.md)
