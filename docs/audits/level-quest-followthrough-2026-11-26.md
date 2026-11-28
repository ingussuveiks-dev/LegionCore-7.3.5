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

Cataclysm TDB apstiprina, ka `14097` tika izņemts no aprites. `37474` Basics First joprojām trūkst apstiprināta sākuma avota. `41016` katalogā ir 100. līmeņa BC TL kvests ar sešiem izdrukātiem Westfall punktiem un bez atrasta piedāvājuma. Visas piecas Oslow kartes grupas tagad ielādējas ar Legion izdrukās pārbaudītajām koordinātām. Pilna izspēle un attēlojums WoW klientā paliek atvērti. [Sākuma avotu pārbaude](low-level-quest-source-review-2026-11-26.md) · [Oslow kartes pārbaude](foreman-oslow-secondary-markers-2026-11-26.md)

Izolētā pasta zonde sākotnēji avarēja, sinhronajā savienojumā izsaucot asinhrono SQL operāciju. Arhīvā saglabāts steks un astoņi no tā izrietošie MariaDB atvienošanās brīdinājumi. Pēc labojuma abās pārbaudēs un darba laidiena palaišanā šie ieraksti neatkārtojās. Agrākais SmartScript ieraksts darba `DBErrors.log` paliek atvērts. Aktīvajā avāriju mapē failu nav. [Sezonas pārbaudes manifests](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-seasonal/manifest.json)

Šajā turpinājumā izņemti 244 sezonas testa artefakti, tostarp aizstātie EXE/MAP, linker un zondes faili. Dzēsto failu loģiskais izmērs bija 6 927 771 639 baiti, un brīvā vieta tajā tīrīšanas reizē pieauga par 6 896 726 016 baitiem. Saglabāts viens jaunākais atkopšanas EXE/MAP pāris un verificēti 381 pārbaudes fails. Papildu pārskatā noņemti 64 atjaunojami eksporta faili pēc to kontrolsummu saglabāšanas. Abi darba servera izpildfaili, konfigurācija un aktīvie žurnāli pēc tīrīšanas pārbaudīti. [Tīrīšanas rezultāts](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-seasonal/cleanup-result.json) · [Pēdējā eksporta tīrīšana](../../build-extractors/bin/Release/fixed/2026-11-26-retired-low-level-offers/cleanup-readonly-exports.csv)

Pārbaudot Audrey ķēdi, izlabota `On The Mend` ieskaite un abu Stormwind petu dziedināšanas izvēļu maksa. Darba laidiens `8B37E071AE40A6806CA5E1408E239E51D3031D078F77778D6A34C50C5098B41E` izgāja palaišanas pārbaudi un atkārtotās Bert un jubilejas zondes. Atkārtotais audits aptver 5 459 ierakstus un 175 sasaistītos skriptu nosaukumus. [Dziedināšanas audits](on-the-mend-pet-healing-2026-11-26.md) · [Jaunākais eksports](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-closeout/coverage-final/summary.json)

Agrīnai dziedināšanas zondei trūka privātā LoginDatabase savienojuma. Tās izņēmums un četri MariaDB atvienošanās brīdinājumi ir arhivēti. Labotajās zondēs un abās servera palaišanās šie ieraksti neatkārtojās, un privātā MariaDB korekti apstājās. Darba `DBErrors.log` palika iepriekšējais neatrisinātais SmartScript ieraksts. Aktīvajā avāriju mapē nebija failu. [Pārbaudes arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-closeout/manifest.json)

Ar pārbaudītām kontrolsummām saglabāti 269 pārbaudes faili. Izņemti 170 pagaidu faili un divi aizstāta atkopšanas būvējuma faili. Brīvā vieta uz C: šajās tīrīšanas reizēs kopā pieauga par 4 245 512 192 baitiem. Jaunākais atkopšanas pāris ir pārbaudīts. [Tīrīšanas kopsavilkums](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-closeout/cleanup-result.json) · [Aizstātā atkopšanas pāra dzēšanas saraksts](../../build-extractors/bin/Release/fixed/2026-11-26-low-level-closeout/superseded-rollback-cleanup.csv)

## Reģionālā petu apmācība

Pārbaudītas 12 `On The Mend` ķēdes un atjaunoti desmit petu apmācības piedāvājumi. Darba eksports pēc abām migrācijām aptver 5 459 kvestu ierakstus, tostarp 4 865 ieslēgtus un 594 atslēgtus. Visi 175 sasaistītie skriptu nosaukumi atrasti gala laidienā. Atkārtoti salīdzinātajās 17 statiskajās ieskaites atzīmēs 15 ceļiem ir iepriekšējas uzvedības pārbaudes. `319` paliek vēsturiskajā ķēdē ar atslēgtu sākumu `317`, savukārt `46064` ir Legion 110. līmeņa pasākuma kvests. [Petu apmācības pārbaudes](regional-pet-tutorial-2026-11-26.md) · [Gala darba datu pārskats](../../build-extractors/bin/Release/fixed/2026-11-26-regional-pet-tutorial/final-coverage/summary.json)

`37474` un atsevišķajai `31554` versijai trūkst pārbaudīta sākuma avota. Vecais `SmartScript: Action target Creature(entry: 0)` ieraksts palika aktīvajā `DBErrors.log`. Ierakstā nav aktiera vai skripta rindas, un tajā pašā sekundē žurnālā redzamās kaujas rindas neļauj sasaistīt kļūdu ar konkrētu 1.–20. līmeņa kvestu. Gala servera pārbaudēs tas neatkārtojās. [Atvērtā sākuma avota audits](low-level-quest-source-review-2026-11-26.md)

2026-11-27 pārbaudē abi 703.00 arhīva nodošanas ieraksti apstiprināja saņēmējus. Atkārtoti izgāja 24 petu dziedināšanas dialogu zondes. Atsevišķā Violet Hold pārbaudē vecais Meryl portāla skripts atkārtoja SmartScript kļūdas tekstu, bet darba datu variants izgāja pārejas un portāla tīrīšanas servera pārbaudi. [Violet Hold pārbaude](dreadlords-prize-portal-2026-11-26.md#2026-11-27-pārbaude)

## Bloodmyst portāls un pārējie klikšķu ceļi

`9740` darba serverī prasa izslēgt visus četrus oriģinālos Sunhawk kontrolierus. Pieņemšana, atmešana, atjaunošanās un nodošana pārbaudīta ar servera paketēm. Jaunākajā datu eksportā ir 5 459 ieraksti, 4 865 ieslēgti kvesti un 176 gala laidienā atrasti piesaistītie skriptu nosaukumi. [Sun Gate audits](bloodmyst-sun-gate-2026-11-27.md) · [Darba datu izlase](../../build-extractors/bin/Release/fixed/2026-11-27-bloodmyst-sun-gate/coverage/summary.json)

`14476` Rigged to Blow pārbaudīta pieņemšana pēc Grounded, visu trīs oriģinālo lādiņu ieskaite un nodošana pie Smooks. Darba `event_scripts` tabulā nav `22494/22497/22498` rindu. Sargu reakcijai un animācijai joprojām vajadzīgs atbilstošs 7.3.5 pierādījums. [Kvesta secība un sargu reakcijas apraksts](https://warcraft.wiki.gg/wiki/Rigged_to_Blow)

`29364` Corn Mash pieder Aska piecu kvestu dienas rotācijai `5684`. Zondē publicēts rotācijā izvēlēts kvests un pārbaudīta Cooking prasība, sešas īstās bļodas, atkārtota klikšķa atteikums un nodošana. Dienu maiņa un malšanas animācija klientā paliek atsevišķas pārbaudes. [Zondes izvade](../../build-extractors/bin/Release/fixed/2026-11-27-bloodmyst-sun-gate/goober-survey-verified-diagnostics/check-output.log) · [Rotācijas dati](../../build-extractors/bin/Release/fixed/2026-11-27-bloodmyst-sun-gate/goober-live-readback.tsv)

`14477` Push the Button! darba serverī prasa nospiest Goblin Detonator pie Smooks. Atjaunoti oriģinālās pogas, kastes un Gorek novietojumi. Pārbaudīta faktiskā slēdzene, vairāku spēlētāju atsevišķi mēģinājumi un Kroum atlīdzība. Legion video redzamā torņa demonstrācija un `14476` precīzā sargu kustība paliek atvērtas. [Azshara audits](azshara-goblin-detonator-2026-11-27.md)

## Kvesta burvestību nosacījumi

Izvietoti `14005` Vengeance of Elune darbību joslas un `24452` Profitability Scouting ierīces labojumi. Atkārtotā darba atlase pārbaudīja 4 865 ieslēgtus kvestus un 177 skriptu nosaukumus Release failā. `24904` Gilneas City revīzijā identificēts bloķēts rapieris, trūkstošs cīņas sākuma skripts un neizsekota fināla ieskaite. Abu izvietoto labojumu izspēle klientā paliek atvērta. [Burvestību nosacījumu audits](low-level-quest-auras-2026-11-27.md)

## Gilneas fināls un laupījuma avoti

Gilneas `24904` klienta datos atrasta oriģinālā `72349` fināla burvestība, kas satur `38854` ieskaiti. Tās izsaukums, viļņu maršruti un dalībnieku atlase darba versijā joprojām trūkst. [Gilneas avoti](gilneas-battle-sources-2026-11-27.md)

2026-11-28 darba serverī atjaunoti visi septiņi `24904` viļņi un fināls. Pēc kaujas `24902` serverī pārbaudīts no Lorna un Tobiasa izsaukuma cauri katedrāles ainai, atpakaļ pie Lorna un līdz `24903` nodošanai pie Genn. Klienta attēls un kustība paliek atvērti. [Gilneas kauja](gilneas-battle-2026-11-28.md) · [The Hunt for Sylvanas](gilneas-hunt-2026-11-28.md)

`24972` Vital Intelligence pats izsniedz vienu `2885` dokumentu no Sarvis pirms nodošanas Simmer. Neizsekotais `1667` laupījuma ieraksts nav prasība šajā piegādes kvestā. `29482` Transmutation Master prasa Alchemy `475` un četrus `72104` transmutācijas materiālus, lai gan kvestam ir zems nominālais minimums.

`2499` Oakenscowl atkal ir atrodams Teldrassil. `29401/29412` Blown Away atjaunoti 38 balonu novietojumi, un svešs spēlētājs nevar iztukšot balonu. Servera pārbaudē savākti prasītie priekšmeti un pārbaudīta to izņemšana pēc atmešanas. [Novietojumu un laupījuma audits](low-level-world-spawns-2026-11-27.md)
