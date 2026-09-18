# DBErrors labošanas žurnāls

Sākts: 2026-09-18. Projekts: LegionCore 7.3.5, MariaDB datubāzes ar `legion_` prefiksu.

## Drošība un sākuma stāvoklis

- Pirms labojumiem izveidota pilna `legion_world` kopija:
  `build-extractors/runtime-test/dberrors-repair-20260918-170732/legion_world_before_repairs.sql`.
- Kopijas izmērs: 246987584 baiti.
- SHA-256: `D4106A664F183326AD74C4107741D68FB543A71770805547C15FD166AFCEC541`.
- Sākuma žurnālos bija 883 `ERROR` ieraksti: 767 `DBErrors.log`, 115 `Server.log` skriptu kļūdas un 1 arēnas sezonas kļūda.
- Labojumi tiek dalīti mazās paketēs. Katra pakete tiek palaista, pārbaudīta, iecommitota un pushota pirms nākamās paketes.

## Pakete 01 — drošie kustības datu fallback labojumi

Fails: `sql/updates/world/2026_09_18_00_dberrors_low_risk_data.sql`.

Šeit datubāzē tiek saglabātas tieši tās pašas vērtības, kuras kodols agrāk tikai uz starta laiku aizvietoja atmiņā:

- `creature_template_movement` ierakstam 88206 nederīgais `Flight=3` kļūst par `None` (`0`);
- NPC veidnei 522111 nulles `speed_walk` kļūst par standarta `1`;
- GUID 800000–800002 nejaušā kustība ar nulles rādiusu kļūst par idle kustību;
- sešiem GUID ar waypoint kustību, bet bez `path_id`, kustība kļūst par idle.

Nekas netiek dzēsts. Visi vaicājumi ir idempotenti un maina tikai iepriekš pārbaudītās vecās vērtības.

### Pārbaudes rezultāts

- `worldserver` pats piemēroja SQL failu un reģistrēja to `updates` tabulā kā `RELEASED`.
- Serveris sasniedza `ready` 29 sekundēs un tika korekti izslēgts ar exit code 0.
- `bnetserver` sasniedza `ready`; tā žurnālā nebija kļūdu.
- Visi 11 mainītie ieraksti tika atkārtoti nolasīti no MariaDB, un tiem ir paredzētās vērtības.
- Neviens no 11 sākotnējiem brīdinājumiem jaunajā `DBErrors.log` vairs neparādās.
- Jaunajā pilnajā startā bija 726 `DBErrors.log` un 116 `Server.log` kļūdu ieraksti. Kopskaits nav tieši salīdzināms ar 2026-09-08 startu, jo LFG un game-event ielāde ir atkarīga no aktīvā kalendāra; šīs paketes rezultāts tādēļ pārbaudīts pēc konkrētajiem ID un ziņojumiem.

## Spēlē vēlāk pārbaudāmais

- NPC 522111 pārvietojas normālā iešanas ātrumā.
- NPC GUID 800000, 800001 un 800002 stāv uz vietas, nevis mēģina nejauši kustēties ar nulles rādiusu.
- GUID 264517, 267575, 271814, 272174, 272175 un 272176 vairs nemēģina izmantot neesošu waypoint ceļu. Ja tiem pēc dizaina tomēr jāpārvietojas, jāatrod pareizie ceļi un jāpiešķir `path_id`; šī pakete saglabā kodola līdzšinējo drošo fallback uzvedību.

## Pakete 02 — pēc-Legion scaling bāreņu ieraksti

Fails: `sql/updates/world/2026_09_18_01_remove_post_legion_scaling_orphans.sql`.

Septiņiem `creature_template_scaling` ierakstiem nav atbilstošas NPC veidnes. To `VerifiedBuild` ir 27404 vai 27602, bet šis serveris un klients izmanto pēdējo Legion 7.3.5 būvējumu 26972. Ieraksti ienāca ar 2024-11-15 Ashamane scaling datu importu un šajā versijā nav izmantojami.

Pirms izņemšanas SQL nokopē precīzos ierakstus datubāzes tabulā `_backup_20260918_creature_template_scaling_orphans`. Dzēšana ir ierobežota ar precīzu ID sarakstu, neesošu `creature_template` un `VerifiedBuild > 26972`; tā nevar skart vēlāk pievienotu korektu Legion veidni. Pilnā pirmsdarbu datubāzes kopija arī paliek pieejama.

Skartie ID: 135201, 135202, 137762, 139093, 140210, 141119 un 141707.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Aktīvajā tabulā palika 0 no 7 ierakstiem; backup tabulā ir visi 7 ar sākotnējiem `VerifiedBuild`.
- Visi 7 konkrētie kļūdu ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 726 līdz 719.
- `worldserver` sasniedza `ready` 14 sekundēs un tika korekti izslēgts ar exit code 0.

### Atjaunošana

Ja šie dati nākotnē kļūst vajadzīgi kopā ar atbilstošām veidnēm, tos var atjaunot ar:

```sql
INSERT IGNORE INTO creature_template_scaling
SELECT * FROM _backup_20260918_creature_template_scaling_orphans;
```

## Pakete 03 — custom uzdevumu rasu maska

Fails: `sql/updates/world/2026_09_18_02_fix_custom_quest_race_mask.sql`.

Custom uzdevumiem 60010–60030 `AllowableRaces` bija `0`. Šī kodola `QuestData` neierobežotu rasu masku glabā kā `UINT64_MAX` un katrā startā šos 21 ierakstus jau pārveidoja uz šo vērtību tikai atmiņā. SQL saglabā to pašu efektīvo vērtību datubāzē. Tas nemaina spēlētājiem pieejamās rases un nav tikai žurnāla filtra maiņa.

Labojums ir atgriezenisks ar `UPDATE quest_template SET AllowableRaces=0 WHERE ID BETWEEN 60010 AND 60030`, un pilnā pirmsdarbu DB kopija satur sākotnējos ierakstus.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visiem 21 uzdevumiem MariaDB tagad glabā `18446744073709551615` (`UINT64_MAX`).
- Visi 21 rasu maskas kļūdu ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 719 līdz 698.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

## Pakete 04 — uzdevumu izsniedzamo priekšmetu skaits

Fails: `sql/updates/world/2026_09_18_03_fix_quest_provided_item_counts.sql`.

33 uzdevumiem ir derīgs `StartItem`, bet nebija `quest_template_addon` ieraksta, tāpēc `ProvidedItemCount` kļuva par `0`. Kodols šiem priekšmetiem jau pārbaudīja eksistenci un starta laikā piespiedu kārtā lietoja skaitu `1`. SQL izveido trūkstošo addon rindu tikai precīzi zināmajiem uzdevumiem, tikai ja `StartItem <> 0`, un saglabā skaitu `1`.

Tas nemaina faktisko līdzšinējo uzvedību; tas padara datubāzi atbilstošu kodola runtime fallback. Nekas netiek dzēsts. Atgriešanai var dzēst tikai šīs 33 addon rindas, kuru pārējie lauki joprojām ir noklusējumi; pilnā pirmsdarbu kopija satur sākotnējo stāvokli.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visiem 33 uzdevumiem tagad ir addon rinda ar `ProvidedItemCount=1`.
- Visi 33 `SourceItemIdCount` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 698 līdz 665.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Pieņemt dažus skartos uzdevumus (īpaši 7061, 45755 un 48027) un apstiprināt, ka tiek izsniegts tieši viens sākuma priekšmets.
- Atmest un atkārtoti pieņemt uzdevumu, pārliecinoties, ka netiek dublēti vairāki priekšmeti.

## Pakete 05 — exploration/event uzdevumu karodziņi

Fails: `sql/updates/world/2026_09_18_04_fix_quest_exploration_flags.sql`.

64 unikāliem uzdevumiem ir reāla saite uz vismaz vienu no mehānismiem, kam obligāti vajadzīgs `QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT (0x002)`:

- 22 ziņojumi no `SPELL_EFFECT_QUEST_COMPLETE`;
- 35 ziņojumi no `SCRIPT_COMMAND_QUEST_EXPLORED`;
- 12 SmartAI darbības (vairākas lieto vienu un to pašu quest ID), kuras bez karodziņa vispār netika ielādētas.

SQL saglabā visus citus esošos `SpecialFlags`, pievieno tikai bitu `0x002` un trūkstošas addon rindas izveido tikai eksistējošiem `quest_template` ID. Kodols pirmajām divām grupām jau uzlika šo pašu bitu atmiņā; SmartAI grupai labojums papildus atjauno iepriekš izlaisto quest-credit darbību.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visiem 64 unikālajiem uzdevumiem datubāzē ir saglabāts bits `0x002`, nezaudējot iepriekšējos bitus.
- Visi 69 saistītie ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 665 līdz 596.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Izpildīt pa vienam uzdevumam no spell grupas (piemēram, 45546), start-script grupas (14395) un SmartAI grupas (30515 vai 46213).
- Apstiprināt, ka uzdevums nepabeidzas uzreiz bez paredzētā notikuma un pēc notikuma saņem completion credit.

## Pakete 06 — legacy waypoint kustības karodziņi

Fails: `sql/updates/world/2026_09_18_05_normalize_legacy_waypoint_move_flags.sql`.

Ceļiem 501874 un 501877 kopā bija 9 punkti ar `move_type=15`. Vecajā kodolā šis lauks saucās `move_flag` un tika lasīts kā `bool`, tāpēc jebkura nenulles vērtība, arī `15`, nozīmēja skriešanu. Commitā `b4d1807` lauks tika pārsaukts un aizvietots ar enum (`walk=0`, `run=1`, `land=2`, `takeoff=3`), bet migrācija tikai pārsauca kolonnu un nenormalizēja vecās bool vērtības.

SQL precīzi šiem diviem ceļiem pārveido legacy `15` uz semantiski identisko `run=1`. Ceļi pieder Darkshard Crystalback (NPC 78233); punkti vairs netiek izlaisti no ielādes. Nekas netiek dzēsts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Ceļam 501874 visi 5 punkti un ceļam 501877 visi 4 punkti tagad ir `move_type=1`.
- Visi 9 `invalid move_type` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 596 līdz 587.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Mapē 1205 pie koordinātēm aptuveni `(51, 3600, 281)` abi Darkshard Crystalback patrulē pa saviem taisnstūrveida ceļiem un neiestrēgst.

## Pakete 07 — bezdzimuma klienta modeļi

Fails: `src/server/game/Globals/ObjectMgr.cpp`.

Septiņi ziņojumi par `gender=4294967295` nebija no `creature_model_info` SQL kolonnas. Vērtība ir klienta `CreatureDisplayInfo` konstante `GENDER_UNKNOWN=-1`, un skartie display ID galvenokārt pieder trigger, area-tripwire, efektu un summoning-circle modeļiem. Iepriekš kodols tos klasificēja kā kļūdu un piespiedu kārtā padarīja par vīriešu dzimuma modeļiem.

Labojums `GENDER_UNKNOWN` normalizē uz servera paredzēto bezdzimuma vērtību `GENDER_NONE`. Citas patiesi nederīgas vērtības joprojām rada kļūdu un saņem iepriekšējo drošo fallback. SQL dati netiek mainīti vai dzēsti.

Skartie display ID: 20688, 20690, 20698, 21263, 39360, 39464 un 39949.

### Pārbaudes rezultāts

- Windows x64 Release kompilācija un instalēšana `compiles` pabeigta ar exit code 0.
- Visi 7 `wrong gender` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 587 līdz 580.
- `worldserver` ar jauno bināro failu sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Area Force Guard Post/Trip Wire NPC 21993, 21996, 22001 un radniecīgie joprojām aktivizē paredzēto aizsardzības uzvedību.
- Lightning Bomb 56177 un garrison summoning-circle efekti 85477/87392 joprojām ir pareizi neredzami vai attēloti un nerāda nepareizu alternatīvo nosaukumu/modeli.

## Pakete 08 — profesiju uzdevumu prasmju prasības

Fails: `sql/updates/world/2026_09_18_06_fix_quest_required_skills.sql`.

30 profesiju uzdevumiem `quest_template_addon.RequiredSkillId` bija `0` vai addon ieraksta nebija vispār, lai gan negatīvais `QuestSortID` tos piesaista konkrētai profesijai. SQL iestata precīzu 7.3.5 kodola kategoriju kartējuma prasīto prasmi: Blacksmithing 164, Leatherworking 165, Herbalism 182, Tailoring 197, Engineering 202, Inscription 773 vai Archaeology 794. Esošās addon rindās citi lauki netiek mainīti; diviem trūkstošajiem ierakstiem tiek izmantotas shēmas noklusējuma vērtības.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visām 30 rindām DB ir tieši paredzētais `RequiredSkillId`; arī abi iepriekš trūkstošie addon ieraksti ir izveidoti.
- Visi 30 `QuestSortID`/`RequiredSkillId` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 580 līdz 550.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Pie attiecīgā profesijas trenera pārbaudīt pa vienam uzdevumam no vairākām grupām, piemēram, Blacksmithing 47095, Tailoring 44741, Engineering 46119 un Archaeology 41170.
- Pārliecināties, ka uzdevumu var paņemt tikai tēls ar paredzēto profesiju/prasmi un ka jau apgūtam profesionālajam tēlam uzdevums nekļūst nepieejams.

## Pakete 09 — novecojis tukša vehicle pasažiera placeholders

Fails: `sql/updates/world/2026_09_18_07_archive_empty_vehicle_accessory.sql`.

Vecais `sql/old/world/0008_misc.sql` labojums Bloodwing vehicle 94517 sēdekļa 0 pasažieri nomainīja no NPC 95309 uz `accessory_entry=0`, ar komentāru, ka pēc noklusējuma tajā nevienam nav jāatrodas. Kodola loaderis `0` neuztver kā “tukšs”, bet ziņo par neesošu creature un šo rindu pilnībā izlaiž. Tātad placeholders jau bija neaktīvs un tā noņemšana nemaina izpildes uzvedību.

Pirms noņemšanas precīza rinda tiek saglabāta DB tabulā `_backup_20260918_vehicle_accessory_placeholder`; atjaunošana ir iespējama ar `REPLACE INTO vehicle_template_accessory SELECT * FROM _backup_20260918_vehicle_accessory_placeholder`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Aktīvajā tabulā placeholders vairs nav, bet backup tabulā ir saglabāta tieši viena pilna sākotnējā rinda.
- Vienīgais `vehicle_template_accessory: Accessory 0 does not exist` ziņojums pazuda; `DBErrors.log` skaits samazinājās no 550 līdz 549.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Stormheim Horde ievada scenārijā “The Splintered Fleet” pārbaudīt, ka Bloodwing (94517) sākumā ir bez automātiska pasažiera un paredzētajā brīdī var uzņemt scenārija pasažieri/spēlētāju.

## Pakete 10 — no klienta izņemtā “Hate to Zero” aura

Fails: `sql/updates/world/2026_09_18_08_archive_removed_hate_aura.sql`.

`2024_12_11_01_auchenai_crypts_trash_ai.sql` Angered Skeleton template 18521 un 20315 piešķīra veco `Hate to Zero` auru 10095. Šis spell bija vecākos Classic/TBC datos, bet 7.3.5 build 26972 `Spell.db2` tā nav, tādēļ abas auras loaderis jau līdz šim noraidīja. Paša servera `lookup spell Hate to Zero` atrod tikai 63984 un 96735; tie ir instant dummy threat-reset efekti, nevis persistējošas template auras, tāpēc tie nav drošs aizvietotājs.

Pilnas sākotnējās `creature_template_addon` rindas tiek saglabātas `_backup_20260918_removed_hate_aura`. Aktīvajās rindās netiek dzēsti addon ieraksti — tiek iztukšots tikai precīzi `10095` saturošais `auras` lauks. Tas saglabā pašreizējo runtime uzvedību, jo nederīgā aura tāpat netika ielādēta.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Abas aktīvās addon rindas ir saglabātas ar tukšu aura lauku; backup tabulā ir abas pilnās sākotnējās rindas ar `10095`.
- Abi `wrong spell 10095` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 549 līdz 547.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Auchenai Crypts normal un heroic grūtībā pārbaudīt Angered Skeleton (18521/20315) uzvedību: tie korekti parādās, neiestrēgst evade stāvoklī un izmanto paredzēto Infuriate mehāniku.
- Īpaši novērot aggro izvēli pēc skeleton parādīšanās; ja retail references rāda tūlītēju threat reset, tas jāievieš AI notikumā ar 7.3.5 derīgu spell vai tiešu threat darbību, nevis pastāvīgu addon auru.

## Pakete 11 — custom debug NPC apakšizvēlņu navigācija

Fails: `sql/updates/world/2026_09_18_09_fix_debug_gossip_navigation.sql`.

Custom NPC 600022 galvenās izvēlnes opcijas “Items”, “Quest Menu”, “Achievements” un abu apakšizvēlņu “Back to main menu” rindas bija ar `OptionNpc=4` (vendor action), lai gan vienlaikus norādīja `ActionMenuID`. Kodols vendor darbībai apakšizvēlni neatļauj un ignorēja `ActionMenuID`, tāpēc navigācija nedarbojās.

Šīm piecām precīzi identificētajām navigācijas rindām iestatīts `OptionNpc=0` (`None`). Pārējās NPC opcijas saglabā vendor tipu, SmartAI notikumi un vendor preču saraksts netiek mainīti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visām piecām navigācijas rindām DB ir `OptionNpc=0`; citas 600022/600123/600124 opcijas netika mainītas.
- Visi 5 `can not use ActionMenuID` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 547 līdz 542.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Pie custom NPC 600022 atvērt “Items”, “Quest Menu” un “Achievements”; katrai jāatveras pareizajai apakšizvēlnei.
- “Quest Menu” un “Achievements” izvēlnēs nospiest “<- Back to main menu” un pārliecināties, ka atgriežas sākuma izvēlnē.
- Atsevišķi pārbaudīt, ka parastās mount/item opcijas joprojām izpilda savas SmartAI darbības un vendor preces joprojām ir pieejamas paredzētajā ceļā.

## Pakete 12 — Lord Gravesbane dialoga kļūdas zars

Fails: `sql/updates/world/2026_09_18_10_fix_gravesbane_gossip_branch.sql`.

Izvēlnes 19634 opcijai 0 mērķis `19637` bija ierakstīts `ActionPoiID`, lai gan POI 19637 neeksistē. Savukārt gossip menu 19637 eksistē: tā NPC teksts paskaidro, ka romantiskais stāsts nav izdevies, un opcija “Erase everything and try again” ved atpakaļ uz sākumu. Arī izvēles teksts “Umm... Lord Gravesbane?” atbilst šim neveiksmes zaram.

SQL pārvieto precīzo ID no `ActionPoiID` uz `ActionMenuID`; nekas netiek dzēsts un pārējais dialoga koks netiek mainīts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- DB rindai tagad ir `ActionMenuID=19637` un `ActionPoiID=0`.
- Vienīgais `non-existing ActionPoiID 19637` ziņojums pazuda; `DBErrors.log` skaits samazinājās no 542 līdz 541.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts ar exit code 0.

### Spēlē vēlāk pārbaudāmais

- Lord Gravesbane romantiskajā dialogā nonākt izvēlnē 19634 un izvēlēties “Umm... Lord Gravesbane?”. Jāatveras neveiksmes tekstam ar “Erase everything and try again”, nevis izvēlei klusi jābeidzas.
- No neveiksmes izvēlnes nospiest reset opciju un pārliecināties, ka dialogs sākas no sākuma.

## Pakete 13 — ignorēto `script_texts` konfliktu arhivēšana

Fails: `sql/updates/world/2026_09_18_11_archive_duplicate_script_texts.sql`.

`script_texts` primārā atslēga pieļauj vienādu negatīvo `entry` dažādiem NPC, bet kodola ielādētājs tekstus glabā globāli pēc `entry`. Tāpēc 25 vēlāk nolasītās rindas jau līdz šim tika pilnībā ignorētas. Koda un datu salīdzinājums parādīja, ka NPC 15420 un 49869 izmanto SmartAI `Talk`/`creature_text`, bet bosi 27655 un NPC 29032 izmanto C++ `Talk()` ar pilniem `creature_text` ierakstiem. Savukārt konfliktos uzvarējušie vispārīgie teksti un NPC 28406 dueļa teksti joprojām tiek lietoti ar `DoScriptText`, tāpēc tie netika mainīti.

Visas 25 iepriekš ignorētās rindas pirms izņemšanas tiek saglabātas tabulā `_backup_20260918_duplicate_script_texts`. Atjaunošana ir iespējama ar `REPLACE INTO script_texts SELECT * FROM _backup_20260918_duplicate_script_texts`, taču tad atkal rastos tie paši globālo ID konflikti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir tieši 25 pilnas sākotnējās rindas; aktīvajā tabulā neviena no tām nav palikusi.
- Visas 25 konfliktos uzvarējušās rindas ir saglabātas.
- Visi 25 `already loaded entry ... ignored` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 541 līdz 516.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Eversong Woods uzdevumā ar Prospector Anvilward (15420) pārbaudīt, ka dialoga secība joprojām izmanto `creature_text` grupas 0–2.
- Oculus izspēlēt Drakos (27654) un Urom (27655) cīņas; abiem jāsaka savas aggro, spēju, nogalināšanas un nāves replikas, īpaši Urom platformu replikas.
- Death Knight sākuma zonā izspēlēt dueli ar Death Knight Initiate (28406) un uzdevuma “A Special Surprise” sarunu ar NPC 29032; dueļa nejaušajām replikām un garajai izpildes sarunai jāpaliek atšķirīgām un pilnām.
- Pie Stormwind Infantry (49869) pārbaudīt SmartAI ievainojuma/palīdzības tekstus; tiem jānāk no `creature_text`, nevis no vecajiem konfliktējošajiem vispārīgo emote ID.

## Pakete 14 — `creature_text` nederīgie metadati

Fails: `sql/updates/world/2026_09_18_12_fix_creature_text_metadata.sql`.

Yarzill the Merc (23141) vienīgajai replikai bija nederīgs čata tips `100`. Kodols to jau automātiski aizvietoja ar `CHAT_MSG_SAY` (`12`), un tieši šo grupu skripts izsauc Lord Illidan Stormrage uzdevuma dialogā. Datubāzē ierakstīta tā pati faktiskā vērtība `12`.

Stormwind Infantry (49869) SmartAI izsauc “HELP!” teksta grupu 0, bet tai bija `BroadcastTextID=111351`, kura 7.3.5 klienta un hotfix datos nav. Nederīgā izvēles atsauce nomainīta uz `0`; pats lokālais teksts, čata tips un SmartAI darbība saglabāti. Tas atbilst loadera iepriekš izmantotajam fallback, bet tagad nav kļūdas.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- DB Yarzill rindai ir `Type=12` un joprojām derīgais `BroadcastTextID=21825`; Infantry rindai ir `Type=12`, teksts “HELP!” un `BroadcastTextID=0`.
- Abi `CreatureTextMgr` kļūdu ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 516 līdz 514.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Shadowmoon Valley pabeigt “Lord Illidan Stormrage” (11108) dialogu līdz brīdim, kad Yarzill saka “You will not harm...” un piedāvā uzkāpt mugurā; tekstam jāparādās kā parastai NPC runai.
- Pie Stormwind Infantry (49869) sagaidīt SmartAI nejaušo skriptu 4986900; grupas 0 gadījumā kareivim joprojām jāsaka “HELP!”.

## Pakete 15 — tukšo child pool saišu arhivēšana

Fails: `sql/updates/world/2026_09_18_13_archive_empty_child_pools.sql`.

Child pool 5132, 5145, 5618 un 11417 bija saglabāti `pool_pool`, bet tiem nebija `pool_template` un nebija neviena creature, gameobject, quest vai apakš-pool dalībnieka. Kodols visas četras saites jau izlaida, tādēļ tās nevarēja radīt nevienu objektu. Tās ir nepilnīgi izņemtu Dragonblight, Wintergrasp un Icecrown rūdas mezglu atliekas.

Precīzās četras saites pirms izņemšanas saglabātas `_backup_20260918_empty_child_pools`. Pārējie attiecīgo mother pool bērni un to mezgli netika mainīti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir 4 sākotnējās rindas, aktīvajā `pool_pool` neviena no četrām vairs nav.
- Visi 4 `pool_pool included pool_id ... skipped` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 514 līdz 510.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Pārlidot Dragonblight, Wintergrasp un Icecrown rūdas maršrutus un pārliecināties, ka blakus esošie aktīvie cobalt/mineral/ore pool mezgli turpina parādīties un respawn rotācija neiestrēgst.

## Pakete 16 — viena dalībnieka Howling Fjord rūdas pool

Fails: `sql/updates/world/2026_09_18_14_fix_single_member_ore_pools.sql`.

Howling Fjord ore child pool 10150 un 10153 katrā ir tikai viens Rich Cobalt Deposit (GUID 44625 un 44628), taču dalībniekam bija atstāta parastās normal/rich alternatīvas `10%` iespēja. Viena dalībnieka pool tādējādi nesasniedza prasītos 100% un kodols abus pool pilnībā noraidīja.

Vienīgā dalībnieka chance iestatīta uz 100%. Tas neliek abiem mezgliem vienmēr būt pasaulē: mother pool 9901 joprojām izvēlas tikai 30 no saviem 100 spawn-point child pool. Izmaiņa tikai nodrošina, ka tad, kad mother pool izvēlas 10150 vai 10153, tā vienīgais reālais dalībnieks var parādīties.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Abos pool ir tieši viens dalībnieks un katra chance summa ir 100%.
- Abi `Pool Id ... explicit chance sum is not 100` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 510 līdz 508.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Howling Fjord izrakt vairākas mother pool 9901 rūdas rotācijas un pārbaudīt Rich Cobalt Deposit vietas ap `(-144.006, -4989.57, 309.673)` un `(85.848, -4961.88, 316.024)`; mezgliem jāparādās tikai rotācijas ietvaros un pēc savākšanas korekti jārespawno.

## Pakete 17 — Emmigosa atlīdzības daudzums

Fails: `sql/updates/world/2026_09_18_15_fix_emmigosa_reward_count.sql`.

Azsuna uzdevumam “On the Brink” (38015) bija pareizais Emmigosa pet priekšmets 129178, bet `RewardAmount1=0`, tāpēc kodols atlīdzību izlaida. Reference dati šim uzdevumam rāda vienu Emmigosa atlīdzību; quest laikā dotais Radiant Ley Crystal ir atsevišķs provided item un netika mainīts.

`RewardAmount1` iestatīts uz 1 tikai precīzajai rindai ar `RewardItem1=129178`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- DB uzdevumam 38015 tagad ir `RewardItem1=129178` un `RewardAmount1=1`.
- `RewardItemId1 ... RewardItemCount1 = 0` ziņojums pazuda; `DBErrors.log` skaits samazinājās no 508 līdz 507.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Ar tēlu, kuram nav Emmigosa pet, izpildīt Azsuna uzdevumu “On the Brink” (38015): ar provided Radiant Ley Crystal vai heal spējām izārstēt četrus drakonus, nodot uzdevumu un saņemt tieši vienu Emmigosa (129178).
- Izmantot atlīdzības priekšmetu un pārbaudīt, ka Emmigosa tiek pievienota pet journal; atkārtota vai dubulta atlīdzība nedrīkst rasties.

## Pakete 18 — novecojušo garrison reward-summary currency atsauču arhivēšana

Fails: `sql/updates/world/2026_09_18_16_archive_obsolete_summary_currency.sql`.

Quest 33749 (“Quest Reward Summary: Train Depot”) un 33750 (“Quest Reward Summary: Pipeworks”) ir Warlords of Draenor iekšēji reward-summary ieraksti, kurus reference dati atzīmē kā obsolete. Lokālajā DB tiem nav neviena creature vai gameobject startera/endera, objective, area trigger, pool, event, world-quest vai mission saites. Abiem bija Lesser Charm of Good Fortune (`RewardCurrencyID1=738`) ar nulles daudzumu, tādēļ kodols atlīdzību jau noraidīja.

Abas pilnās sākotnējās `quest_template` rindas pirms labojuma tiek saglabātas `_backup_20260918_obsolete_summary_currency`. Aktīvajās rindās tiek notīrīta tikai nulles daudzuma currency atsauce; quest un addon ieraksti netiek dzēsti, un patvaļīgs atlīdzības daudzums netiek piešķirts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir abas pilnās sākotnējās rindas ar `RewardCurrencyID1=738`; aktīvajām rindām tagad ir `RewardCurrencyID1=0` un nemainīts daudzums `0`.
- Abi 33749/33750 `RewardCurrencyCount1 = 0` ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 507 līdz 505.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Ja testa rīki ļauj piespiedu kārtā palaist quest 33749 vai 33750, apstiprināt, ka to pabeigšana nepiešķir veco Lesser Charm of Good Fortune valūtu un nerada klienta vai servera kļūdu.
- Draenor garrison Lumber Mill/Train Depot un Goblin Workshop/Pipeworks darbībā pārbaudīt, ka parastās ēku un darba pasūtījumu atlīdzības turpina nākt no to īstajiem sistēmas datiem, nevis no šiem obsolete summary ierakstiem.

## Pakete 19 — “Crushing the Legion” nederīgās fixed currency atsauces arhivēšana

Fails: `sql/updates/world/2026_09_18_17_archive_invalid_world_quest_currency.sql`.

Stormheim Legion Assault world quest “Crushing the Legion” (46179) ir aktīvs uzdevums ar diviem objectives un `world_quest_update` rotācijas ierakstu. Tā mainīgās world-quest atlīdzības nāk no `world_quest_template` tipam `QuestInfoID=139`, kur ir derīgās Legion valūtas Order Resources (1220) un Nethershards (1226). Quest fiksētajā atlīdzības laukā papildus bija currency 6 ar daudzumu 0; šāda currency 7.3.5 klienta datos nav, un kodols to noraidīja.

Pilnā sākotnējā `quest_template` rinda pirms labojuma tiek saglabāta `_backup_20260918_invalid_world_quest_currency`. Aktīvajam quest tiek notīrīts tikai nederīgais `RewardCurrencyID1=6`; tā objectives, Valarjar reputācija, reward spell, rotācija un dinamiskās atlīdzības netiek mainītas.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir pilnā sākotnējā rinda ar `RewardCurrencyID1=6`; aktīvajam quest tagad ir `RewardCurrencyID1=0` un nemainīts daudzums `0`.
- Abi 46179 ziņojumi — nulles currency daudzums un neeksistējošais currency ID — pazuda; `DBErrors.log` skaits samazinājās no 505 līdz 503.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Stormheim Legion Assault laikā izpildīt “Crushing the Legion” (46179): ar extra-action spēju iznīcināt 6 Fel Corruptors un nogalināt 20 demonus.
- Nododot world quest, jāsaņem 75 Valarjar reputācija un rotācijai atbilstošā mainīgā world-quest atlīdzība; currency ID 6 nedrīkst parādīties, un Order Resources/Nethershards atlīdzībām jāturpina darboties.

## Pakete 20 — obsolete “All The King’s Men” choice reward arhivēšana

Fails: `sql/updates/world/2026_09_18_18_archive_obsolete_quest_choice_reward.sql`.

Quest “All The King’s Men” (26156) un tā Loch Modan ieroču atlīdzības ir Blizzard atzīmētas kā obsolete un spēlētājiem nepieejamas. `item_template` tabulā vēl ir sena build 13329 rinda priekš Dagger of Loch Modan (59021), taču šis kodols reālos item template ielādē no 7.3.5 `ItemSparse.db2`; tajā priekšmeta vairs nav, tādēļ quest atlīdzība nekad nevarēja tikt izsniegta. Quest bija saglabājusies tikai piektā choice atlīdzība 59021 ar daudzumu 1.

Pilnā sākotnējā `quest_template` rinda pirms labojuma tiek saglabāta `_backup_20260918_obsolete_quest_choice_reward`. Aktīvajā rindā tiek notīrīts tikai nederīgais piektās izvēles item ID un tā daudzums. Pats quest, tā ķēdes metadati un creature ender saite netiek dzēsti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir pilnā sākotnējā rinda ar choice item 59021 un daudzumu 1; aktīvajai rindai abi piektās izvēles lauki tagad ir `0`.
- 26156 neeksistējošā reward item ziņojums pazuda; `DBErrors.log` skaits samazinājās no 503 līdz 502.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Parastā spēlē quest 26156 nedrīkst būt iegūstams, jo tas ir obsolete. Pie High Mountain (NPC 1073) pārbaudīt, ka neparādās salauzta “All The King’s Men” reward izvēle.
- Ja quest piespiedu kārtā pievieno ar GM komandu, tā nodošana nedrīkst piedāvāt neeksistējošo Dagger of Loch Modan (59021) vai radīt klienta kļūdu; pārējā vēsturiskā quest ķēde nav ar šo labojumu aktivizēta.

## Pakete 21 — abu “Lost Mail” ieejas ceļu kopīgais Postmaster uzaicinājums

Fails: `src/server/game/Globals/QuestData.cpp`.

Quest 41368 un 41411 abi saucas “Lost Mail”, bet sākas no dažādām priekšmeta versijām: 41368 ir Dalaranā atrodamās BoP vēstules ceļš uz “Return to Sender” 46278, savukārt 41411 ir nododamās/tirgojamās vēstules ceļš uz 41397. Abu `quest_template_addon` dati no build 24367 apzināti izmanto vienu Postmaster mail template 426 ar tēmu “Need help!”. Pēc jebkura ceļa pabeigšanas spēlētājam jāsaņem uzaicinājums uz Postmaster biroju.

Kodola vispārīgā validācija iepriekš pieļāva tikai vienu quest katram mail template un starta laikā atņēma 41411 pasta atlīdzību. Validācijai pievienots šaurs izņēmums tikai template 426 un tieši secīgajam pārim 41368/41411. Visi citi dublēti mail template joprojām tiek noraidīti un ziņoti kā kļūda; datubāzes rindas nav mainītas vai dzēstas.

### Pārbaudes rezultāts

- `worldserver` Release mērķis pārbūvēts bez kompilatora kļūdām un jaunais binārs instalēts `compiles`.
- Starta validācija saglabāja mail template 426 abiem quest; dublikāta ziņojums pazuda un `DBErrors.log` skaits samazinājās no 502 līdz 501.
- Citiem mail template izņēmums nav piemērots; vispārīgā dublikātu pārbaude paliek aktīva.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Atrast Dalaran mailbox tuvumā BoP Lost Mail, izpildīt quest 41368 pie Madam Goya un pārbaudīt, ka pienāk Postmaster vēstule “Need help!” un var sākt 46278.
- Ar citu tēlu izmantot tirgojamo/BoU Lost Mail, izpildīt quest 41411 un pārbaudīt, ka pienāk tas pats uzaicinājums un var sākt 41397.
- Abos ceļos pārbaudīt visu Postmaster ķēdi līdz Katy’s Stampwhistle; katrai quest nodošanai jānosūta tikai viena uzaicinājuma vēstule.

## Pakete 22 — obsolete “Revered Among Thrallmar” zonas arhivēšana

Fails: `sql/updates/world/2026_09_18_19_archive_obsolete_quest_zone.sql`.

TBC reputācijas quest “Revered Among Thrallmar” (10559) no spēles tika izņemts ilgi pirms Legion. Lokālajā DB tam nav startera, endera vai objective, bet bija saglabāts vecais Hellfire Citadel zonas kategorijas ID 3535, kura 7.3.5 klienta `AreaTable` vairs nav. Tādēļ quest nevar korekti piesaistīt šai izņemtajai zonai, un pārcelšana uz citu mūsdienu zonu būtu izdomāta datu maiņa.

Pilnā sākotnējā `quest_template` rinda pirms labojuma tiek saglabāta `_backup_20260918_obsolete_quest_zone`. Aktīvajā rindā tiek notīrīts tikai nederīgais `QuestSortID`; pats vēsturiskais quest un tā Thrallmar reputācijas nosacījums netiek dzēsts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir pilnā sākotnējā rinda ar `QuestSortID=3535`; aktīvajai rindai tagad ir `QuestSortID=0`.
- 10559 neeksistējošās zonas ziņojums pazuda; `DBErrors.log` skaits samazinājās no 501 līdz 500.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Parastā 7.3.5 spēlē quest 10559 nedrīkst parādīties pie Nazgrel vai cita NPC.
- Ja quest piespiedu kārtā pievieno ar GM komandu, quest log nedrīkst rādīt neeksistējošu zonas kategoriju; šis labojums neaktivizē izņemto TBC reputācijas uzdevumu.

## Pakete 23 — “A Slip of the Hand” nederīgā source spell labojums

Fails: `sql/updates/world/2026_09_18_20_fix_aethril_rank3_source_spell.sql`.

Legion Herbalism quest “A Slip of the Hand” (40017) tiek nejauši atklāts, vācot Aethril pēc otrā ranga iegūšanas, un pēc nodošanas apmāca Aethril Rank 3 ar reward spell 193417. Tā `SourceSpellID` laukā kļūdaini bija ierakstīts 40016 — iepriekšējā Herbalism questa ID, nevis spell. Publiskais questa apraksts apstiprina tiešo discovery mehāniku bez starter item, bet visi pārējie tās pašas profesijas Rank 3 discovery questi DB izmanto `SourceSpellID=0` un savu tehniku māca ar `RewardSpell`.

Pilnā sākotnējā `quest_template_addon` rinda tiek saglabāta `_backup_20260918_invalid_quest_source_spell`. Aktīvajā rindā tikai nederīgais `SourceSpellID` tiek labots uz 0; quest, tā nejaušās atklāšanas mehānika, ender NPC un reward spell 193417 netiek mainīti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir sākotnējā 40017 addon rinda ar `SourceSpellID=40016`; aktīvajā rindā tagad ir `SourceSpellID=0`, bet Herbalism prasības un pārējie lauki ir saglabāti.
- 40017 neeksistējošā source spell ziņojums pazuda; `DBErrors.log` skaits samazinājās no 500 līdz 499.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Ar tēlu, kuram ir Aethril Rank 2, vākt Aethril Azsunā līdz automātiski parādās “A Slip of the Hand” (40017); quest nedrīkst prasīt starter item vai mēģināt izpildīt neeksistējošu spell 40016.
- Nodot quest Kuhuine Tenderstride Dalaranā un pārbaudīt, ka tiek apgūta “Herbalism Technique: Aethril (Rank 3)” (193417).
- Ar profesijas “relearn” spēju pārbaudīt, ka jau pabeigta 40017 gadījumā spell 193417 tiek atjaunots.

## Pakete 24 — allied-race galvaspilsētu discovery objective atjaunošana

Fails: `sql/updates/world/2026_09_18_21_restore_allied_race_city_objectives.sql`.

Trim Legion 7.3.5 allied-race sākuma quest “Stranger in a Strange Land” variantiem bija saglabāti pareizie starteri, enderi, lokalizētais mērķa teksts un build 26124 metadati, bet importētajās objective rindās bija pazaudēts gan NPC `ObjectID`, gan discovery flags. Tāpēc kodols tos noraidīja kā neizpildāmus. Tās pašas ķēdes nebojātais Lightforged variants 50313 parāda paredzēto modeli: objective ir attiecīgais galvaspilsētā esošais allied-race emissary ar `Flags=1`.

Atjaunoti tikai trīs esošie discovery objectives no katra questa jau piesaistītā ender NPC: Nightborne quest 50303 izmanto Melitier Vahlouran (131328), Void Elf quest 50305 izmanto Keira Onyxraven (131347), bet Highmountain Tauren quest 50319 izmanto Halian Shlavahawk (131201). Sākotnējās pilnās objective rindas glabājas `_backup_20260918_allied_race_city_objectives`; questi, NPC saites un lokalizācijas nav dzēstas.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Visām trim aktīvajām rindām ir pareizais emissary `ObjectID`, discovery `Flags=1` un atbilstošs apraksts; backup tabulā ir trīs sākotnējās nulles rindas.
- Visi trīs “non existing creature entry 0” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 499 līdz 496.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Ar tikko izveidotu Nightborne pieņemt 50303 no Ambassador Blackguard, atrast Orgrimmar Warchief's Command Board un Melitier Vahlouran; discovery mērķim jāieskaitās un quest jāvar nodot emissary.
- Ar tikko izveidotu Void Elf pieņemt 50305 no Ambassador Moorgard, atrast Stormwind Hero's Call Board un Keira Onyxraven; mērķim jāieskaitās un quest jāvar nodot.
- Ar tikko izveidotu Highmountain Tauren pieņemt 50319 no Ambassador Blackguard, atrast Orgrimmar board un Halian Shlavahawk; mērķim jāieskaitās un quest jāvar nodot.
- Salīdzinājumam pārbaudīt Lightforged quest 50313: tā esošā Vindicator Minkey discovery darbība nedrīkst mainīties.

## Pakete 25 — izņemtā `SHWAYDER TEST` criteria objective arhivēšana

Fails: `sql/updates/world/2026_09_18_22_archive_removed_test_criteria_objective.sql`.

Iekšējam quest 34360 ar nosaukumu `SHWAYDER TEST` nav neviena startera vai endera, un tā vecais build 22522 objective 286350 atsaucās uz CriteriaTree 53086, kura gala 7.3.5.26972 klienta datos vairs nav. Tam pašam test quest vēl ir jaunāks build 23877 talk objective uz eksistējošu creature, tādēļ netika dzēsts pats quest vai jaunākā testa rinda.

Tikai novecojušais criteria objective tiek pilnā veidā saglabāts `_backup_20260918_removed_test_criteria_objective` un pēc tam izņemts no aktīvās `quest_objectives` tabulas. Tas nav spēlētājiem paredzēts saturs, un tam netiek izdomāts aizvietojošs criteria ID.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja un reģistrēja kā `RELEASED`.
- Backup tabulā ir pilnā build 22522 criteria objective rinda; aktīvajā quest saglabāts jaunākais build 23877 talk objective uz creature 110717.
- 34360 neeksistējošā CriteriaTree ziņojums pazuda; `DBErrors.log` skaits samazinājās no 496 līdz 495.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Parastā spēlē quest 34360 nedrīkst būt pieejams, jo tas ir iekšējs tests bez startera vai endera.
- Ja quest tiek pievienots ar GM rīkiem testa vajadzībām, jaunākais talk objective uz creature 110717 joprojām paliek DB; vecais neeksistējošais CriteriaTree 53086 vairs nedrīkst parādīties quest datos.

## Pakete 26 — no Legion izņemtā orku Hunter “Steady Shot” questa arhivēšana

Fails: `sql/updates/world/2026_09_18_23_archive_removed_steady_shot_quest.sql`.

Orku Hunter apmācības quests “Steady Shot” (25139) un tā prasītā Steady Shot spēja 56641 tika izņemti no retail spēles patch 7.0.3. Legion klientā spell vairs nav, tāpēc quest nevar izpildīt un aizvietot to ar citu Hunter spēju būtu nepareiza mehānikas maiņa. Lokālā DB joprojām piedāvāja šo questu pie Karranisha un no iepriekšējā “Etched Parchment” (3087).

Pirms izņemšanas pilnā veidā arhivētas questa template, addon, enUS locale, abas objectives un to locales, reward dialoga un creature starter/ender rindas deviņās `_backup_20260918_obsolete_steady_shot_*` tabulās. Aktīvajā DB noņemts tikai quest 25139 saturs un saites, bet quest 3087 saglabāts, atvienojot gan tā addon `NextQuestID`, gan template `RewardNextQuest` uz 0; abas sākotnējās 3087 rindas ir backupā.

### Pārbaudes rezultāts

- SQL updateris galīgo faila versiju atkārtoti piemēroja pēc ķēdes abu lauku papildināšanas un reģistrēja kā `RELEASED`.
- Quest 25139 aktīvajās template, objective, locale, reward un creature relation tabulās vairs nav; visās deviņās backup tabulās ir paredzētās sākotnējās rindas.
- Quest 3087 palika aktīvs ar abiem turpinājuma laukiem (`RewardNextQuest` un addon `NextQuestID`) iestatītiem uz 0, bet backupā abos saglabāta sākotnējā vērtība 25139.
- 25139 neeksistējošā spell objective ziņojums pazuda un pēc abu ķēdes lauku atvienošanas neradās jauna dangling-chain kļūda; `DBErrors.log` skaits samazinājās no 495 līdz 494.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts.

### Spēlē vēlāk pārbaudāmais

- Izveidot orku Hunter un iziet Valley of Trials sākuma questus: Karranisha nedrīkst piedāvāt izņemto “Steady Shot” (25139), un pēc “Etched Parchment” (3087) nedrīkst parādīties salauzts turpinājums.
- Hunter pamata spēju iegūšanai jāseko Legion 7.3.5 klašu sistēmai; neeksistējošajam spell 56641 vai piecu Steady Shot treniņu objective quest logā nav jāparādās.
