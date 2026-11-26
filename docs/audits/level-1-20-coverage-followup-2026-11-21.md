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

`9670` statiski neizsekoto ceļu pārbaudījām ar esošo `mob_webbed_creature` skriptu. Bloodmyst Isle ir 39 Webbed Creature `17680` novietojumi, un Researcher Cornelius `17686` gan piedāvā, gan pieņem kvestu. `Test-QuestObjectLootNative.ps1 -WebbedResearcher` pārbaudīja nejaušo Expedition Researcher `17681` izsaukumu pēc īstas nogalināšanas, kredītu spēlētājam un mājdzīvnieka īpašniekam, piecus glābšanas gadījumus un jaunu pieņemšanu pēc atmešanas. Kvestam nav nododama priekšmeta. Vēsturiskajos avotos grupas kredīta noteikumi palika neapstiprināti. [Bloodmyst Isle vēsturiskais ceļvedis](https://www.wow-pro.com/source-code-bloodmyst-isle/)

`29421` Master Li Fei duelis tagad pieder vienam kvesta mēģinājumam. Native pārbaude apstiprināja cita spēlētāja sitiena atteikumu, atmešanu, mājdzīvnieka izraisītu padošanos un dueli zaudējušā spēlētāja statusu. [Only the Worthy Shall Pass pārbaude](li-fei-duel-2026-11-22.md)

Atkārtots eksports pēc šiem labojumiem joprojām atlasīja 5 459 ierakstus, no kuriem 4 865 ir ieslēgti. Kanoniskajā Release atrasti visi 157 ar sākotnējo pasauli sasaistītie skriptu nosaukumi. Eksports un pārskats glabājas lokāli `.codex/quest-coverage-20261122-current/`. Statiskais skripts joprojām rāda 17 neizsekotus aktiera vai kredīta ceļus un 66 pārskatāmus priekšmetu lietojumus ar pasaulē novietotu devēju. Pirmajā skaitlī paliek arī šajā auditā pārbaudītie C++ ceļi.

`46064` ir March of the Tadpoles pasākuma `305` kvests, kam šajā datubāzē mērķis Mrrga `118662` ir 110. līmeņa. Tajā joprojām trūkst Mrrga pasaules novietojuma un pilna pasākuma ceļa. Pasākums jāskata ar Legion 110. līmeņa datiem. [Legion pasākuma apraksts](https://www.worldofmoudi.com/micro-holiday-march-of-the-tadpoles)

`29787` atjaunots Shang Xi izsaukums, personīgais sargs, īstas nogalināšanas kredīts un ceļa turpinājums pēc agras uzvaras. [Worthy of Passing pārbaude](shang-xi-escort-2026-11-22.md)

Legion `Lock.db2` ieraksti sasaista `877` sēklas `5068` ar `3737`, `9193` lāpu `22755` ar `181148`, `9544` atslēgu `23801` ar `181714`, `9582` atslēgu `23925` ar `181849` un `9667` atslēgu `24099` ar `181928`. Visi pieci objekti ir novietoti pasaulē. Pamata `CanOpenLock` salīdzina atslēgas priekšmeta ID ar konkrētās slēdzenes ierakstu. Šī pārbaude apstiprina priekšmetu mērķu sasaisti. Faktiskā atvēršana, objekta redzamība un iespējamo priekšmetu izņemšana paliek atsevišķi pārbaudāma.

`264` kulonam pielāgots Yuriv kapakmens īstais slēdzenes tips un atļautais mērķis. Servera pārbaude apstiprināja nodošanas izvēlni un somas tīrīšanu. [Until Death Do Us Part pārbaude](yuriv-pendant-2026-11-22.md)

`13730` tvertne atrod dzīvus Felfire Hill ugunskurus 10 jardu rādiusā un atdod kredītu spēlētājam pēc pilna kanāla. Servera pārbaude sasniedza visas septiņas atzīmes un apstiprināja atlīdzības izvēli, nodošanu un tvertnes izņemšanu. [Playing With Felfire pārbaude](felfire-canister-2026-11-22.md)

`13925` stāds pieņem visas četras paredzētās dzīvnieku sugas un piešķir progresu faktiskajam lietotājam. Nodevēja saņemtā stāda izņemšana un atmešanas ceļš pārbaudīts serverī. [An Ounce of Prevention pārbaude](lifebringer-sapling-2026-11-22.md)

`13918` detektora tuvākais atradums pieejams tikai attiecīgajam kvesta mēģinājumam, arī sadursmju aprēķinos. Servera pārbaude izgāja piecu fragmentu iegūšanu, salikšanu un abu prasīto priekšmetu nodošanu pie Ferd. [The Titans’ Terminal pārbaude](titans-terminal-2026-11-22.md)

Pēc `13918` atkārtotā izolētā datu eksportā bija 5 459 tvēruma ieraksti, 4 865 ieslēgti kvesti un visi 160 sasaistītie skriptu nosaukumi atrodami atbilstošajā Release. Palika 62 priekšmetu lietošanas kandidāti un 17 ar statisko analīzi līdz galam neizsekoti aktiera vai progresa ceļi, kuros ir arī serverī jau pārbaudītie C++ ceļi. Eksports, kontrolsummas un saraksts ir `.codex/quest-coverage-20261122-sealed/`.

`9538` valodas grāmata pieņem aktīvu kvestu, pēc pilnas lasīšanas pabeidz tikai lasītāja kvestu un tiek izlietota. Pie Totem of Coo pārbaudīta nodošana un nākamais piedāvājums `9539`. [Learning the Language pārbaude](stillpine-primer-2026-11-22.md)

`14019` banānus var lietot tikai ar aktīvu kvestu uz dzīvu Bomb-Throwing Monkey. Serverī pārbaudīti desmit pasaules novietojumi, sākotnējais kaujas piesaistes noteikums, atlīdzības izvēle un banānu izņemšana pēc nodošanas vai atmešanas. [Monkey Business pārbaude](monkey-business-2026-11-22.md)

`13961` tīkla trāpījums un izsaukums piesaistīts konkrētajai kvesta pieņemšanas reizei. Serverī pārbaudīti divi neatkarīgi gūstekņi, sekošanas komanda, reālais Togrik pabeigšanas notikums un tīkla izņemšana pēc nodošanas vai atmešanas. [Drag it Out of Them pārbaude](hogtied-razormane-2026-11-22.md)

`13988` Thunderhead Summit karogs ieskaita tikai paša spēlētāja atvesto vai izsaukto ērgli un parāda privātu Tonga garu. Novērsta vecā karoga skripta avārija. Serverī pārbaudīti divi spēlētāji pēc kārtas, izsaukuma atmešana un totema izņemšana pie Jerrik. [A Growing Problem pārbaude](tonga-summit-2026-11-22.md)

Pēc `13988` vēlreiz eksportēta pasaules datubāze un hotfix dati. Izlasē paliek 5 459 ieraksti, no kuriem 4 865 ir ieslēgti. Kanoniskajā Release atrodami visi 160 piesaistītie skriptu nosaukumi. Starp kvestiem ar pasaulē novietotu devēju paliek 58 priekšmetu lietošanas kandidāti un 17 statiski neizsekoti aktiera vai kredīta ceļi. Kandidātu sarakstos ietilpst jau serverī pārbaudīti C++ ceļi. Pārskats un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-summit/`.

`26078` dzēšanas konuss atlasa tikai dzīvas lidlauka ugunis un vienu uguni ieskaita vienreiz. Serverī pārbaudītas sešas sākotnējās ugunis, atteikums ārpus lidlauka un ekipējuma auras un priekšmeta izņemšana. [Extinguish the Fires pārbaude](airfield-firefighting-2026-11-22.md)

`26446` paugurs prasa īsto stādīšanas burvestību un vienu stādu devu par katru ieskaiti. Serverī pārbaudīti astoņi pauguru novietojumi, nederīgs objekts, parastais klikšķis, atmešana stādīšanas laikā, vecais kvesta ieraksts `9526` un nodošana Gnarl. [Reclaiming Felfire Hill pārbaude](felfire-seedlings-2026-11-22.md)

Pēc `26446` atkārtotajā eksportā joprojām ir 5 459 tvēruma ieraksti, 4 865 ieslēgti kvesti un visi 160 sākotnējai pasaulei piesaistītie C++ skriptu nosaukumi atrodami atbilstošajā Release. Pārskatā palika 56 priekšmetu lietošanas kandidāti un 17 ar statisko analīzi neizsekoti aktiera vai progresa ceļi starp kvestiem ar devēja novietojumu pasaulē. Eksports un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-seedlings/`.

`14461` lāpa atlasa trīs faktiskos barības traukus, un iepriekšējās pieņemšanas reizes metiens vairs neieskaitās. Serverī pārbaudīti visi trīs trauki, ieskaites robežas, atmešana un nodošana pie Adana. [Feed of Evil pārbaude](battleboar-troughs-2026-11-22.md)

`14250` lādiņš gaida Balboa sākotnējā maršrutā un izkaisa piecus gabalus tikai attiecīgajam kvesta mēģinājumam. Serverī pārbaudīti divi vienlaicīgi lādiņi, pieci laupījuma paņēmieni, nodošana un atmešana. [Renewable Resource pārbaude](balboa-basalt-2026-11-22.md)

Pēc `14250` atkārtotajā eksportā ir 5 459 tvēruma ieraksti un 4 865 ieslēgti kvesti. Kanoniskajā Release atrodami visi 160 pārbaudītie skriptu nosaukumi. Kvestiem ar pasaulē novietotu devēju paliek 54 priekšmetu lietošanas kandidāti un 17 statiski neizsekoti aktiera vai progresa ceļi. Eksports un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-balboa/`.

`24974` pavada pieņem abas paredzētās ievainotās murloku sugas. Servera pārbaudē abi gūstekņi seko savam īpašniekam, un pie Sedrick katrs saņem savu atgriešanas atzīmi. Pārbaudīta pavadas un gūstekņa noņemšana. Jaunajā eksportā paliek 53 priekšmetu lietošanas kandidāti ar pasaulē novietotu devēju. Pārējie iepriekšējā eksporta skaitļi atkārtojas. Eksports ir lokāli `.codex/quest-coverage-20261122-leash/`. [Ever So Lonely pārbaude](vile-fin-leash-2026-11-22.md)

`14323` sūklis savāc pilienus tikai ar savu periodisko auru pie paredzētā dzīvā elementāļa. Pārbaudīti 20 pilieni, pilna soma, attālinātā nodošana un priekšmetu izņemšana pēc atmešanas. Atkārtotajā eksportā paliek 52 priekšmetu lietošanas kandidāti ar pasaulē novietotu devēju. Eksports ir lokāli `.codex/quest-coverage-20261122-absorbent/`. [Absorbent pārbaude](absorbent-2026-11-22.md)

`14472` SFG izvēlas tikai paredzēto dzīvo milzi un saglabā vienas devas patēriņu. Faktiskā šāviena nogalināšana ieskaitās aktīvajam šāvējam un derīgajiem grupas biedriem. Pārbaudīta veca šāviena atcelšana, aizkavētā ieskaite un nodošana pie Glix. Jaunajā eksportā ir 50 priekšmetu lietošanas kandidāti ar pasaulē novietotu devēju un 17 statiski neizsekoti aktiera vai progresa ceļi. Eksports ir lokāli `.codex/quest-coverage-20261122-sfg/`. [In The Face! pārbaude](sfg-minefield-2026-11-22.md)

`14423` signālraķete sāk privātu rituālu tikai ar aktīvu kvestu pie faktiskā Azsharas buldozera. Pārbaudīti divi vienlaicīgi rituāli, kopīgā `14209` klikšķa saglabāšana, mājdzīvnieka un grupas ieskaite, atkārtota pieņemšana un faktiskā signālraķetes nodošana pie Custer. [Dozercism pārbaude](dozercism-2026-11-22.md)

`877` klikšķis pie sākotnējās zemūdens plaisas sāk īsto stādīšanas burvestību, kas patērē vienu Dried Seeds devu un ieskaita tikai tās lietotāju. Pārbaudīts nederīgs objekts, atmešana stādīšanas laikā, nerādīta mijiedarbība pēc pabeigšanas un reāla nodošana pie Tonga. [The Stagnant Oasis pārbaude](stagnant-oasis-2026-11-22.md)

`9193` lāpa izmanto vienu devu par katru no desmit dedzināšanām. Ieejas trigeris ieskaita tikai faktisko apmeklējumu, un Valwyn nodošanā izņem iztērēto lāpu. Serverī pārbaudīti 25 novietojumi, abas izpētes un dedzināšanas secības, vecs priekšlaicīgs pabeigums un atmešana. [Investigate the Amani Catacombs pārbaude](amani-catacombs-2026-11-22.md)

`9582` piecu sekunžu atslēgas darbība atver faktiskā Death Ravager būri. Serverī pārbaudīta pārtraukta un atkārtota atvēršana, cīņas atcelšana, mājdzīvnieka un grupas ieskaite un atslēgas izņemšana, nododot kvestu pie Ruada. [Strength of One pārbaude](ravager-cage-2026-11-22.md)

`9544` gūstekni var atbrīvot ar īsto Bristlelimb atslēgu tikai pie viņa faktiskā būra. Serverī pārbaudīti trīs sākotnējie laupījuma avoti, 15 pāri, astoņas kustības ieskaites, nederīga atkārtota atvēršana un rezerves atslēgu noņemšana. [The Prophecy of Akida pārbaude](akida-cages-2026-11-22.md)

`9667` parāda virsaiti pēc vietējā furbolga nāves, pieņem atslēgu tikai īstajā princeses būrī un atjauno princeses piedāvājumu pēc tās atgriešanās. Serverī pārbaudīta sākotnējā pieņemšana, mājdzīvnieka pēdējais sitiens, īstā līķa laupījuma paņemšana, divi vienlaicīgi atvēršanas mēģinājumi, atkārtota parādīšanās un Frasaboo atlīdzības pakete. [Saving Princess Stillpine pārbaude](princess-stillpine-2026-11-22.md)

`13875` pārbauda aktīvu kvestu, lietojot Thorned Bloodcup ziedus un aužot bizi. Serverī pārbaudīti 59 novietojumi, astoņi laupījuma paņēmieni, pilna soma, atcelta aušana un gatavās bizes un rezerves materiālu izņemšana pie Draaka. [Gurtar's Request pārbaude](gurtars-request-2026-11-22.md)

Pēc `13875` atjaunotais eksports aptver 5 459 ierakstus un 4 865 ieslēgtus kvestus. Atbilstošajā Release apstiprināti visi 160 aptvertie skriptu nosaukumi. Statiskajā atlasē kvestiem ar novietotu devēju ir 44 priekšmeta lietošanas kandidāti un 17 neizsekoti aktiera vai progresa ceļi. Eksports ir lokāli `.codex/quest-coverage-20261122-braid/`.

`14006` saliek rokasgrāmatu no lietotāja faktiskā vāka un piecām lapām tikai aktīvā mēģinājumā. Serverī pārbaudīti abi lapu avoti, saglabātais `3924`, gatavās rokasgrāmatas izņemšana pēc atmešanas un Sputtervalve nodošana. Kopā pārbaudīta arī `13875` gatavās bizes izņemšana pēc atmešanas. [Read the Manual pārbaude](samophlange-manual-2026-11-22.md)

`26995` priekšmeti pieejami no paredzētajiem Silverpine zvēriem. Izņemta neatbilstošā Hillsbrad lādes rinda. Serverī pārbaudīts pārējais lādes laupījums, abi zvēru veidi, sešu priekšmetu nodošana un priekšmeta izņemšana pēc atmešanas. [Guts and Gore pārbaude](guts-and-gore-2026-11-22.md)

`25734` saista faktiskā Thelgen seismogrāfa marķieri ar tā laupījuma grupu. Serverī pārbaudīta īstā atvēršanas burvestība, atsevišķi spēlētāji, ieraksta saņemšana un noņemšana pēc nodošanas vai atmešanas. [Down In Thelgen Rock pārbaude](thelgen-seismograph-2026-11-22.md)

`24597` un `24609–24615` saņem aproci pie attiecīgā Legion saņēmēja. Atjaunota kolekcionāra komplekta dialoga izvēle, piecu žetonu atlīdzība un svētku grafiks. Serverī pārbaudīta astoņu pilsētu pieņemšana un nodošana, talismani pēc tiesīgas nogalināšanas, aproces izgatavošana un komplekta saglabāšana. [Love is in the Air dāvanu pārbaude](lovely-charm-gifts-2026-11-22.md)

`29506`, `29510` un `29515–29517` izmanto savu recepšu priekšmetus tikai aktīvā kvestā, kamēr notiek Darkmoon Faire. Serverī pārbaudīti 102 stikla un ieroču novietojumi, īstais laupījums, prasītā profesija, pilna soma, tintes un gatavā izstrādājuma noņemšana un visu piecu kvestu nodošana uz salas. [Darkmoon ražošanas pārbaude](darkmoon-crafting-2026-11-22.md)

`29509` un `29511` pārbaudītas divas īstās pannas, piecu varžu apcepšana un kvestu mantu noņemšana. Serverī apstiprināta arī 15 bojāto tonku sasniedzamība, pieci remonti un uzgriežņatslēgas nodošana. Cepšanai un remontam ir vajadzīgs attiecīgais aktīvais kvests. [Darkmoon pannu un tonku pārbaude](darkmoon-frogs-tonks-2026-11-22.md)

`29520` atjaunota karoga uzstādīšana pie paredzētajiem akmeņiem. Serverī pārbaudīti 18 īstie objektu un marķieru pāri, auduma komplekta izlietošana, karoga izņemšana no somas pēc uzstādīšanas, divu spēlētāju redzamība un sadursmes, un abu karogu neatkarīga noņemšana pie nodošanas vai atmešanas. [Darkmoon karoga pārbaude](darkmoon-banner-2026-11-22.md)

`29350` pārbaudīta Grimnur dienas rotācija, abu tirgotāju piedāvājumi, tramvaja ieejas, 18 žurku slazdi un īstā recepte. Pēc labojuma slazdu var lietot aktīvajā kvestā, rasējums nepieņem nederīgu mēģinājumu un gatavā ēsma pēc nodošanas vai atmešanas pazūd no somas. [The Gnomish Bait-o-Matic pārbaude](ironforge-bait-2026-11-22.md)

`29351` porciju var izmantot tikai uz īsta Dzelzsvelves sarga aktīva kvesta laikā. Serverī pārbaudīti seši paēdušā efekti, viena ieskaite par katru, atteikums barot tikko pabarotu sargu un porciju noņemšana pēc nodevuma vai atmešanas. [A Round for the Guards pārbaude](ironforge-rations-2026-11-22.md)

`24575` pārbaudīts īstais atslēgas avots, 14 ķēžu un gūstekņu pāri, piecas atbrīvošanas ar vienu atslēgu katrā un grupas ieskaite tuvumā. Četras ķēdes bez gūstekņa nevar dot ieskaiti. Nodošanas un atmešanas testi apstiprināja atlikušo atslēgu noņemšanu. [Liberation Day pārbaude](liberation-day-2026-11-22.md)

`25735` pārbaudītas 12 īstās rūdas dzīslas un īpašā cirtņa lietošana bez Mining prasmes. Servera pārbaudē svešam spēlētājam laupījums nebija pieejams, cirtnis noraidīja veco tāda paša nosaukuma objektu, un Dunlor pēc astoņu rūdu nodošanas izņēma gan rūdu, gan cirtni. Pārbaudīta arī to izņemšana pēc atmešanas. [Incendicite Ore pārbaude](incendicite-ore-2026-11-22.md)

`24678` atjaunota tuneļa oriģinālā fāze ar 60 radījumiem un lāpas iedarbība 15 jardu rādiusā. Serverī pārbaudīti visi trīs radījumu veidi, kvesta atmešana un atkārtošana, Krennana nodošana un lāpas noņemšana. Saglabāta Genna fāze no priekšnoteikuma un Krennana nākamais piedāvājums. Lāpas atbaidīšanas vizuālais efekts vēl jāpārbauda klientā. [Knee-Deep pārbaude](knee-deep-2026-11-22.md)

`24536` un `24655` analizators darbojas uz attiecīgās galvaspilsētas sasmaržināta sarga svētku laikā. Serverī pārbaudīti seši sargi katrā pilsētā, sarga auras nomaiņa un atjaunošanās, grupas ieskaite un analizatora noņemšana pēc atmešanas vai nodošanas. [Something Stinks pārbaude](something-stinks-2026-11-22.md)

`24626` laso iedarbojas uz īsto Swiftclaw aktīvajā kvestā. Serverī pārbaudīta faktiskā iekāpšana seglos, atmešana pirms vai pēc iekāpšanas, atkārtota noķeršana, pasažiera pozīcijas atjaunināšana pāri kartes šūnas robežai un nodošana Morajai. Aploks ieskaita tikai faktisko jātnieku, un Moraja pēc nodošanas paņem laso. [Young and Vicious pārbaude](young-and-vicious-2026-11-22.md)

`24472` Korota karoga marķieris un paņemšana pieejami tikai pieņemtā kvestā, kamēr vēl vajadzīgs karogs. Īsta paņemšana iedarbina Korota reakciju. Serverī pārbaudīta ogra ieskaite pēc mājdzīvnieka pēdējā sitiena, karoga izņemšana pēc atmešanas vai nodošanas un Liama nākamais piedāvājums. [Introductions Are in Order pārbaude](koroths-banner-2026-11-22.md)

`24646` atjaunoti desmit sākotnējie rangeru izvietojumi pie Woods’ End Cabin. Rags darbojas aktīvā kvestā pie Tobiasa. Serverī pārbaudīta desmit izsekotāju cīņa, atkāpšanās un apdullināšana, lādes laupījums tikai spēlētājam ar aktīvu mērķi un abu priekšmetu izņemšana pēc atmešanas vai nodošanas Dariusam. [Take Back What's Ours pārbaude](woods-end-coffer-2026-11-22.md)

`14204` kaklasiksnas izsauktais mastifs seko savam spēlētājam, ar īsto mājdzīvnieka komandu atklāj Bloodfang Lurker un uzbrūk pēc lēciena. Serverī pārbaudītas sešas faktiskas nogalināšanas, ieskaitot mastifa pēdējo sitienu, kaklasiksnas izņemšana un tā paša Hunter mājdzīvnieka atjaunošana pēc atmešanas vai nodošanas. [From the Shadows pārbaude](from-the-shadows-2026-11-22.md)

`24958` Hobarts un dzīvais Volcanoth parādās pareizajā kvesta fāzē, Bootzooka šauj uz paredzēto bosu, un pēc nāves alā redzams kūpošais līķis un Sassy. Serverī pārbaudīta mājdzīvnieka pēdējā sitiena ieskaite īpašniekam un tiesīgam grupas biedram, Sassy atlīdzība, Bootzooka izņemšana un alas atgriešanās punkts. [Volcanoth! pārbaude](volcanoth-2026-11-22.md)

`25023` Sassy piedāvā lidojumu no īstā alas lidaparāta. Serverī pārbaudīta personīgā lidaparāta iekāpšana, visas sešas Sassy replikas 25 punktu maršrutā, izkāpšana uz zemes, nodošana Thrall un nākamais kvests. Pēc atmešanas abi personīgie aktieri pazūd, un kvestu var pieņemt vēlreiz. [Old Friends pārbaude](old-friends-flight-2026-11-22.md)

`25165` svešs totēms vairs nedod indes paraugu. Spēlētāja paša totēms novērš īstā Envenom trāpījuma bojājumus un piešķir vienu paraugu no attiecīgā skorpiona. Serverī pārbaudīti seši novietoti skorpioni, mājdzīvnieka īpašnieka un grupas ieskaite, kā arī totēma un priekšmeta izņemšana pēc atmešanas un nodošanas. [Durotar indes paraugu pārbaude](scorpid-venom-2026-11-22.md)

`25937` ir 34 sākotnējās artefaktu vietas. Kvestu var izpildīt ar Excavator’s Pick bez Mining profesijas, un cirtni nevar izmantot vara dzīslai. Serverī pārbaudīta piekļuve tikai aktīva kvesta laikā, trīs artefaktu izņemšana pēc atmešanas, astoņu artefaktu savākšana atkārtotā mēģinājumā un abu priekšmetu izņemšana pēc nodošanas. [Priceless Treasures pārbaude](priceless-treasures-2026-11-22.md)

`26271` personīgais sautējuma katls pieder aktīvam kvesta mēģinājumam. Pieci Sentinel Hill iedzīvotāju veidi saņem spēles divu minūšu Full Belly auru pēc īstas barošanas. Serverī pārbaudīti 20 ieskaitīti iedzīvotāji, noraidīta atkārtota ieskaite citam spēlētājam un pārbaudīta katla un priekšmeta aizvākšana pēc nodošanas Hope Saldean. [Westfall sautējuma pārbaude](westfall-stew-2026-11-22.md)

`26333` Techno-Grenade iznīcina tikai īstu paredzēto tanku aktīvā kvesta mēģinājumā. Serverī pārbaudīti pieci tanki, sākotnējā grupas ieskaite, atmešana granātas lidojuma laikā un granātas izņemšana pēc nodošanas Hinkles. Iepriekšējā metiena trāpījums pēc atkārtotas kvesta pieņemšanas tika noraidīts. [No Tanks! pārbaude](no-tanks-2026-11-22.md)

`26342` Paintinator darbojas tikai uz īstiem aktīviem Iceflow Lake sargrobotiem. Apžilbināts robots pārtrauc uzbrukumu un pazūd pēc sākotnējās aiztures. Serverī pārbaudītas piecas ieskaites krāsotājam, grupas ieskaite arī pēc sveša uzbrukuma robotam, atmešana, atkārtota pieņemšana un priekšmeta izņemšana pēc nodošanas Jarvi. [Paint it Black pārbaude](paint-it-black-2026-11-22.md)

Klasiskā Evershine ķēde `317–320` tika izņemta versijā `4.0.3a`. Datubāzē `317` jau ir atzīmēts kā novecojis, un vienīgais ceļš uz `318–320` ved caur šo sākuma kvestu. Pārbaudē netika atrasta cita aktīva ķēde, kas uz tiem atsauktos. Tāpēc trūkstošais Elder Crag Boar `1127` pie `319` ir izņemtas ķēdes ieraksts. [Stocking Jetsteam](https://warcraft.wiki.gg/wiki/Stocking_Jetsteam) · [Evershine](https://warcraft.wiki.gg/wiki/Evershine) · [A Favor for Evershine](https://warcraft.wiki.gg/wiki/A_Favor_for_Evershine) · [Return to Bellowfiz](https://warcraft.wiki.gg/wiki/Return_to_Bellowfiz)

Pēc `9193` jaunākajā datu eksportā ir 5 459 tvēruma ieraksti un 4 865 ieslēgti kvesti. Atbilstošajā Release atrodami visi 160 pasaules skriptu nosaukumi. Kvestiem ar pasaulē novietotu devēju paliek 47 priekšmeta lietošanas kandidāti un 17 statiski neizsekoti aktiera vai kredīta ceļi. Šie ir pārskatāmi kandidāti, kuros ietilpst arī jau pārbaudīti C++ ceļi. Eksports un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-catacombs/`.

Brazie the Botanist `49687` ir novietots Hillsbrad Foothills un piedāvā sešus aktīvus 20. līmeņa kvestus `28733/28617/28744/28747/28748/28751`. `28744` un `28747` datu pārskatā ir atrastas native ieskaites burvestības `92340` un `92435`, tāpēc tās neparādās četrās pilnībā neizsekotajās atlasēs. Pārbaudītajā datubāzē šīm divām burvestībām arī trūkst izsaukuma sasaistes ar minispēles kontroli. Visu sešu kvestu faktiskā iedarbināšana, viļņi un nodošana paliek atvērti.

`27232` trīs esošajiem lielgabaliem atjaunots sēdeklis, Rocket Blast un peldētāju viļņi. Servera pārbaude apstiprināja 50 faktiskas nogalināšanas un nodošanu pie Admiral Hatchet. [The Waters Run Red pārbaude](waters-run-red-2026-11-22.md)

`13885` Orseus `34392` jāatver hipogrifa posms, un Kathrena Winterwisp `34301` piedāvā un pieņem kvestu. 7.3.5 datiem atrasti `65401 → 65372` izsaukums, pasažiere `34376`, `65384` iekāpšana, `65385` pacelšanās un `65373` laukuma burvestība. 2017. gada 4:24 video visa ceļa kadri ar sešu sekunžu soli parāda divus braucējus, Kathrena vadītu lidojumu, spēlētāja darbību uz trīs sugām un nolaišanos pirms nodošanas. Pasaulē vēl trūkst Orseus funkcijas, transportlīdzekļa datu sasaistes, lidojuma koordinātu un progresa sasaistes ar `34323/34324/34325`. [2017. gada gājiens](https://www.youtube.com/watch?v=Rc2HWdXKOVo)

## Atkārtotā atlase pēc `26364`

`26364` Mekkatorque saruna sāk kauju, un ierīce izmanto spēlētāja paša mērķa marķieri. Serverī pārbaudīta Razlo pilota iekāpšana, sabiedroto pienākšana un cīņa, orbitālā ieroča trāpījums, mājdzīvnieka pēdējais sitiens, grupas ieskaite un Jarvi atlīdzība. Kanoniskais Release un datu migrācija ir uzstādīti. [Down with Crushcog! pārbaude](down-with-crushcog-2026-11-22.md)

Svaigajā pasaules un hotfix eksportā ir 5 459 tvēruma ieraksti. No tiem 4 865 ir ieslēgti un 594 atslēgti. Visi 166 atrastie skriptu nosaukumi ir kanoniskajā Release. Kvestiem ar pasaulē novietotu devēju statiskajā atlasē palika 17 aktieru vai kredīta atzīmes un 31 priekšmeta lietošanas atzīme. Iepriekšējās 66 priekšmetu atzīmes bija no agrāka eksporta. Jaunais eksports, kandidātu saraksti un kontrolsummas glabājas lokāli `.codex/quest-coverage-20261122-crushcog/`.

- No 17 aktieru vai kredīta atzīmēm desmit ceļiem ir iepriekšēja servera pārbaude. Pieciem vēl trūkst izpildāma ceļa. Tie ir `13885` un četri Brazie kvesti `28617/28733/28748/28751`. Pilnai Brazie minispēlei vēl atvērti arī `28744/28747`. `319` pieder izņemtajai Evershine ķēdei, un `46064` jāizskata kopā ar Legion 110. līmeņa pasākumu. `9670` grupas noteikumi joprojām jāapstiprina atsevišķi.
- No 31 priekšmeta lietošanas atzīmes 23 ir iepriekš aprakstītas servera pārbaudes. Astoņi atvērtie gadījumi šajā atlasē ir `14071/26557/26646/26651/26694/27088/29422/29664`.

`26557` izmeklēšanā pārbaudīti abi Sandy Carp `58946` avoti. Valley of Wisdom laupījuma tabulai `5171` jau ir Stag Eye burvestības `80868` nosacījums. Orgrimmar rezerves tabulā `1637` tam pašam priekšmetam ir `QuestRequired=1` un 2,8042% iespēja bez ēsmas auras prasības. Kad apakšzonai nav savas laupījuma tabulas, makšķerēšanas kods izmanto pilsētas tabulu. Pareizajai Stag Eye `58949` rindai ir aktīva kvesta nosacījums un 50 Weakened Mosshoof Stag `35096` novietojumi Azsharā. Rezerves laupījuma labojums un makšķerēšanas pārbaude vēl ir atvērti. [Makšķerēšanas laupījuma atlase](../../src/server/game/Entities/GameObject/GameObject.cpp#L962)

Kopējais audits paliek atvērts. Šoreiz faktisko kvestu izspēli nevarēja pārbaudīt, jo spēles klients nedarbojās.

## Nākamie priekšmeta lietošanas labojumi

`26557` Sandy Carp tagad iegūstams Valley of Wisdom ar aktīvu kvestu un izlietotu Stag Eye. Servera pārbaude aptvēra nepareizās apakšzonas atteikumu, parasto zivju saglabāšanu, atmešanu, jaunu lomu un nodošanu pie Razgar. Migrācija uzstādīta darba MariaDB. [A Staggering Effort pārbaude](stag-eye-fishing-2026-11-22.md)

`26651` Bravo Company Field Kit dod īsto lādiņa darbību nepabeigtā kvestā. Serverī pārbaudīti abi novietotie fokusi, abu būvju ieskaite, atkārtots trāpījums vienā būvē, kvesta atmešana un nodošana pie Keeshan. Migrācija uzstādīta darba MariaDB. [To Win a War, You Gotta Become War pārbaude](redridge-seaforium-2026-11-22.md)

Atkārtotais eksports aptver 5 459 ierakstus, 4 865 ieslēgtus kvestus un visus 166 pārbaudītos skriptu nosaukumus kanoniskajā Release. Pie pasaulē novietota devēja palika 17 aktieru vai kredīta atzīmes un 29 priekšmeta lietošanas atzīmes. Seši priekšmeta lietošanas gadījumi šajā atlasē joprojām ir atvērti, proti, `14071/26646/26694/27088/29422/29664`. Eksports un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-fishing-and-seaforium/`.

`26646` spēles slēdzenei `1930` vajag Blackrock Holding Pen Key `59261`. Atbilstošajam būrim `204441/1379` ieskaite ir piesaistīta atvērta objekta stāvoklim. Pārskatīts arī vispārīgais pogas mijiedarbības ceļš, kas var pārslēgt objekta stāvokli bez slēdzenes pārbaudes. Jāpārbauda šī ceļa faktiskā uzvedība ar būri bez atslēgas, īstā atslēgas izlietošana un gūstekņu secība. [Objekta lietošanas kods](../../src/server/game/Entities/GameObject/GameObject.cpp#L1905)

`26646` servera zonde apstiprināja sākotnējo ieskaiti bez atslēgas. Labojums kanoniskajā Release pieprasa īsto atslēgu un atbrīvojamus gūstekņus. Pārbaudīta komplekta darbība, atvēršana, ieskaite un nodošana. [Prisoners of War pārbaude](blackrock-prisoners-2026-11-22.md)

`26694` servera zonde apstiprināja, ka iepriekš Ilgalar palātu varēja atvērt bez atslēgas. Labojums izmanto īsto atslēgu gan ieejot, gan izejot pēc Doane nāves. Pārbaudīts mājdzīvnieka pēdējais sitiens, īpašnieka ieskaite un Keeshan atlīdzība. [The Grand Magus Doane pārbaude](ilgalar-ward-2026-11-22.md)

`29664` krāsainie ugunskuri tagad prasa īsto iedegto lāpu un faktiskās iedegšanas ieskaiti. Pārbaudīta lāpas maiņa, visu trīs krāsu ieskaite, atmešana un atkārtota pieņemšana pēc lāpas pazaudēšanas, kā arī nodošana pie Li Fei. [The Challenger’s Fires pārbaude](challenger-fires-2026-11-22.md)

`9670` servera zonde ar diviem tuvumā esošiem grupas dalībniekiem apstiprināja personisku ieskaiti sitiena izdarītājam vai mājdzīvnieka īpašniekam pēc faktiskas pētnieka atbrīvošanas. Jaunākā vietējā TrinityCore rekonstrukcija arī sasaista atbrīvoto pētnieku ar izsaucēju. Grupas uzvedība vēsturiskajā Legion klientā vēl jāapstiprina.

`29422` Huo’s Offerings tagad sākas pie redzamā Huo un pabeidz kvestu tikai pēc īstās auras beigām. Pārbaudīta priekšmeta lietošana, dabiskā auras izbeigšanās, aiziešana, atmešana, atkārtots mēģinājums un nodošana. [Huo, the Spirit of Fire pārbaude](huo-offerings-2026-11-22.md)

Atkārtotajā eksportā pēc Redridge un Svētnīcas labojumiem ir 5 459 ieraksti un 4 865 ieslēgti kvesti. Visi 167 atrastie skriptu nosaukumi ir kanoniskajā Release. Atlasē ar pasaulē novietotu devēju palika 17 aktieru vai kredīta atzīmes un 28 priekšmeta lietošanas atzīmes. Šajā priekšmetu atlasē turpināmi `14071` un `27088`. Eksports, kandidātu saraksti un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-shrine/`.

`27088` atjaunoti Forest Ettin oriģinālie sēdekļi. Servera zonde ar īsto būri pārbaudīja vistas satveršanu, pārvietošanu uz mutes sēdekli, sprādzienu, indi, mājdzīvnieka pēdējo sitienu un nodošanu pie Wormcrud. [Forest Ettin pārbaude](forest-ettin-2026-11-22.md)

`14071` atslēgas un trīs pasažieru ieskaite tagad seko faktiskajiem sēdekļiem spēlētāja paša Hot Rod. Pārbaudīti Ace, Gobber un Izzy, atteikums citā transportlīdzeklī, nodošana pie Megs un tās pašas atslēgas izmantošana pēc nodošanas. [Rolling with my Homies pārbaude](kezan-hot-rod-2026-11-22.md)

## 23. novembra audita stāvoklis

Atkārtotais izolētās pasaules un hotfix eksports pēc Kezanas migrācijas aptver 5 459 ierakstus. No tiem 4 865 ir ieslēgti un 594 atslēgti. Kanoniskajā Release atrasti visi 169 pārbaudītie skriptu nosaukumi. Pie novietota kvesta devēja atlasē palika 27 priekšmeta lietošanas un 17 aktiera vai ieskaites atzīmes. Eksports, kandidātu saraksti un kontrolsummas ir lokāli `.codex/quest-coverage-20261123-final/`. Kanoniskais Release un abi eksporta soļi pārbaudīti bez jaunām avārijām un `DBErrors.log` ierakstiem.

Visiem 27 atlasītajiem priekšmetu lietošanas gadījumiem šajā pārskatā ir konkrēta servera pārbaude. Iepriekš atvērtie `14071`, `26557`, `26646`, `26651`, `26694`, `27088`, `29422` un `29664` ir pārbaudīti pēc attiecīgā labojuma. Šie rezultāti attiecas uz aprakstītajiem testiem. Pilnas secības, klienta vadība un attēlojums paliek jāpārbauda spēles klientā.

| Atvērtā aktiera vai ieskaites pārbaude | Atrastā informācija un atlikušais darbs |
| --- | --- |
| `13885` In Defense of Darkshore | Ir Orseus dialoga izvēlne `10509/14541` ar teksta ierakstu `34585`, bet pašam `34392` pašreizējā datubāzē izvēlne un izvēles darbība nav piesaistīta. Vietējā TrinityCore 4.3.4 datu kopija apstiprina izvēlnes saistību ar Orseus. Tai trūkst iekāpšanas, lidojuma ceļapunktu un mērķu progresa sasaistes. Legion ierakstā redzama pilna divu braucēju secība. Apskatītā 5.4.8 realizācija izlaiž lidojumu un dialogā uzreiz piešķir visus 24 punktus. Spēles gaitas atjaunošanai joprojām vajadzīgi lidojuma un mērķu dati. [Legion spēles ieraksts](https://www.youtube.com/watch?v=Rc2HWdXKOVo) · [5.4.8 salīdzinājums](https://gamedebug.github.io/Build-a-MoP-Server-5-Kalimdor/#19-in-defense-of-darkshoreorseus-的选项看不见) |
| Brazie `28733/28617/28744/28747/28748/28751` | Ir kvesta attiecības, dalībnieku veidnes un divas ieskaites burvestības `92340/92435`. Servera datos un vietējās TrinityCore atsaucēs trūkst personīgā laukuma iedarbināšanas, augu izvietojuma un viļņu sasaistes ar konkrētā spēlētāja mēģinājumu un atlīdzību. Blizzard apraksts apstiprina personīgo laukumu un pieejamību abām frakcijām no 20. līmeņa. [Blizzard par personīgajiem izsaukumiem](https://worldofwarcraft.blizzard.com/en-us/news/2428835/in-with-the-new-cataclysm-quest-mechanics) · [Brazie sešu kvestu apraksts](https://worldofwarcraft.blizzard.com/en-gb/news/20115340) |
| `9670` They're Alive! Maybe... | Viens spēlētājs vai mājdzīvnieka īpašnieks saņem ieskaiti pēc faktiskas pētnieka atbrīvošanas. Zonde ar diviem tuvumā esošiem grupas biedriem apstiprina pašreizējo personīgo ieskaiti. Legion oriģinālā grupas uzvedība nav apstiprināta. |

Pārējiem atlasītajiem aktiera ceļiem ir iepriekš aprakstītās servera pārbaudes, izņemtās Evershine ķēdes skaidrojums (`319`) vai atsevišķs Legion 110. līmeņa pasākuma ieraksts (`46064`). Kopējais 1.–20. līmeņa audits paliek atvērts, jo vēl trūkst abu izpildāmo secību un pilnas spēles klienta pārbaudes.

### Atvērto ceļu paplašinātā pārbaude

`9670` pilnajā servera zondē abi grupas dalībnieki pieņēma kvestu pie novietotā Cornelius, un katram glābšana tika ieskaitīta pēc viņa vai viņa mājdzīvnieka darbības. Nodošana ar īsto paketi atklāja divus Venomous Silk Cover eksemplārus. Darba MariaDB izņemts `26017` dublikāts no fiksētās atlīdzības. Pakete `18595` saglabāta, un pēc labojuma Cornelius izsniedz tieši vienu. Iepriekš pārbaudītā personīgā ieskaite serverī saglabājas. Oriģinālā Legion grupas uzvedība joprojām ir vēsturiskās atbilstības jautājums. [Bloodmyst nodošanas pārbaude](webbed-researcher-reward-2026-11-23.md)

`13885` burvestības `65372` galamērķa tips ir `TARGET_DEST_DB`. Pārbaudītajā MariaDB trūkst attiecīgās `spell_target_position` rindas. Pie Orseus atrastie 26972 `TaxiPath` ieraksti savieno Grove lidojumu punktu `339` ar citiem lidojumu punktiem. Pārskatot tuvumā esošos `PathNode` un `Location` ierakstus, neizdevās atrast ar `34375` sasaistāmu maršrutu, kas izlidotu pār dzīvniekiem un atgrieztos Grove. Datubāzē nav arī `34376` sēdekļa sasaistes vai `65373` mērķa nosacījumu. Pārbaudes izdrukas un ievades kontrolsummas ir `.codex/quest-open-20261123/`. [Legion lidojuma ieraksts](https://www.youtube.com/watch?v=Rc2HWdXKOVo)

Brazie vietējā 26972 katalogā `91683` izsauc vadāmo `49176` ar īpašībām `3035`. `49176` transportlīdzekļa lauks ir `0`, un sasaisti ar kvesta pieņemšanu neatrada. Spēles datos ir Spitter, Rocknut un Freezya stādīšanas burvestības `91649/91704/92440`, divi viļņa rezultāta efekti `92340/92435` un Warden Stillwater izsaukums `92767`. 138 sekunžu 2015. gada Basic Botany `28733` ieraksts pārskatīts no pirmā līdz pēdējam posmam ar piecu sekunžu soli. Redzama spēlētāja pāreja uz laukuma kameru un vadības joslu, saules enerģijas ievākšana, Spitter, pretinieku vilnis un atgriešanās pie kvesta nodošanas. Visu sešu kvestu izpildei serverī vēl trūkst laukuma sākuma, vietu un viļņu sasaistes. [Basic Botany ieraksts](https://www.youtube.com/watch?v=eFFdFVJ354Q)

### Klienta palaišana un mērķu novietojumi

23. novembrī instalētais Legion klients sasniedza pieteikšanās ekrānu un uzrādīja būvējumu `7.3.5.26972`. Klients tika aizvērts ar savu `Quit` pogu. Klienta žurnālos ir inicializācija un parasta izslēgšanās. Nebija jaunu klienta vai servera avāriju un `DBErrors.log` izmaiņu. Pārbaudē netika veikta pieteikšanās vai ielāde pasaulē. Pirms un pēc palaišanas nolasītie žurnālu izmēri un kontrolsummas saglabāti lokāli `.codex/level-audit-client-20261123/`.

Questie `v10.0.10` Cataclysm labojums `13885` norāda trīs mērķu veidnes `2165/2071/34318` un saista sarunu ar Orseus `34392`. Vietējā Darkshore izgriezumā šīm veidnēm ir attiecīgi 24, 9 un 19 novietojumi. Ir arī citu dzīvnieku paveidu novietojumi. Questie ierakstā nav hipogrifa kustības ceļa vai sēdekļu. [Questie avota ieraksts](https://github.com/Questie/Questie/blob/56d654dc3c3f68565337a3e56c3d6dfc0c04ef3c/Database/Corrections/cataQuestFixes.lua)

90 jardu apkārtnē pie novietotā Brazie `49687/131600` atrodami deviņi redzamie augu un ierīču paraugi, tostarp divi Fertilize-o-tron `49691`. Personīgā transportlīdzekļa `49176` un personīgā Fertilize-o-tron `49192` veidnēm šajā laukumā nav novietojumu. Šie dati apstiprina fermas atrašanās vietu un dažus dalībnieku paraugus. Vēl vajadzīga kvestu laukuma un viļņu secība.

Izolētās MariaDB `quest_poi_points` nolasījums sasaista Darkshore trīs mērķus ar sākotnējiem kartes laukumiem. Baltastes briežu un lāču laukumiem ir pa astoņām virsotnēm, un naktskaķiem ir trīs atsevišķi laukumi. Brazie sešiem kvestiem ir pa vienam progresa marķierim fermā. Basic Botany marķieris ir pie `(-79, 189)`, un Warden Stillwater marķieris ir pie `(-87, 197)`. Šiem kartes punktiem nav augstuma, kustības secības vai viļņu laiku. Nolasījums saglabāts lokāli `poi-regions.tsv` blakus klienta palaišanas pierakstam.

### Wowhead avotu pārbaude

Parastā Windows Chrome sesijā atvērās pilna `9670` kvesta lapa. Tiešais tīmekļa rīks sākotnēji saņēma robotu pārbaudi vairākām adresēm, bet indeksētās versijas deva kvestu aprakstus un spēlētāju komentārus.

- **Darkshore `13885`.** Publicētais apraksts apstiprina trīs atsevišķus mērķus ar astoņiem dzīvniekiem katrā. Spēlētāju apraksti no agrākām versijām apstiprina automātisku hipogrifa lidojumu, manuāli mērķētu Protect Wildlife, dzīvnieku secību (brieži ziemeļos, naktskaķi rietumu posmā, lāči dienvidu posmā) un iespēju lidot atkārtoti, ja ar vienu reizi nepietiek. Tas saskan ar iepriekš apskatīto Legion ierakstu, vietējiem mērķu laukumiem un burvestību `65373`. Precīzas lidojuma koordinātas un sēdekļu sasaistes publiskajos aprakstos nav dotas. [Kvesta apraksts un lidojuma ziņojumi](https://www.wowhead.com/quest=13885/in-defense-of-darkshore) · [Protect Wildlife](https://www.wowhead.com/spell=65373/protect-wildlife)
- **Brazie.** Spēlētāja ceļvedis, kas sākotnēji rakstīts 2010. gadā, apraksta piecu pamatkvestu secību, 5 × 8 laukumu un Basic Botany sākumu. Vadība tiek nodota spēlētājam pēc pieņemšanas, bet pirmais pretinieks parādās pēc pirmā Spitter iestādīšanas centrālajā joslā. Ceļveža ievads par atkārtotas spēlēšanas nepieejamību attiecas uz agrāku versiju. Atsevišķs spēlētāja ziņojums tieši par 7.3.5 apstiprina, ka pēc saulespuķes balvas bija pieejams dienas kvests `28751` ar diviem viļņiem. Darba MariaDB pamatķēdes saites ir `28733 → 28617 → 28744 → 28747 → 28748`, bet `28751` jau ir dienas kvests ar mērķi `49626`, kura daudzums ir divi. Tā `PrevQuestID` vērtība ir `0`, un kvesta piedāvājumam trūkst saites ar pabeigto `28748`, kas izsniedz `66067`. Tas ir konkrēts piedāvājuma trūkums līdzās joprojām atvērtajiem personīgā laukuma un viļņu realizācijas jautājumiem. [Pamatķēdes ceļvedis](https://www.wowhead.com/guide/peacebloom-vs-ghouls-the-strategy-guide-450) · [7.3.5 spēlētāja ziņojums par dienas kvestu](https://www.wowhead.com/quest=28751/tending-the-garden)
- **Bloodmyst `9670`.** Wowhead apraksts apstiprina piecus atbrīvotus pētniekus un vienu Venomous Silk Cover atlīdzības ierakstu, kāds ir pēc servera labojuma. Kvesta kopīgošanas pazīme apraksta uzaicinājumu pieņemt kvestu. Pārskatītie avoti joprojām nenosaka, vai viena glābšana Legion klientā deva ieskaiti diviem tuvumā esošiem aktīva kvesta dalībniekiem. [Kvesta lapa](https://www.wowhead.com/quest=9670/theyre-alive-maybe)

### Brazie dienas kvesta piedāvājums

`28751` Tending the Garden priekšnoteikums ir saņemta `28748` Lawn of the Dead atlīdzība. Blizzard aprakstā dienas kvests seko pamatķēdei, un 7.3.5 spēlētāja ziņojums apstiprina atvēršanu pēc saulespuķes saņemšanas. Šo secību atjauno pozitīvs `PrevQuestID=28748`, ko esošais servera kvesta piedāvājums un pieņemšana pārbauda pret atlīdzināto kvestu sarakstu. [Blizzard ķēdes apraksts](https://worldofwarcraft.blizzard.com/en-us/news/20115340/basic-botany-peacebloom-vs-ghouls) · [7.3.5 spēlētāja ziņojums](https://www.wowhead.com/quest=28751/tending-the-garden)

`Test-BrazieDailyAdmission.ps1` pārbaudīja esošo kvesta saišu ielādētāju un priekšnoteikuma un dienas atkārtošanas metodes ar AddressSanitizer. Sākotnējā datu kopā dienas kvests izturēja priekšnoteikuma pārbaudi arī pirms pamatķēdes atlīdzības. Salabotajos datos nepabeigts vai pabeigts, vēl nenodots fināls, un četri iepriekšējie atlīdzinātie kvesti ir nepietiekami. Pēc fināla atlīdzības dienas kvests iztur pārbaudi un pēc dienas atjaunošanas kļūst pieņemams atkārtoti. Pagaidu MariaDB tabulā pārbaudīta atkārtota migrācija un administratora jau norādīts priekšnoteikums. Pamatķēde sākas ar `28733` un turpinās pa četrām jau esošajām saitēm.

## Darkshore lidojums, 2026-11-23

`13885` servera labojums ir uzlikts. Pārbaudīta Orseus izvēle, divu pasažieru iekāpšana, viss rekonstruētais lidojums, katrai sugai atsevišķa `8/8` ieskaite pēc trāpījumiem, nolaišanās, nodošana pie Kathrenas un jauns mēģinājums pēc atmešanas. Klientā vēl jāapstiprina kamera, mērķēšana un animācijas. [Lidojuma pārbaudes un būvējums](darkshore-wildlife-flight-2026-11-23.md)

## Basic Botany, 2026-11-24

`28733` servera labojums ir uzlikts. Pārbaudīti divi privāti laukumi vienlaikus, stādīšana, saules enerģija, īstas pretinieku nāves, nodošana un jauns mēģinājums. Klientā vēl jāapstiprina kameras skats, stādīšanas un kaujas vizuālie efekti. `28617/28744/28747/28748/28751` spēles gaita vēl ir atvērta. [Pamācības pārbaudes un būvējums](brazie-basic-botany-2026-11-24.md)

`28617` Flower Power servera pārbaudē darbojas visas trīs vidējās joslas, papildu saules enerģija, Freezya sēklas pēc faktiskas kaujas un atlīdzība. `28744/28747/28748/28751` vēl ir atvērti. [Flower Power pārbaudes](brazie-flower-power-2026-11-24.md)

Kanoniskais worldserver reģistrēja `2026_11_23_11_brazie_daily_prerequisite.sql` ar SHA-1 `FD84B6BB736B74DC86C67FEE071E8D3F157AED6E`, sasniedza gatavību un korekti apstājās. Darba MariaDB pārbaudīta ar `-VerifyLiveData`. Startupā, datu pārbaudēs un zondē nebija jaunu avāriju vai servera un datubāzes kļūdu. Diagnostikas arhīvs ir `build-extractors/bin/Release/fixed/2026-11-23-open-level-quests/`.

Visiem sešiem Brazie kvestiem joprojām jāatjauno personīgā laukuma spēle. Vietējais 26972 katalogs rāda `91683 → 49176` ar transportlīdzekļa izsaukuma kategoriju, kā arī `91688 → 49194` saules enerģijai un `91680 → 49192` Fertilize-o-tron. `91683` parastā iekāpšana prasa `49176` transportlīdzekļa komplektu, bet darba MariaDB un 7.3.5 TDB šai veidnei transportlīdzekļa ID ir nulle. Sēdeklis, personīgās kameras iedarbināšana un viļņu saites paliek atvērtas. `28752` Unstoppable Onslaught datubāzē ir tikai nodošanas saite, kas saskan ar ziņojumu par beta brīvās spēles variantu. [Unstoppable Onslaught vēsture](https://www.wowhead.com/quest=28752/unstoppable-onslaught)

### Darkshore kartes dati un hipogrifa izsaukuma punkts

TDB 7.3.5 pasaules arhīvā burvestības `65372` nulles efektam kartē `1` atrasta pozīcija `4961.64, 130.561, 70.5063`. TDB 4.3.4 arhīvā ir tā pati pozīcija un virziens `0.957487`. Legion 26972 burvestība joprojām izsauc `34375` un lieto `TARGET_DEST_DB`. Migrācija atjauno šo rindu, ja datubāzē tai vēl nav vērtības.

Kartes ielādētājs pievieno punktus pēc kvesta un `Idx1`. Pieci vecie `23877` mērķa ieraksti sūtīja tos pašus laukumus vēlreiz, un četros vecajos ierakstos mērķa indekss bija `0`. Migrācija izņem tikai veco ierakstu ar precīzi atbilstošu `26124` definīciju. Pēc labojuma kartes atbildē ir septiņi ieraksti, un visi 31 sākotnējie punkti, Kathrena nodošanas atzīme un `293859` izsekošanas ieraksts ir saglabāti.

`Test-DarkshoreDefenseData.ps1` ar AddressSanitizer izpildīja īsto POI ielādi un atbildes izveidi ar vecajiem un labotajiem datiem. Pagaidu MariaDB tabulās pārbaudīta divkārša migrācija, pazudis jaunais līdzinieks, pielāgots vecais POI, pielāgots izsaukuma mērķis un blakus esošā `13925` dati. Saglabātā Vrekt POI pārbaude arī izturēja. Darba datubāzes nolasījums sakrita ar pārbaudītajiem labojumiem.

Kanoniskais worldserver reģistrēja `2026_11_23_12_darkshore_defense_map_and_summon.sql` ar SHA-1 `5C4B179FF0EB8D24EF4F41076256B030A2AEA5E5`, sasniedza gatavību un korekti apstājās. Abos kanoniskajos startos, pārējos testos un dzēšanas beigās netika konstatētas jaunas avārijas vai DB kļūdas. Esošais 587 baitu `DBErrors.log` saglabāts. Ar pārbaudītām kontrolsummām arhivēti 49 diagnostikas un datu pārbaudes faili. Pēc pārbaudēm izņemti 14 testa būvju faili, kopā 67 766 716 baiti. Arhīvs un dzēšanas saraksts ir `build-extractors/bin/Release/fixed/2026-11-23-open-level-quests/`.

Darkshore lidojuma izpilde paliek atvērta. Pārbaudītajā Legion datubāzē un 7.3.5 TDB `34375` transportlīdzekļa ID ir nulle. Orseus izvēle `10509/0`, burvestība `65401` un atjaunotais izsaukuma punkts ir zināmi. Vēl jāsaista faktiskie divi sēdekļi, ceļš pāri mērķu laukumiem un `65373` ieskaite. Legion video un esošie kartes laukumi rāda ceļa posmus, bet pārbaudītajos pasaules datu arhīvos un klienta maršrutu tabulās lidojuma ceļapunktus neatrada. Kartes attēlojums un pilnais lidojums paliek jāpārbauda klientā.

## Atvērto labojumu noslēgums, 2026-11-24

- **In Defense of Darkshore `13885`.** Serverī darbojas Orseus izvēle, iekāpšana, lidojums, mērķēšana, katrs no trim 8/8 mērķiem un nodošana pie Kathrena. Atkārtota pārbaude pēc pēdējā Brazie būvējuma izturēja. [Lidojuma audits](darkshore-wildlife-flight-2026-11-23.md)
- **Brazie ķēde `28733/28617/28744/28747/28748/28751`.** Visi seši kvesti izieti servera pārbaudēs no pieņemšanas līdz nodošanai. Finale izsniedz Singing Sunflower sēklas, un dienas kvestam pārbaudīta pieejamība pēc atiestatīšanas. [Basic Botany](brazie-basic-botany-2026-11-24.md) · [Flower Power](brazie-flower-power-2026-11-24.md) · [Ghouls Hate My Grains](brazie-ghouls-hate-my-grains-2026-11-24.md) · [Someone Setup the Pumpkin Bomb](brazie-pumpkin-bomb-2026-11-24.md) · [Lawn of the Dead](brazie-lawn-of-the-dead-2026-11-24.md) · [Tending the Garden](brazie-tending-the-garden-2026-11-24.md)
- **They're Alive! Maybe… `9670`.** Atkārtoti pārbaudīta katra spēlētāja un mājdzīvnieka īpašnieka personīgā ieskaite, pieci īsti glābšanas gadījumi, atmešana, atkārtojums un viena Venomous Silk Cover atlīdzība. Vēsturiskā Legion ieskaite vairākiem grupas dalībniekiem par vienu glābšanu paliek neapstiprināta. [Bloodmyst audits](webbed-researcher-reward-2026-11-23.md)

Noslēdzošais lasāmais eksports pret uzstādīto būvējumu atkal atrada 5 459 atlases ierakstus, 4 865 ieslēgtus kvestus un visus 170 atrastos skriptu nosaukumus Release failā. Salīdzinājumā ar 23. novembri nav jaunu statisko atzīmju aktiera vai ieskaites ceļiem, priekšmetu lietošanai vai priekšmetu ieguvei pie novietotiem devējiem. Statiskā atlase joprojām uzrāda 17 aktiera vai ieskaites, 27 lietošanas un divas ieguves atzīmes. Pēc iepriekš aprakstītajām servera pārbaudēm starp šīm atzīmēm nav jauna apstiprināta izpildes defekta. Pilns pārskats ar ievades kontrolsummām ir lokāli `.codex/quest-coverage-20261124-final/survey/`.

Kanoniskais `worldserver.exe` ir uzstādīts, sasniedza gatavību un korekti apstājās. Pārbaudēs nebija jaunu avāriju vai servera un datubāzes kļūdu. Darkshore maršruts un Brazie viļņu laiki ir atjaunoti no pieejamiem datiem un spēles gaitas avotiem. To izpilde ar 7.3.5 klientu, attēlojums un vadība paliek jāpārbauda spēlē.

### Izolētās klienta pārbaudes gatavība, 2026-11-24

`worldserver` un `bnetserver` sasniedza gatavību ar izolētu MariaDB, testa kontu un tēlu shēmām un atsevišķiem lokālajiem portiem. Pirmā gatavības zonde atvēra un aizvēra TCP savienojumu pirms protokola sākuma. Tas izraisīja paredzamas nepabeigta pasaules un TLS savienojuma kļūdas. Atkārtotajā palaišanā gatavību noteica pēc serveru žurnāliem, un abu serveru stderr bija tukšs. Abi beidzās ar kodu 0. Izolētā MariaDB arī korekti apstājās. Atkārtotajā pārbaudē nebija jaunu avāriju vai DBErrors ierakstu.

Instalētais Legion `7.3.5.26972` klients ar atsevišķu konfigurāciju sasniedza pieteikšanās ekrānu un aizvērās ar `Quit`. Parastās `Config.wtf` kontrolsumma sakrita pirms un pēc pārbaudes. Klienta diagnostikā nav jaunu avāriju. Pieteikšanās vēl jāveic lietotājam, tāpēc šī pārbaude neapstiprina ielādi pasaulē, Darkshore karti vai lidojuma vadību un Brazie kameru vai darbību joslu.

18 diagnostikas failu pārbaudītās kopijas atrodas `build-extractors/bin/Release/fixed/2026-11-24-level-client-readiness/`. Palaisti esošie Release izpildfaili. Automātiskā komandu pārbaude bloķēja izolētās pieteikšanās konfigurācijas un sertifikāta kopiju dzēšanu no ignorētās testa mapes.

## Paplašinātais audits, 2026-11-25

Atkārtotais darba datubāzes eksports un izvietotā Release pārbaude atkal aptver 5 459 ierakstus un 4 865 ieslēgtus kvestus. Atrasti visi 170 sasaistītie skriptu nosaukumi. Audits tagad nolasa Adventure Guide kvesta darbību, klases kartes kvesta punktus un to PlayerCondition līmeņus. Pārbaudīta arī tēlu izveides tabula un Paragon reputācijas ieraksti. Neviens no šiem ceļiem nepievienoja jaunu 1.–20. līmeņa piedāvājumu no iepriekš ārpus atlases esošās kopas.

Starp 3 304 nulles minimuma ierakstiem bez atrasta piedāvājuma ir 3 127 ar slēptās ieskaites karogu, 74 izslēgti, 49 bez nosaukuma un 18 ar iekšēja vai testa ieraksta pazīmēm. Atlikušie 36 ar parastiem nosaukumiem ir turpmākas identifikācijas kandidāti. No tiem `13804` ir prasīts atlīdzināts `13802`, kas pieejams no 22. līmeņa. `25229` un sekojošais `25199` attiecas uz Gnomeregan pirms-Cataclysm pasākumu. `14097` un `37474` ir nodošanas saites gan darba datos, gan TDB 7.3.5 arhīvā, un abos avotos nav piedāvātāja. Spēles gaitas pierādījums šo divu ierakstu aktivizēšanai joprojām trūkst. [Gnomeregan pasākuma vēsture](https://github.com/Questie/Questie/issues/3995)

Pārskatītas visas 11 tiešās skaitliskās C++ atsauces. `12845` ir atslēgta un neizmantota teleportācijas konstante. `29592` un `29709` attiecīgajās C++ rindās apzīmē NPC un nogalināšanas kredītu. `33348` ir Timeless Isle ievada ieskaite, `38995` Argus svilpes nosacījums, `39695` Legion reidu ieskaite, `40076` Demon Hunter Mardum ainas ieskaite, `41246` Highmountain ainas atzīme, un `45536–45538` apkalpo nedēļas PvP saskarni. Neviena no šīm atsaucēm neatver parastu 1.–20. līmeņa kvestu.

Tiešajos priekšnoteikumos un kvesta piedāvājuma nosacījumos atrastas četras atsauces uz ārpus atlases palikušiem ierakstiem. `259` ir atslēgts, `2020` atzīmēts kā nelietots, `25229` ir iepriekš minētā pasākuma ieraksts. `29404` ir vecais Pandaren ievads, uz kuru atsaucas arī devēja nepiedāvātais `29405` variants. Atsevišķi pārbaudīta šīs ķēdes vietējā 26972 secība un salabots septiņu faktiski piedāvāto klašu mācību sākums. [Wandering Isle labojums](wandering-isle-first-lesson-2026-11-25.md)

Zonas automātiskā pieņemšana tagad pārbauda vietu žurnālā un inventārā pirms pievienošanas, un kvesta logu sūta pēc veiksmīga ieraksta. Sākotnējais kods reproducēja lieku logu ar pilnu žurnālu. Labotais kods izturēja arī atgriešanos zonā un neveiksmīgu pievienošanu. [Zonas kvesta pieņemšanas audits](area-quest-admission-2026-11-25.md)

Ar aktuālajām gala saistes bibliotēkām atkārtoti izturēja Darkshore `13885` lidojums ar nodošanu un atkārtojumu, visu sešu Brazie kvestu secība un Bloodmyst `9670` personalizētā glābšana grupā un viena atlīdzība. Izolētā MariaDB korekti izslēdzās. Pārbaudēs nebija jaunu avāriju vai servera kļūdu. Kanoniskajā Release mapē ar pārbaudītām kontrolsummām arhivēti 83 diagnostikas faili zem `fixed/2026-11-25-level-quest-coverage/`. Izņemti 24 pagaidu faili, 1 357 462 907 baiti. Saglabāta iepriekšējā Release EXE un MAP atkopšanas kopija. Atkārtotā eksporta kontrolsummas un kandidātu saraksti ir lokāli `.codex/level-coverage-20261125/final-survey/`.

Atlikušās pārbaudes 7.3.5 klientā ir Darkshore un Brazie vadība un vizuālie efekti, jaunais Pandaren sākums un zonas kvesta logs. Bloodmyst grupas pārbaude apstiprina šī servera personīgo ieskaiti. Vēsturiskajai Legion ieskaitei vairākiem spēlētājiem par vienu glābšanu joprojām trūkst atbilstoša oriģinālā ieraksta. Statiskais datu pārskats aptver visu atlasi, un kvestu individuālā izspēle ir dokumentēta konkrētajos auditos.

Papildu pārbaudē `14097` saņēmējs ir fāzē novietotā Gwen Armstead `34936`, bet `37474` saņēmēja ir Audrey Burnhep `63596`. Abiem ir Legion `QuestV2` indekss, taču darba datos trūkst mērķu, starteru, DB starta skriptu un `QuestLineXQuest` vai `QuestV2CliTask` ierakstu. Skaitlis `37474` izvēlnē `10937` ir Taurajo dialoga teksta ID. Spēlētāju kvesta statusa tabulās abu ID kopskaits ir nulle. Pašreizējā laidienā neizdevās atrast parasto sākuma ceļu. Abiem paliek sākotnējie dati un saņēmēji.

Visām septiņām Pandaren klasēm serverī pārbaudīts Shang Xi piedāvājums un parastā pieņemšana, kā arī vecā ievada nodošana un turpinājums mūkam. [Wandering Isle servera tests](wandering-isle-first-lesson-2026-11-25.md#servera-kvesta-apstrādātāja-pārbaude)

## Singing Pools un Pool of Reflection, 2026-11-26

Servera pārbaudēs izgāja Shang Xi ceļa sākums, Aysa nodošana, zvans pa abiem klienta mijiedarbības ceļiem un visu sešu baseinu pārejas starp sausumu un ūdeni. Novietotais līdzsvara stabs ļauj uzkāpt sausam tēlam un noraida vardes formu. [Training Bell](wandering-isle-training-bell-2026-11-26.md) · [Pool Curses](wandering-isle-pool-curses-2026-11-26.md)

Pārbaudīti 30 niedru novietojumi ar astoņu īstu priekšmetu nodošanu, seši Tushui Monk sakāves kredīti un viņu pagaidu stabu noņemšana, kā arī Old Man Liang piedāvājums, gliemene un pērles atlīdzība. `29666` Water Pincers ieskaite darbojas ar spēlētāja un peta pēdējiem sitieniem. [Stronger Than Reeds](wandering-isle-tearwood-reeds-2026-11-26.md) · [Balanced Rock](wandering-isle-balanced-rock-2026-11-26.md) · [The Sun Pearl](wandering-isle-sun-pearl-2026-11-26.md) · [Liang priekšnoteikumi](wandering-isle-shu-prerequisites-2026-11-26.md)

Shu uzdevums pārbaudīts no abu Liang uzdevumu nodošanas cauri četriem pabeigtiem lēcieniem, īstās pērles izmantošanai baseinā un attālajai nodošanai. Pēc tam `A New Friend` izgāja pieņemšanu no nākamā piedāvājuma, piecas faktiskas strūklas un atlīdzību pie Aysa. [Reflection pārlēkšana](wandering-isle-reflection-crossing-2026-11-26.md) · [Coax Shu](wandering-isle-coax-shu-2026-11-26.md) · [A New Friend](wandering-isle-water-spout-2026-11-26.md)

Lasāmais darba datu eksports pēc šī laidiena aptver 5 459 ierakstus un 4 865 ieslēgtus kvestus. Visi 173 atrastie skriptu nosaukumi ir Release failā `AD7D072963EA748DE22B0DD7EA16B676E052EA87995D0E8615ACAD2E101E2863`. Atzīmēti tie paši 17 aktiera vai ieskaites, 27 priekšmeta lietošanas un divi ieguves kandidāti ar novietotiem devējiem. Eksports izmantoja lokālu, tikai lasāmu savienojumu. MariaDB klients brīdināja par sertifikāta pārbaudi savā lokālajā savienojumā. Servera diagnostikā nav jaunu avāriju vai DBErrors. Kopsavilkums ar ievades kontrolsummām ir lokāli `.codex/wandering-shu-20261126/level-coverage-post-deploy-survey/`. [Diagnostika un dzēšanas saraksts](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/)

Servera testi pārbauda uzskaitītos ceļus. Pilna 4 865 kvestu izspēle, kustība un vizuālie efekti klientā, kā arī iepriekš dokumentētie neatrisinātie oriģinālo piedāvājumu vai vēsturiskās uzvedības gadījumi paliek atvērti.

## Dai-Lo ferma un kvesta logi, 2026-11-26

Servera pārbaudēs izgāja visu trīs ratu iekāpšana un agras izkāpšanas tīrīšana, desmit Virmen ieskaites ar spēlētāja un peta pēdējiem sitieniem, 52 savākšanas vietas abiem krājumu kvestiem, āmura paņemšana un atkārtojums, kā arī gonga ieskaite pa abiem klienta mijiedarbības ceļiem. Ji piedāvāja nākamo kvestu pēc gonga nodošanas. [Rati](wandering-isle-dai-lo-cart-2026-11-26.md) · [Krājumi](wandering-isle-dai-lo-supplies-2026-11-26.md) · [Missing Mallet](wandering-isle-missing-mallet-2026-11-26.md) · [Raucous Rousing](wandering-isle-break-gong-2026-11-26.md)

Atkārtotā darba datubāzes atlase joprojām aptver 5 459 ierakstus, no tiem 4 865 ieslēgtus un 594 atslēgtus. Visi 173 atrastie skriptu nosaukumi ir darba Release failā `6467DC52D5685635BF7EEA8A904236D06EEFAF5CBD83CC5E1AC31B4384859B36`. [Kandidātu saraksts un diagnostika](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/quest-packets-and-break-gong-survey/)

Kvesta detaļu pārbaude apstiprināja vienu serializāciju un vienu sūtījumu. Adventure Guide pārbaudēs izmantoti 15 kartes veidņu ieraksti. Pārbaudīta pieejamība, pašpiedāvājums, viena automātiska pieņemšana, dublēta pieprasījuma noraidīšana un garrisona pakalpojumu izvēle. Faktiskajā Release failā pārbaudīta žurnāla darbības lauka izmantošana, sākšanas izsaukums un viena detaļu serializācija. Sākotnējai pārbaudei trūka `IsAutoAccept` imitācijas un tā mēģināja palaist Windows Store Python aizstājēju. Abas pārbaudes nepilnības novērstas. Pirmajā atsevišķajā Release verifikatora izsaukumā trūka ceļa argumenta. Ar darba Release ceļu pārbaude izgāja. C++ testa būvē palika C4244 brīdinājums par esošās kartes kvesta lauka 16 bitu tipu. Servera pārbaudēs nav jaunu avāriju, DBErrors vai servera brīdinājumu. Faktiskās kartes atzīmes, logu aizvēršana un Argus, Brawl un Suramar pieņemšana 7.3.5 klientā paliek atvērtas.

Ar kontrolsummām saglabāti 55 diagnostikas un atlases faili. Izņemti 25 abu testu pagaidu faili, 1 304 916 baiti. [Arhīvs un dzēšanas saraksts](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/quest-packets-and-break-gong-survey/)

## Wugou un atgriešanās templī, 2026-11-26

Reģistrēta īstā Shu sarunas pakete, trīs kustības punkti, dabiskās šļakatas trāpījums un Ji atlīdzība. Pārbaudīts atkārtojums pēc kvesta atmešanas, otra spēlētāja vienlaicīga izvēle un divi paša spēlētāja pavadoņi. Visi trīs salas rati serverī sasniedza savus sākotnējos galapunktus. Pārbaudīta tempļa atlīdzība, pavadoņu noņemšana un `29776` pieņemšana. [Wugou](wandering-isle-wake-wugou-2026-11-26.md) · [Ratu ierašanās](wandering-isle-cart-arrival-2026-11-26.md)

Pēc tempļa labojuma darba datubāzes pārskats aptver 5 459 ierakstus, no tiem 4 865 ieslēgtus. Visi 173 atrastie skriptu nosaukumi ir Release failā `B68B8C5F3B25F86FB0616BCF1F4F0B19D499A344D2AE26AD3CEE1753C8B83763`. Eksporta un Release pārbaudē nav konstatētas jaunas servera kļūdas vai avārijas. Ar kontrolsummām arhivēti 13 pārskata un diagnostikas faili. [Darba datubāzes pārskats](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/after-temple-arrival/)

## Morning Breeze, Fe-Feng un Dafeng, 2026-11-26

Pārbaudīts 16 punktu lidojums, piecas īstas ruļļu sadedzināšanas, 72 krājumu vietas abiem vākšanas kvestiem, astoņas otas no Hozen un Jade Tiger Pillar sākuma priekšmets un atlīdzība. [Ruļļu un lidojuma audits](wandering-isle-defaced-scrolls-2026-11-26.md) · [Krājumi](wandering-isle-village-supplies-2026-11-26.md) · [Jade Tiger Pillar](wandering-isle-fe-feng-ji-2026-11-26.md)

Atjaunots personīgais Ji Fe-Feng ciemā, pārbaudīta viņa noņemšana, atkārtots piedāvājums un abu cīņas kvestu nodošana. Visi 28 Ruffian vairs nepārmanto pastāvīgo tēloto nāvi. Pārbaudīti desmit īsti Ruffian un desmit īsti Firethief, pēdējos sitienus pamīšus izdarīja spēlētājs un pets. Pārbaudīta nākamā kvesta nodošana Aysa. [Ji Fe-Feng ciemā](wandering-isle-fe-feng-ji-2026-11-26.md) · [Ruffian cīņa](wandering-isle-fe-feng-ruffians-2026-11-26.md)

`29785` oriģinālajā `7041` ieejā pārbaudīts Aysa izsaukums. Parastā darbība pie Dafeng `55592/245027` ieskaitīja diviem spēlētājiem pie viņa, neieskaitīja spēlētājam bez kvesta un no attālās ieejas. Abi spēlētāji nodeva uzdevumu pie Dafeng. Sākotnējais pieņēmums par neaktīvu datu rindu netika apstiprināts, tāpēc šim nodošanas ceļam servera kods un dati netika mainīti. Saglabāts arī sākotnējās zondes rezultāts. Aysa gājiens starp vēja šķēršļiem un klienta izspēle šajā pārbaudē nav veikta. Ar kontrolsummām arhivēts 31 fails un izņemti 20 pagaidu faili, 730 615 108 baiti. [Dafeng zondes](../../build-extractors/bin/Release/fixed/2026-11-26-wandering-isle-dafeng-arrival/)

Vēja tempļa servera pārbaudē abi privātie Aysa pavadoņi apstājās pie abiem oriģinālajiem vēja šķēršļiem un turpināja tikai pēc atbilstošā vēja signāla. Abi sasniedza 11. ceļa punktu, un pēc atmešanas varēja saņemt jaunu pavadoni. Pārbaudīta arī Dafeng ieskaite pie paša aktiera un atlīdzība. Gājiena un vēja animācijas klientā vēl jāpārbauda. [Wind Temple pavadoņu audits](wandering-isle-wind-temple-escorts-2026-11-26.md)

Pēc vēja tempļa laidiena darba datubāzes pārskatā ir 5 459 ieraksti, no tiem 4 865 ieslēgti un 594 atslēgti. Visi 173 atrastie skriptu nosaukumi ir Release failā `CF7C79A6F7A444A14AC8509B4504F38AD48857D98A330C2FB3D233D370FEB4B8`. Pārskatā nav jaunu servera kļūdu vai avāriju. Ar kontrolsummām arhivēti 13 faili. [Datubāzes atlase](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/after-wind-temple/)

`Battle for the Skies` servera pārbaudē pieci oriģinālie lādiņi nogāza Zhao-Ren. Peta pēdējais sitiens deva ieskaiti īpašniekam un otram tiesīgam spēlētājam tuvumā. Elder Yun piešķīra vienu apmetni un piedāvāja `Worthy of Passing`. Pārbaudīts arī attāls klikšķis, parasts uzbrukums lidojuma laikā un atkārtots atlīdzības pieprasījums. [Zhao-Ren cīņa un atlīdzība](wandering-isle-zhao-ren-2026-11-26.md)

Pēc Zhao-Ren laidiena atkārtotā datubāzes atlasē ir 5 459 ieraksti, no tiem 4 865 ieslēgti. Visi 173 sasaistītie skriptu nosaukumi ir pārbaudīti attiecīgajā Release failā. Vēl ir 17 statiski neizsekoti aktiera vai ieskaites ceļi un 27 priekšmetu lietošanas kandidāti ar atrastu devēja novietojumu. Tie ietver ceļus, kuru izsaukumus statiskā atlase nesasaista. Jaunas servera kļūdas vai avārijas netika atrastas. [Datu pārskats](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/after-zhao-ren/)

Templī pārbaudīts faktiskais Shang Xi maršruts, sarga uzvara, nodošana, abi sagatavošanās kvesti un amuletu izņemšana pēc atmešanas. `Passing Wisdom` pārbaudīta pieņemšana pēc abām sagatavošanās atlīdzībām, atkārtojums ar jaunu privāto aktieri, spieķa iestādīšana un nodošana Aysa. Servera pārbaude sasniedza `The Suffering of Shen-zin Su` pieņemšanu. [Shang Xi un tempļa sagatavošanās kvesti](wandering-isle-passing-wisdom-2026-11-26.md)

`The Suffering of Shen-zin Su` pārbaudīta reāla iekāpšana ar sākotnējo klikšķi, divi paralēli privāti baloni, atmešana un jauns lidojums. Abi pavadoņi iekāpa, balons sasniedza visus 21 punktus, un Elder Shaopai pieņēma abus pabeigtos mērķus. [Shen-zin Su lidojums](wandering-isle-shen-zin-su-flight-2026-11-26.md)

`Bidden to Greatness` pārbaudīti divi privāti vārtu komplekti, īstais kontakts katram īpašniekam, abu vārtu atvēršana eskorta laikā, atmešana un nodošana pie Wei. Ji un Jojo īpašās kustības vairs neizraisa nepareiza ceļa punkta kļūdas. [Mandori un Pei-Wu vārti](wandering-isle-forest-gates-2026-11-26.md)

Pēc meža vārtu laidiena atkārtotā datu atlasē ir 5 459 ieraksti, 4 865 ieslēgti un 594 atslēgti. Visi 173 sasaistītie nosaukumi ir Release failā `79E2CB0AEFD692C88C9C2F548B8FEE2E8AECDF05EF79AD82462D4B67605A2C8A`. Atkārtotas atlases diagnostikā jaunu servera kļūdu vai avāriju nav. Ar kontrolsummām saglabāti deviņi kopsavilkuma un diagnostikas faili. Visiem 4 865 ieslēgtajiem kvestiem pilna izspēle klientā vēl nav veikta. [Datu atlase pēc meža vārtu labojuma](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/after-forest-gates/)

Atjaunota kvesta bambusa piekļuve un bambusa un sprāgstvielu tīrīšana pēc atmešanas. Pārbaudīti desmit paņemti bambusi, sešas sprāgstvielas, Makael pāreja un deviņas Pei-Wu tīģeru uzvaras ar spēlētāja un peta ieskaiti. [Dienvidu meža krājumi](wandering-isle-south-supplies-2026-11-26.md)

Pie avārijas vietas pārbaudītas astoņas `Evil from the Seas` uzvaras, `Urgent News` nodošana no Aysa pie Ji, un `Medical Supplies` ieguve un nodošana. Medicīnas saiņu izņemšana pēc atmešanas ir darba datubāzē. [Avārijas vietas medicīnas krājumi](wandering-isle-crash-site-medical-2026-11-26.md)

`None Left Behind` pārbaudīta īsta jūrnieka paņemšana, atmešana nešanas un nodošanas brīdī, trīs atsevišķas nodošanas ieskaites un atlīdzība pie Ji. Darba servera pārbaude apstiprināja jauno laidienu bez jaunām avārijām vai DBErrors. [Ievainoto jūrnieku glābšana](wandering-isle-sailor-rescue-2026-11-26.md)

`From Bad to Worse` un `An Ancient Evil` izgāja faktiskas uzvaras ar peta ieskaiti un sākotnējos nodošanas ceļus. `Risking It All` pārbaudīti divi privāti eskorti, atmešana, aktieru saruna, filmas un pārvietošanas apstiprinājumi un nodošana pie Ji. Pēdējā Aysa noiešanas punkta navigācija klientā vēl jāpārbauda. [Vordraka un avārijas vietas eskorts](wandering-isle-thorn-escort-2026-11-26.md)

`The Healing of Shen-zin Su` atjaunota progresa josla un novērsta norise pēc pēdējā dalībnieka aiziešanas. Servera pārbaudē ieskaitīti dzīvie dziednieki, 35 īstas uzvaras pār uzbrucējiem, 700 dziedināšanas punkti un atlīdzība pie Ji. [Shen-zin Su dziedināšana](wandering-isle-healing-defense-2026-11-26.md)

`New Allies` pārbaudīta 24 punktu ratu kustība, izvēles ieskaite un abas nodošanas iespējas pie Master Shang Xi. `A New Fate` serverī pārbaudīti abi frakciju ceļi cauri dabiskajām dialoga, filmas un pārcelšanas paketēm līdz Aysa atlīdzībai Stormwind un Ji atlīdzībai Orgrimmar. Filmas attēlojums WoW klientā paliek atvērts. [Wandering Isle atvadīšanās](wandering-isle-departure-2026-11-26.md)

Atkārtotā 1–20 līmeņa darba datu atlasē ir 5 459 ieraksti, tostarp 4 865 ieslēgti un 594 atslēgti. Visi 173 sasaistītie skriptu nosaukumi atrasti darba Release failā `45D61C62EBA9266F397C4F0AB76DE6C369693547D5A1F371DF5F42B2FA83371C`. Saglabātas 64 lasāmo ievades failu kontrolsummas un deviņi kopsavilkuma un diagnostikas faili. Visiem ieslēgtajiem kvestiem atsevišķa izspēle klientā nav veikta. [Darba datu atlase pēc Wandering Isle](../../build-extractors/bin/Release/fixed/2026-11-26-level-quest-coverage/after-wandering-isle-departure/)

Visiem 27 priekšmeta lietošanas kandidātiem ar novietotu piedāvājumu atrastas iepriekšējās servera pārbaudes. No 17 aktiera vai ieskaites kandidātiem ar novietotu piedāvājumu 15 ir izsekoti konkrētajos auditos. Pārējie divi ir agrākās Evershine ķēdes `319`, kuras sākums `317` ir apzināti atslēgts, un `46064`, kas piesaistīts tikai March of the Tadpoles pasākumam `305`. Pārbaudītajiem agrāko ķēžu kvestiem netika atrasti ieraksti aktīvajos spēlētāju kvesta žurnālos. Abas neizsekotās parastās nodošanas rindas `39657` un `43055` ir piesaistītas Hallow's End objektiem garnizonā un Legion kartē. To sezonas izspēle ir atsevišķa pārbaude.

Westfall `26320` izņemta neaktīvā pieņemšanas ieskaite. Serverī atkārtoti pārbaudīta īstā Thoralius pieņemšana, Incense Burner un nepareizās atrašanās vietas atteikums. Deadmines vīzijas pilnā secība paliek atvērta, jo pašreizējais aktiera izsaukums pabeidz kvestu uzreiz. [A Vision of the Past](westfall-vision-admission-2026-11-26.md)

Westfall `26322` Vanessa fināls tagad izmanto parasto kvesta pabeigšanu. Serverī pārbaudīta sākotnējā aina, divu tiesīgu aculiecinieku ieskaite, atlīdzība pie Anduin un `Return to Sentinel Hill` pieņemšana. Darba servera palaišanā jaunu avāriju vai DBErrors nebija. Ainas attēlojums klientā paliek atvērts. [Rise of the Brotherhood](westfall-brotherhood-2026-11-26.md)

Deeprun Tram žurkai noņemtas 26 ar veco Westfall ainu saistītas rindas, kas aizstāja tās parasto skriptu. Serverī pārbaudīti pieci īsti ķērieni, atteikums noķert to pašu žurku divreiz, nodošana pie Monty un jauna pieņemšana pēc atmešanas. Darba serveris ielādēja labojumu bez jaunām avārijām vai DBErrors. [Deeprun Rat Roundup](deeprun-rat-roundup-2026-11-26.md)

`A Vision of the Past` serverī tagad sākas ar faktisku iekāpšanu un pabeidzas pēc lidojuma, Edwin nāves un Vanessas fināla. Pārbaudīti divi privāti braucieni, pārtraukšana un nodošana pie Marshal ar pieejamu nākamo kvestu. Kamera un attēlojums WoW klientā paliek jāpārbauda. [Deadmines vīzijas audits](westfall-vision-encounter-2026-11-26.md)
