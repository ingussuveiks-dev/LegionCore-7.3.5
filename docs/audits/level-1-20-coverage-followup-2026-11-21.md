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

Brazie the Botanist `49687` ir novietots Hillsbrad Foothills un piedāvā sešus aktīvus 20. līmeņa kvestus `28733/28617/28744/28747/28748/28751`. `28744` un `28747` datu pārskatā ir atrastas native ieskaites burvestības `92340` un `92435`, tāpēc tās neparādās četrās pilnībā neizsekotajās atlasēs. Pārbaudītajā datubāzē šīm divām burvestībām arī trūkst izsaukuma sasaistes ar minispēles kontroli. Visu sešu kvestu faktiskā iedarbināšana, viļņi un nodošana paliek atvērti.

`27232` trīs esošajiem lielgabaliem atjaunots sēdeklis, Rocket Blast un peldētāju viļņi. Servera pārbaude apstiprināja 50 faktiskas nogalināšanas un nodošanu pie Admiral Hatchet. [The Waters Run Red pārbaude](waters-run-red-2026-11-22.md)

`13885` Orseus `34392` jāatver hipogrifa posms, un Kathrena Winterwisp `34301` piedāvā un pieņem kvestu. 7.3.5 datiem atrasti `65401 → 65372` izsaukums, pasažiere `34376`, `65384` iekāpšana, `65385` pacelšanās un `65373` laukuma burvestība. 2017. gada 4:24 video visa ceļa kadri ar sešu sekunžu soli parāda divus braucējus, Kathrena vadītu lidojumu, spēlētāja darbību uz trīs sugām un nolaišanos pirms nodošanas. Pasaulē vēl trūkst Orseus funkcijas, transportlīdzekļa datu sasaistes, lidojuma koordinātu un progresa sasaistes ar `34323/34324/34325`. [2017. gada gājiens](https://www.youtube.com/watch?v=Rc2HWdXKOVo)
