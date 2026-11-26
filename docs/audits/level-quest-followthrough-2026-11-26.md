# 1.–20. līmeņa kvestu turpinājums

Labotas Hallow’s End `39657` un `43055` nodošanas, Aysa pēdējais `30767` eskorta posms, `47430` trīs novērošanas un karte, kā arī `32470` un `32471` kartes punkti. Native pārbaudes un darba datubāzes nolasījumi dokumentēti katra labojuma auditā. [Hallow’s End](hallows-end-candy-buckets-2026-11-26.md) · [Risking It All](wandering-isle-thorn-deck-descent-2026-11-26.md) · [Moonkin Monitoring](moonkin-monitoring-observation-2026-11-26.md) · [Moonglade atzīmes](moonkin-monitoring-markers-2026-11-26.md) · [Light Camera Action](light-camera-action-markers-2026-11-26.md)

Darba eksports aptver 5 459 ierakstus, tostarp 4 865 ieslēgtus un 594 atslēgtus. Visi 174 sasaistītie skriptu nosaukumi atrasti darba EXE `AF0CA95838BDC4F24D1FF1EC7E818FE60A5A946AC19BAEB43BCA656D9B4EEDE6`. Statiskais pārskats joprojām atzīmē 17 aktiera vai progresa un 27 priekšmeta lietošanas ceļus ar novietotu piedāvājumu. Daudzus no tiem izpilda C++ skripti, kuru konkrētās pārbaudes uzskaitītas pārklājuma auditā. Pilna 4 865 kvestu izspēle un vizuālās pārbaudes WoW klientā paliek atvērtas. [Pārklājuma audits](level-1-20-coverage-followup-2026-11-21.md) · [Atkārtotā darba atlase](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/final-survey/summary.json)

## Atlikušās kartes rindas

| Kvests | Pārbaudes rezultāts |
| --- | --- |
| `31035` Enemies Below | Vietējais teksts un nākamais kvests `26858` sakrīt ar veco Cataclysm variantu pie Stone Guard Kurjack `44217`. Šo variantu izņēma 5.0.4, un darba Ragefire Chasm ir jaunās ķēdes saņēmējs Commander Bagran. [Kurjack ķēdes vēsture](https://warcraft.wiki.gg/wiki/Enemies_Below_%28Cataclysm_Undercity%29) |
| `41016` BC TL | Nomināli 100. līmeņa ieraksts ar `133955` objektīvu, bez piedāvājuma un ārpus pasaules esoša T.Q. Bret `102735`. Parasts Legion ceļš vēl nav apstiprināts. [Kvesta kataloga ieraksts](https://www.wowhead.com/quest=41016/bc-tl) |

`47251/47252` jubilejas vēstules, abi vēsturnieki un kartes punkti ir atjaunoti. `47895` atjaunots Bert piedāvājums, pretinieku komanda, derīgās uzvaras ieskaite un kartes punkti. Abu remontu gala pārbaudes ir dokumentētas katra kvesta auditā. [Jubilejas vēstules](legion-anniversary-letters-2026-11-26.md) · [Bert’s Bots](berts-bots-2026-11-26.md)

Foreman Oslow pamatpunkti darbojas. Papildu grupu `2/3/4` 26972 koordinātu avots joprojām nav apstiprināts. `319` pieder izņemtai Evershine ķēdei. `46064` mērķis pieder Legion 110. līmeņa March of the Tadpoles, un `37644/37645` izpilde prasa Draenor garnizona un Pet Menagerie priekšnoteikumus. [Iepriekšējā kartes pārbaude](level-native-poi-markers-2026-11-26.md) · [Tvēruma pārbaude](level-1-20-coverage-followup-2026-11-21.md)

## Diagnostika un tīrīšana

Darba serveris un izolētie serveri sasniedza gatavību un korekti apstājās. Gala pārbaudēs nav jaunu avāriju, DBErrors vai servera brīdinājumu. Izolētās MariaDB žurnālā 12 pārtraukta savienojuma brīdinājumi sakrīt ar divām agrīnām Hallow’s End zondes avārijām. Labotajās zondēs tie neatkārtojās. MariaDB žurnāls apstiprina parastu izslēgšanu, un sākotnējais brīdinājumu fails ir arhivēts. Iepriekšējais SmartScript ieraksts palika darba `DBErrors.log`. [Diagnostikas arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/manifest.json)

Pēdējā darba palaišanā automātiski reģistrēta arī koplietotajā SQL mapē paralēli pievienotā `2026_11_26_33_dreadlords_prize_portal.sql`. Tās spēles ceļš šajā uzdevumā netika pārbaudīts. [Updater nolasījums](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-followthrough/same-date-world-updater.tsv)

Pārskatīto veco būvējumu un šī uzdevuma ģenerētie faili bija 228 faili ar kopējo loģisko izmēru 11 008 793 561 baits. Jaunākie atkopšanas pāri un tagadējā darba versija paliek pieejami. [Veco testa būvējumu tīrīšana](old-level-testservers-cleanup-2026-11-26.md)

## Gala pārbaude pēc sezonas labojumiem

Atjaunots `47895` un abi jubilejas vēstuļu pāri `47251/47252` un `43471/43472`. Pārbaudītas arī abu vēsturnieku ar vienu pareizu atbildi izpildāmās dienas kvesta versijas `43323/43461`. Izņemtas kopš Cataclysm un Legion vairs nepiedāvāto `325/8329` sākuma saites. [Bert](berts-bots-2026-11-26.md) · [Jubilejas vēstules](legion-anniversary-letters-2026-11-26.md) · [Dienas kvesti](legion-anniversary-trivia-2026-11-26.md) · [Vecie piedāvājumi](retired-low-level-offers-2026-11-26.md)

Gala darba atlase aptver 5 459 ierakstus, tostarp 4 865 ieslēgtus un 594 atslēgtus. Visi 175 sasaistītie skriptu nosaukumi atrasti laidienā `79AFE6388DE916BDFF2B46D2579B69936265152A07B9E79E585A5C0D5DF4C411`. Atlikušās 34 novietojuma atzīmes ar atrastu piedāvājumu ietver papildu aktieru rindas, kurām trūkst novietojuma. `7061` izmanto Winter Veil pasākuma `2` piedāvājumu un Baine. `12409` izmanto sezonālo Outland konfekšu trauku. `43412` ir Legion dēmonu mednieka klases zāles kvests ar `44214` priekšnoteikumu. `38300` ir Draenor garnizona petu dienas kvests. [Gala atlase](../../build-extractors/bin/Release/fixed/2026-11-26-retired-low-level-offers/coverage-final/summary.json) · [Draenor kvests](https://www.wowhead.com/quest=38300/critters-of-draenor)

`14097` ir Gilneas ieraksts ar Gwen Armstead saņēmēju un `UNAVAILABLE` karogu. `37474` ir Basics First ieraksts ar Audrey Burnhep tekstā un saņēmēja saitē. Abiem avotos trūkst parastās pieņemšanas ceļa un mērķu. `41016` joprojām trūkst apstiprināta 100. līmeņa BC TL piedāvājuma. Oslow glābšanas un nodošanas punkti ir pārbaudīti, papildu `2/3/4` grupām joprojām trūkst 26972 koordinātu avota. Pilna izspēle un attēlojums WoW klientā paliek atvērti. [Pārklājuma audits](level-1-20-coverage-followup-2026-11-21.md) · [Oslow kartes pārbaude](level-native-poi-markers-2026-11-26.md)

Izolētā pasta zonde sākotnēji avarēja, sinhronajā savienojumā izsaucot asinhrono SQL operāciju. Arhīvā saglabāts steks un astoņi no tā izrietošie MariaDB atvienošanās brīdinājumi. Pēc labojuma abās pārbaudēs un darba laidiena palaišanā šie ieraksti neatkārtojās. Agrākais SmartScript ieraksts darba `DBErrors.log` paliek atvērts. Aktīvajā avāriju mapē failu nav. [Sezonas pārbaudes manifests](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-seasonal/manifest.json)

Šajā turpinājumā izņemti 244 sezonas testa artefakti, tostarp aizstātie EXE/MAP, linker un zondes faili. Dzēsto failu loģiskais izmērs bija 6 927 771 639 baiti, un brīvā vieta tajā tīrīšanas reizē pieauga par 6 896 726 016 baitiem. Saglabāts viens jaunākais atkopšanas EXE/MAP pāris un verificēti 381 pārbaudes fails. Papildu pārskatā noņemti 64 atjaunojami eksporta faili pēc to kontrolsummu saglabāšanas. Abi darba servera izpildfaili, konfigurācija un aktīvie žurnāli pēc tīrīšanas pārbaudīti. [Tīrīšanas rezultāts](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-seasonal/cleanup-result.json) · [Pēdējā eksporta tīrīšana](../../build-extractors/bin/Release/fixed/2026-11-26-retired-low-level-offers/cleanup-readonly-exports.csv)
