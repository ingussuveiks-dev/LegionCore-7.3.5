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

Klasiskā Evershine ķēde `317–320` tika izņemta versijā `4.0.3a`. Datubāzē `317` jau ir atzīmēts kā novecojis, un vienīgais ceļš uz `318–320` ved caur šo sākuma kvestu. Pārbaudē netika atrasta cita aktīva ķēde, kas uz tiem atsauktos. Tāpēc trūkstošais Elder Crag Boar `1127` pie `319` ir izņemtas ķēdes ieraksts. [Stocking Jetsteam](https://warcraft.wiki.gg/wiki/Stocking_Jetsteam) · [Evershine](https://warcraft.wiki.gg/wiki/Evershine) · [A Favor for Evershine](https://warcraft.wiki.gg/wiki/A_Favor_for_Evershine) · [Return to Bellowfiz](https://warcraft.wiki.gg/wiki/Return_to_Bellowfiz)

Pēc `9193` jaunākajā datu eksportā ir 5 459 tvēruma ieraksti un 4 865 ieslēgti kvesti. Atbilstošajā Release atrodami visi 160 pasaules skriptu nosaukumi. Kvestiem ar pasaulē novietotu devēju paliek 47 priekšmeta lietošanas kandidāti un 17 statiski neizsekoti aktiera vai kredīta ceļi. Šie ir pārskatāmi kandidāti, kuros ietilpst arī jau pārbaudīti C++ ceļi. Eksports un kontrolsummas ir lokāli `.codex/quest-coverage-20261122-catacombs/`.

Brazie the Botanist `49687` ir novietots Hillsbrad Foothills un piedāvā sešus aktīvus 20. līmeņa kvestus `28733/28617/28744/28747/28748/28751`. `28744` un `28747` datu pārskatā ir atrastas native ieskaites burvestības `92340` un `92435`, tāpēc tās neparādās četrās pilnībā neizsekotajās atlasēs. Pārbaudītajā datubāzē šīm divām burvestībām arī trūkst izsaukuma sasaistes ar minispēles kontroli. Visu sešu kvestu faktiskā iedarbināšana, viļņi un nodošana paliek atvērti.

`27232` trīs esošajiem lielgabaliem atjaunots sēdeklis, Rocket Blast un peldētāju viļņi. Servera pārbaude apstiprināja 50 faktiskas nogalināšanas un nodošanu pie Admiral Hatchet. [The Waters Run Red pārbaude](waters-run-red-2026-11-22.md)

`13885` Orseus `34392` jāatver hipogrifa posms, un Kathrena Winterwisp `34301` piedāvā un pieņem kvestu. 7.3.5 datiem atrasti `65401 → 65372` izsaukums, pasažiere `34376`, `65384` iekāpšana, `65385` pacelšanās un `65373` laukuma burvestība. 2017. gada 4:24 video visa ceļa kadri ar sešu sekunžu soli parāda divus braucējus, Kathrena vadītu lidojumu, spēlētāja darbību uz trīs sugām un nolaišanos pirms nodošanas. Pasaulē vēl trūkst Orseus funkcijas, transportlīdzekļa datu sasaistes, lidojuma koordinātu un progresa sasaistes ar `34323/34324/34325`. [2017. gada gājiens](https://www.youtube.com/watch?v=Rc2HWdXKOVo)
