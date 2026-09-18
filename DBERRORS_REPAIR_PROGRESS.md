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

## Pakete 27 — Postmaster ķēdes servera kill-credit veidņu atjaunošana

Fails: `sql/updates/world/2026_09_18_24_restore_postmaster_kill_credits.sql`.

Četri aktīvās Legion Postmaster ķēdes creature objectives atsaucās uz servera kill-credit ID, kuri importa laikā bija izlaisti gan no `creature_template_wdb`, gan `creature_template`: teleportam uz Frozen Throne (104071), Twinkles piemiņas godināšanai (104099), Solid Stone maisa saņemšanai (104177) un nodošanai (104180). Tie nav pasaulē izvietojami NPC — tos kā progresa marķierus piešķir questa spell vai skripts. Wowhead, piemēram, spell 202645 trešajā efektā tieši norāda teleportēšanās kill credit, bet Tauri Legion reference katram ID apstiprina īsto nosaukumu, 1. līmeni, neitrālo Creature faction un clientā eksistējošo display 42661.

Abās creature template tabulās tikai pievienotas četras trūkstošās rindas; esošie questi, objectives, spelli un skripti nav mainīti vai dzēsti. `INSERT ... WHERE NOT EXISTS` nosacījumi pasargā no jau eksistējošu datu pārrakstīšanas. Servera veidnēm izmantoti 7.3.5 build 26124 metadati un Legion expansion 6.

Izmantotās atsauces:

- <https://www.wowhead.com/npc=104071/kill-credit-teleport-to-the-frozen-throne>
- <https://www.wowhead.com/spell=202645/teleport-to-the-frozen-throne>
- <https://legion-shoot.tauri.hu/?npc=104071>
- <https://legion-shoot.tauri.hu/?npc=104099>
- <https://legion-shoot.tauri.hu/?npc=104177>
- <https://legion-shoot.tauri.hu/?npc=104180>

### Pārbaudes rezultāts

- SQL updateris pēc obligāto `WorldEffects` un `PassiveSpells` tukšo vērtību precizēšanas failu atkārtoti piemēroja bez SQL kļūdām un reģistrēja jauno checksum.
- Visas četras rindas ielasās no abu template tabulu apvienojuma ar pareizajiem nosaukumiem, `Displayid1=42661`, `RequiredExpansion=6`, `VerifiedBuild=26124`, 1. līmeni un faction 35.
- Visi četri neeksistējošo creature 104071/104099/104177/104180 ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 494 līdz 490.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts. Šis labojums neradīja jaunus šo veidņu validācijas ziņojumus.

### Spēlē vēlāk pārbaudāmais

- Izpildīt “A Huge Package” (41397): saņemt 1,362 Solid Stone maisu un nogādāt to Ethereal Portal. Abiem mērķiem secīgi jāieskaitās (104177 un 104180), un quest jāvar nodot Postmaster.
- Izpildīt “Priority Delivery” (41367): izmantot teleportu uz Frozen Throne. Teleportam jānostrādā un objective 104071 jāieskaitās automātiski; pēc tam jāturpina atlikušais quest ceļš līdz Steam Pools.
- Izpildīt “Due Reward” (41395): pie Twinkles kapa jāvar nolikt rotaļlietu, jāieskaitās objective 104099 un quest jāvar nodot Wilson.
- Pārbaudīt, ka neviena no četrām tehniskajām kill-credit būtnēm nav redzami/spontāni izvietota pasaulē; tās drīkst eksistēt tikai kā progresa ID.

## Pakete 28 — Argus ievada galvaspilsētu portālu kill-credit atjaunošana

Fails: `sql/updates/world/2026_09_18_25_restore_argus_portal_kill_credits.sql`.

Četri aktīvie “The Hand of Fate” varianti — Alliance 47221/48506 un Horde 47835/48507 — satur izvēles mērķi izmantot Dalaran portālu uz savas frakcijas galvaspilsētu. Šie objectives pareizi atsaucās uz kill-credit 123564 vai 124365, taču abi tehniskie creature ieraksti importa laikā bija izlaisti no abām template tabulām. Wowhead spell 245992 un 247057 datos pirmais efekts tieši piešķir attiecīgi Stormwind vai Orgrimmar portāla kill credit; Tauri Legion reference apstiprina abus ID, nosaukumus, 1. līmeni, Creature faction un display 42661.

Atjaunotas tikai divas servera kredīta veidnes ar 7.3.5 build 26124 metadatiem. Četri questa objectives un to izvēles statuss nav mainīts, un nekas nav dzēsts. Nosacītie inserti nepārraksta jau eksistējošas rindas.

Izmantotās atsauces:

- <https://www.wowhead.com/spell=245992/portal-stormwind>
- <https://www.wowhead.com/spell=247057/portal-orgrimmar>
- <https://legion-shoot.tauri.hu/?npc=123564>
- <https://legion-shoot.tauri.hu/?npc=124365>

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām.
- Abi ID ielasās no `creature_template` un `creature_template_wdb` ar pareizajiem nosaukumiem, `Displayid1=42661`, Legion expansion un build 26124.
- Visi četri atkārtotie 123564/124365 objective ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 490 līdz 486.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jaunus abu veidņu validācijas ziņojumus tas neradīja.

### Spēlē vēlāk pārbaudāmais

- Ar Alliance tēlu pieņemt “The Hand of Fate” 47221 un atkārtot tā rezerves variantu 48506: izmantot Dalaran portālu uz Stormwind. Izvēles mērķim 123564 jāieskaitās pirms/pēc teleportācijas, pēc tam jāvar satikt escort Stormwind Harbor.
- Ar Horde tēlu tāpat pārbaudīt 47835 un 48507, izmantojot Dalaran portālu uz Orgrimmar. Izvēles mērķim 124365 jāieskaitās, pēc tam jāvar turpināt uz Bladefist Bay.
- Abām frakcijām pārbaudīt arī ceļu, kur izvēles portāla mērķis tiek izlaists un uz ostu dodas citādi; questa obligātā daļa nedrīkst būt bloķēta.
- Tehniskās kill-credit būtnes nedrīkst parādīties pasaulē kā redzami NPC.

## Pakete 29 — Deaths of Chromie scenārija sliekšņu kredītu atjaunošana

Fails: `sql/updates/world/2026_09_18_26_restore_chromie_scenario_kill_credits.sql`.

Chromie scenārija questi “Preserve the True Future” (47904) un “The Deaths of Chromie” (47906) pareizi prasa apturēt attiecīgi četrus un astoņus uzbrukumus, bet to gala creature objectives atsaucās uz importa laikā izlaistiem servera kredītiem 124644 un 124646. Tauri Legion reference tieši sasaista 124644 “Save 4 Chromies Credit” ar 47904 un 124646 “Save 8 Chromies Credit” ar 47906; abiem norādīts 1. līmenis un display 47169. Wowhead Legion datos abi tāpat ir klasificēti kā tehniski, pasaulē neizvietoti credit NPC.

Abās creature template tabulās pievienotas tikai abas trūkstošās sliekšņu veidnes ar Legion/build 26124 metadatiem. Questa skaitītāji, scenārija skripti un objectives nav mainīti vai dzēsti, un nosacītie inserti nepārraksta esošus datus.

Izmantotās atsauces:

- <https://legion-shoot.tauri.hu/?npc=124644>
- <https://legion-shoot.tauri.hu/?npc=124646>
- <https://www.wowhead.com/npc=124644/save-4-chromies-credit>
- <https://www.wowhead.com/npc=124646/save-8-chromies-credit>

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām.
- Abi ieraksti ielasās no abu template tabulu apvienojuma ar pareizajiem nosaukumiem, `Displayid1=47169`, Legion expansion, build 26124, 1. līmeni un faction 35.
- Abi 124644/124646 objective ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 486 līdz 484.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jaunas šo veidņu validācijas kļūdas neradās.

### Spēlē vēlāk pārbaudāmais

- Pieņemt “Preserve the True Future” (47904), ieiet “The Deaths of Chromie” scenārijā un apturēt četrus dragonshrine uzbrukumus. Pēc ceturtā glābšanas notikuma objective 124644 jāieskaitās un quest jāvar nodot Chromie.
- Pieņemt “The Deaths of Chromie” (47906) un apturēt visus astoņus uzbrukumus. Pēc astotā notikuma jāieskaitās 124646 un quest jāvar pabeigt.
- Pārbaudīt robežvērtības: 4/8 kredīts nedrīkst tikt dots par agru, atkārtots notikums nedrīkst skaitīties divreiz, un pēc scenārija restarta skaitītājam jāatbilst questa progresam.
- 124644 un 124646 nedrīkst būt redzami vai spawn-oti NPC; tie ir tikai scenārija progresa ID.

## Pakete 30 — “nākamās burvestības” proc izpildes fāze

Fails: `sql/updates/world/2026_09_18_27_fix_next_spell_proc_phase.sql`.

Astoņām aktīvām, ar lādiņiem ierobežotām aurām `spell_proc` rindā bija norādīts lādiņu skaits, bet nebija obligātās burvestības izpildes fāzes. 7.3.5 `SpellAuraOptions` jau dod katras auras pareizo proc tipu un iespējamību, tādēļ SQL nepārraksta klienta datus un pievieno tikai trūkstošo `spellPhaseMask=1` (`CAST`). Tas patērē auru vienreiz uz atbilstošu burvestības izpildi, nevis atkārtoti uz katru trāpīto mērķi. Pirms izmaiņas visu astoņu rindu pilnas kopijas saglabātas `_backup_20260918_next_spell_proc_phase`.

Labotie efekti ir Solar Empowerment (164545), Lunar Empowerment (164547), Gathering Storms (198300), Taste for Blood (206333), Rhonin's Assaulting Armwraps (208081), Precise Strikes (209493), Shattered Defenses (209706) un Galactic Guardian (213708). Nosaukumi, efekta apraksti, proc tipi un iespējamības pārbaudīti pret lokāli lejupielādētajām build 26972 `Spell` un `SpellAuraOptions` tabulām; izpildes fāzes nozīme pārbaudīta šī koda `SpellMgr::CanSpellTriggerProcOnEvent` implementācijā.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir tieši astoņas sākotnējās rindas.
- Visām astoņām dzīvajām rindām ir `spellPhaseMask=1`, bet to pārējās pielāgotās vērtības nav mainītas.
- Astoņi “spellPhaseMask value defined” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 484 līdz 476. Palika tikai divi atsevišķi ziņojumi par nepilnīgo Necrosis (216974) rindu, kas tiks labota nākamajā paketē.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jaunas šo astoņu proc ierakstu validācijas kļūdas neradās.

### Spēlē vēlāk pārbaudāmais

- Balance Druid: iegūt Solar Empowerment un Lunar Empowerment; katram efektam jāpastiprina un jāpatērējas tikai pie nākamā attiecīgi Solar Wrath vai Lunar Strike. Pretēja burvestība nedrīkst patērēt lādiņu.
- Enhancement Shaman: ar Crash Lightning trāpīt vairākiem mērķiem, tad lietot Stormstrike. Gathering Storms bonuss jāpiemēro vienam Stormstrike cast un jānoņem tikai vienreiz, neatkarīgi no trāpījumu skaita.
- Fury Warrior: pēc Furious Slash Taste for Blood jāietekmē nākamais Bloodthirst; cita spēja nedrīkst patērēt efektu.
- Arcane Mage ar Rhonin's Assaulting Armwraps: pēc Arcane Missiles proc nākamajam Arcane Blast jābūt bez mana izmaksām, un aura jānoņem tieši pēc šī cast.
- Arms Warrior: pēc Colossus Smash atsevišķi pārbaudīt Precise Strikes un Shattered Defenses. Nākamajam Mortal Strike vai Execute jāsaņem attiecīgais rage/damage/critical bonuss, un AoE vai neveiksmīgs mērķa rezultāts nedrīkst patērēt auru vairāk nekā vienreiz.
- Guardian Druid: Galactic Guardian automātiskajam Moonfire jāpiešķir 213708; nākamajam manuālajam Moonfire jāsaņem rage un direct-damage bonuss, pēc tam aurai jāpazūd.

## Pakete 31 — Necrosis sekundārās auras proc definīcija

Fails: `sql/updates/world/2026_09_18_28_fix_necrosis_proc.sql`.

Unholy Death Knight talants Necrosis (207346) pēc Death Coil bojājuma piešķir sekundāro vienas lietošanas auru 216974, kurai jāpastiprina nākamais Scourge Strike vai tā talanta aizvietotājs Clawing Shadows. Atšķirībā no primārās auras 216974 build 26972 datos nav savas `SpellAuraOptions` rindas, tādēļ tās nepilnais `spell_proc` ieraksts nevarēja saņemt noklusēto proc tipu vai iespējamību un nekad nenostrādāja.

Ierakstam definēts Death Knight spell family 15, klienta efekta class mask (`0 / 134217728 / 128 / 0`), melee-damage spell proc tips 16, `CAST` fāze un 100% iespējamība. Tas saglabā jau esošo vienu charge/modcharge un nodrošina, ka lādiņu patērē tikai Scourge Strike/Clawing Shadows ģimenes burvestība, nevis jebkura melee spēja. Pilna sākotnējā rinda pirms labojuma saglabāta `_backup_20260918_necrosis_proc`.

Datu salīdzināšanai izmantotas build 26972 `Spell`, `SpellEffect`, `SpellAuraOptions`, `SpellClassOptions`, `SpellCategories` un `SpellMisc` tabulas no Wago DB2 eksportiem. Tajās Scourge Strike (55090) un Clawing Shadows (207311) ir Death Knight ģimenes melee spelli ar kopīgu class-mask bitu, bet 216974 efekts ir tieši 40% spell modifier šai maskai.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena pilna sākotnējā rinda.
- Dzīvajā 216974 ierakstā ir paredzētā DK family/mask, `typeMask=16`, `spellPhaseMask=1`, `chance=100`, kā arī saglabāti `charges=1` un `modcharges=1`.
- Abi atlikušie `spell_proc` ziņojumi pazuda un šajā kategorijā vairs nav nevienas kļūdas; kopējais `DBErrors.log` skaits samazinājās no 476 līdz 474.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauna Necrosis vai cita `spell_proc` validācijas kļūda neradās.

### Spēlē vēlāk pārbaudāmais

- Ar Unholy Death Knight izvēlēties Necrosis, uzbrukt mērķim ar Death Coil un pārliecināties, ka bojājums piešķir 216974 auru.
- Kamēr 216974 ir aktīva, lietot citas melee spējas: tās nedrīkst patērēt Necrosis. Nākamajam Scourge Strike jāsaņem 40% bonuss un pēc viena cast aurai jāpazūd.
- Atkārtot ar izvēlētu Clawing Shadows: tai jāsaņem tas pats bonuss un jāpatērē tieši viens lādiņš arī no distances.
- Pārbaudīt neveiksmīgu/atceltu cast un vairākus mērķus: aura nedrīkst pazust pirms derīga Scourge Strike/Clawing Shadows cast un nedrīkst tikt patērēta vairākas reizes viena cast laikā.

## Pakete 32 — svešo The Paraxis Dalaran spawn arhivēšana

Fails: `sql/updates/world/2026_09_18_29_archive_stray_dalaran_paraxis.sql`.

Creature 124445 ir Eonar encounter kuģis The Paraxis. Tā C++ AI atrodas `AntorusTheBurningThrone/boss_eonar.cpp`, paļaujas uz Antorus `InstanceScript`, un Eonar cīņā šo būtni dinamiski izsauc encounter īpašnieks. Importā papildus bija divi pilnīgi identiski statiski spawni ar GUID 146853540 un 146853552 virs Dalaran Eventide uz atvērtās pasaules kartes 1220. Abiem bija vienādas koordinātas, nebija phase, event, addon, waypoint, formation vai condition saites, un šī bija vienīgā 124445 statiskā spawn vieta.

Šie nav derīgi Eonar encounter spawni: Antorus ir raid karte 1712, savukārt Dalaran ir continent/world karte un tajā raid instance-bind semantika nevar darboties. Abas pilnās importa rindas saglabātas `_backup_20260918_stray_dalaran_paraxis` un tikai pēc tam izņemtas no aktīvās `creature` tabulas. `creature_template`, raid AI un dinamiskā summon uzvedība nav mainīta.

### Pārbaudes rezultāts

- Sākotnējā migrācijas versija koordinātas salīdzināja kā precīzus `FLOAT` un droši neveica nevienu izmaiņu. Nosacījums tika labots uz unikālo GUID/entry/map kombināciju; updateris izmainīto checksum atkārtoti piemēroja bez SQL kļūdām.
- Backup tabulā ir tieši abas pilnās rindas ar sākotnējiem GUID un koordinātām; aktīvajā `creature` tabulā abu vairs nav.
- Abi The Paraxis instance-bind ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 474 līdz 472. Palicis viens atšķirīgs Deathbringer Saurfang transporta validācijas ziņojums.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; Paraxis rindu izņemšana neradīja jaunas trūkstošu saistību kļūdas.

### Spēlē vēlāk pārbaudāmais

- Dalaran Eventide un debesīs virs tā nedrīkst būt divi pārklājošies, statiski The Paraxis (124445) raid NPC.
- Antorus Eonar encounter sākt vismaz normal un heroic grūtībā: encounter izsauktajam The Paraxis jāparādās, jāuzsāk paredzētie portālu/kuģa notikumi un jāsaņem encounter-frame stāvoklis.
- Pabeigt Eonar cīņu un pārbaudīt, ka Paraxis nāve joprojām piešķir encounter 2075/raid progresu un pēc wipe tas tiek dinamiski izsaukts no jauna.

## Pakete 33 — instance-bind validācija instanču transportiem

Faili: `src/server/game/DataStores/DB2Structure.h` un `src/server/game/Globals/ObjectMgr.cpp`.

Deathbringer Saurfang (37813) ir korekti novietots gan Icecrown Citadel raid kartē 631, gan uz Orgrim's Hammer transporta kartes 673. Tā `CREATURE_FLAG_EXTRA_INSTANCE_BIND` ir nepieciešama, lai bosa nāve piesaistītu raid saglabājumu, taču loaderis agrāk par derīgu uzskatīja tikai pašu dungeon/raid/scenario karti un neņēma vērā, ka pie transporta piesaistītas būtnes datubāzē glabājas transporta paša kartē.

Build 26972 `Map` klienta datos karte 673 ir “Transport: Orgrim's Hammer (Icecrown Citadel Raid)” ar `MapType=3`. Kopā ir 63 šī tipa transporta kartes. `MapEntry` pievienota šaura `IsTransportMap()` pārbaude, un creature validācija tagad pieņem instance karti vai transporta karti. Parasta atvērtās pasaules karte joprojām saņem kļūdu, ja uz tās mēģina izmantot instance-bind flag. Saurfang spawn un template netika mainīts vai dzēsts.

### Pārbaudes rezultāts

- Pilna Release kompilācija ar Visual Studio 2022/MSBuild pabeidzās sekmīgi (`exit code 0`); jaunais `worldserver.exe` tika uzinstalēts `compiles` mapē.
- Ar jauno bināro failu Saurfang GUID 146816955 transporta false-positive pazuda, un neviena `INSTANCE_BIND` kļūda vairs nepalika.
- `DBErrors.log` skaits samazinājās no 472 līdz 471; datubāzes rindas šī koda labojuma laikā netika mainītas.
- Pārbūvētais `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts. Starta virsrakstā bija jaunā dirty-tree revīzija `55e2a0c+`, kas apliecina, ka tests izmantoja tikko kompilēto kodu.

### Spēlē vēlāk pārbaudāmais

- Icecrown Citadel 10/25 režīmā iziet gunship daļu uz Orgrim's Hammer un sākt Deathbringer Saurfang cīņu; boss jāielādē uz transporta bez spawn/AI problēmām.
- Nogalinot Saurfang, raid grupai joprojām jāsaņem permanent instance bind un encounter progress; pēc reloga/restarta nogalinātajam bosam nav jāatdzimst saglabātajā lockout.
- Atkārtot vismaz vienā heroic režīmā (`spawnMask=8`), jo tieši transporta ieraksts izmanto šo masku.

## Pakete 34 — Draenor garnizona sūtījumu validācija

Fails: `src/server/game/Globals/ObjectMgr.cpp`.

Septiņi `GAMEOBJECT_TYPE_GARRISON_SHIPMENT` objekti atsaucas uz klienta `CharShipmentContainer` ierakstiem 30, 36, 57, 59, 60, 62 un 63. Build 26972 datos visi septiņi ir `GarrTypeID=2` — Draenor garnizona sūtījumi — ar konkrētiem Shipyard, Lumber Mill, Alchemy, Enchanting, Engineering, Jewelcrafting vai Leatherworking sūtījumu aprakstiem. Tie nav nezināmi vai Legion versijai neatbilstoši dati.

Kodols jau pilnvērtīgi apstrādā abus atbalstītos tipus: `GARRISON_TYPE_GARRISON=2` un `GARRISON_TYPE_CLASS_ORDER=3`. Piemēram, sūtījumu izveidē Draenor tipam tiek meklēts atbilstošais plot/building un izmantota ēkas ietilpība, bet Class Hall tipam — klases halles sūtījumu limits. Tikai pasaules objektu ielādes validācija kļūdaini atļāva vienīgi Class Hall tipu un tādēļ septiņus derīgus WoD objektus atmeta ar kļūdu.

Validācija tagad atļauj abus kodola apkalpotos tipus un turpina noraidīt jebkuru citu `GarrTypeID`, kļūdas tekstā norādot arī neatbalstīto vērtību. Neviens gameobject, shipment vai DB2 ieraksts nav mainīts vai dzēsts.

### Pārbaudes rezultāts

- Build 26972 `CharShipmentContainer` datos visiem septiņiem objektu izmantotajiem container ID apstiprināts `GarrTypeID=2` un atbilstošs Draenor sūtījuma saturs.
- Datubāzē apstiprināts, ka četri Alliance profesionu objekti atrodas kartē 1116, Lumber Mill objekts — Horde garnizona kartē 1159, zivju muca — Shipyard kartē 1330, bet Engineering pasūtījums — Orgrimmar kartē 1.
- Release kompilācija pabeidzās sekmīgi (`exit code 0`), un jaunais `worldserver.exe` tika uzinstalēts `compiles` mapē.
- Visi septiņi `GARRISON_SHIPMENT ... GarrTypeID != GARRISON_TYPE_CLASS_ORDER` false-positive ziņojumi pazuda; neviens jauns `unsupported GarrTypeID` ziņojums neradās. `DBErrors.log` skaits samazinājās no 471 līdz 464.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts. `Server.log` nav jaunu `FATAL`, assertion, trūkstošu failu vai neveiksmīgas ielādes ziņojumu.

### Spēlē vēlāk pārbaudāmais

- Alliance Draenor garnizonā pie atbilstoši uzbūvētām Alchemy, Enchanting, Jewelcrafting un Leatherworking ēkām atvērt darba pasūtījumu objektus, iesniegt materiālus, sagaidīt pabeigšanu un saņemt paredzētos profesiju reaģentus.
- Horde Draenor garnizonā pie Lumber Mill iesniegt timber darba pasūtījumu; tam jāparādās Garrison Report, jāpabeidzas un jāizdod Garrison Resources.
- Garnizona Shipyard pārbaudīt Barrel of Draenor Fish sūtījumu: mijiedarbībai jāatver sūtījuma logs, rindai jāizpildās un gatavajai piegādei jābūt savācamai.
- Orgrimmar pārbaudīt “Order for engineering workshop” objektu ar tēlu, kam pieejams attiecīgais WoD Engineering pasūtījums; objektam jāatver pareizais sūtījuma interfeiss un jāizmanto Gearspring Parts container.
- Kā regresijas testu Class Order Hall izveidot un savākt vismaz vienu Legion sūtījumu; `GarrTypeID=3` darbība nedrīkst mainīties.

## Pakete 35 — Professor Pallin nulles priekšmetu nosacījuma arhivēšana

Fails: `sql/updates/world/2026_09_18_30_archive_zero_item_condition.sql`.

Inscription trenera Professor Pallin (92195) gossip izvēlnei 18598/1 “Here are the cards you wanted.” datubāzē bija divi vienādi item 129092 nosacījumi: derīgā rinda pieprasa 10 priekšmetus, bet importa dublikāts pieprasa neiespējamu skaitu 0 un tādēļ ConditionMgr to vienmēr izlaida. Arī vecākajā 2020. gada LegionCore world dump abi ieraksti ir blakus, kas apstiprina, ka nulles rinda nav alternatīva funkcija.

Pilna nederīgā rinda pirms izņemšanas saglabāta `_backup_20260918_zero_item_condition`. Derīgais 10 priekšmetu nosacījums, questa 39933 nosacījums un pati gossip izvēlne nav mainīti.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir tieši viena pilna nulles skaita rinda, bet aktīvajos datos palicis tieši viens item 129092 nosacījums ar `ConditionValue2=10`.
- “Item condition has 0 set for item count” ziņojums pazuda; `DBErrors.log` skaits samazinājās no 464 līdz 463.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; `Server.log` nav jaunu fatālu, assertion vai trūkstošu resursu ziņojumu.

### Spēlē vēlāk pārbaudāmais

- Ar tēlu, kam aktīvs izvēlnei vajadzīgais quests 39933, bet ir mazāk nekā 10 item 129092, Professor Pallin izvēlei “Here are the cards you wanted.” nav jāparādās.
- Ar to pašu tēlu savākt tieši 10 item 129092; izvēlei jāparādās, jānostrādā un jāpatērē/jāapstrādā priekšmeti tā, kā paredz attiecīgais Inscription questa skripts.
- Atkārtot ar vairāk nekā 10 priekšmetiem un pēc questa pabeigšanas, pārbaudot, ka skaita robeža darbojas, bet izvēle ārpus vajadzīgā questa nav pieejama.

## Pakete 36 — spell implicit-target distances nosacījumu parametri

Fails: `sql/updates/world/2026_09_18_31_fix_spell_distance_conditions.sql`.

Trim spell implicit-target conditions rindām bija palicis vecā divu parametru distances formāta izkārtojums `(10, 4, 0)`: 10 jardu attālums un salīdzinājums “mazāks vai vienāds”. Šajā kodola ConditionMgr `CONDITION_DISTANCE_TO` formāts ir `(otrs condition target, distance, comparison)`, tādēļ skaitlis 10 tika kļūdaini interpretēts kā neeksistējoša target slota numurs un nosacījumi netika ielādēti.

Rindas pārveidotas uz `(1, 10, 4)`: pārbaudāmais implicit target slots 0 tiek salīdzināts ar otru pieejamo spell target slotu 1, attālums ir 10 jardi un `COMP_TYPE_LOW_EQ=4`. Tas atbilst komentāram “Only in 10 Yards” un citu strādājošu SourceType 13 distances nosacījumu izkārtojumam. Labojums attiecas uz “Tied Up” (181555) mērķi 35845 un abiem “Leading Musken” (214176) alternatīvajiem mērķiem 107852/108538. Visas trīs pilnās sākotnējās rindas saglabātas `_backup_20260918_spell_distance_conditions`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir visas trīs sākotnējās rindas, un visas trīs aktīvās rindas tagad ir `(1, 10, 4)`.
- Visi trīs “DistanceTo condition has invalid ConditionValue1 ... (10)” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 463 līdz 460.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauni distance-condition vai servera ielādes ziņojumi neradās.

### Spēlē vēlāk pārbaudāmais

- Draenor saturā atrast notikumu, kas lieto “Tied Up” (181555) uz Dave's Industrial Light and Magic Bunny (35845): casts drīkst izvēlēties mērķi 10 jardu robežās, bet nedrīkst izvēlēties tālāku mērķi.
- Highmountain saturā ar abiem Stout Highlands Runehorn (107852) un Highlands Runehorn (108538) variantiem pārbaudīt “Leading Musken” (214176): katram attiecīgajam ElseGroup jāizvēlas pareizais creature tips tikai 10 jardu robežās.
- Īpaši pārbaudīt robežu ap 10 jardiem un vairākus derīgus/tālus NPC vienlaikus, lai implicit-target atlasītājs neizvēlētos tālo vienību un neizlaistu tuvāko.

## Pakete 37 — Ysera Nightmare taxi auras efekta indekss

Fails: `sql/updates/world/2026_09_18_32_fix_ysera_taxi_aura_condition.sql`.

Val'sharah phase definition 7558/28 ir saistīta ar “Summon Taxi Ysera to Nightmare” (183851), bet tās negatīvais aura nosacījums mēģināja atrast efekta indeksu 3. Build 26972 `SpellEffect` dati rāda četrus spell efektus, taču vienīgais unit-owned aura efekts ir nulles bāzes indekss 2 (`SPELL_EFFECT_APPLY_AURA`, aura type 4). Indekss 3 ir atsevišķs trigger efekts, tāpēc `HasAuraEffect(183851, 3)` nevar raksturot šīs auras klātbūtni un loaderis to pamatoti noraidīja.

Nosacījumam mainīts tikai efekta indekss no 3 uz 2; spell ID, negatīvā pārbaude un phase definition nav mainīta. Tas ļauj parastajām Val'sharah fāzēm būt aktīvām, kamēr taxi aura nav uz spēlētāja, un izslēgties lidojuma/notikuma laikā. Pilna sākotnējā rinda saglabāta `_backup_20260918_ysera_taxi_aura_condition`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena sākotnējā rinda, un aktīvais nosacījums tagad pārbauda efekta indeksu 2.
- “Aura condition has non existing effect index (3)” ziņojums pazuda; `DBErrors.log` skaits samazinājās no 460 līdz 459.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns šīs phase definition vai auras validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Val'sharah questa posmā, kas izsauc Ysera Nightmare taxi spell 183851, sākt lidojumu/notikumu un pārbaudīt, ka spēlētājs saņem tā aura efektu, bet parastās phase definition 7558/28 fāzes lidojuma laikā nav redzamas.
- Pabeigt un arī pārtraukt taxi braucienu; pēc auras noņemšanas parastajai Val'sharah videi un NPC jāatgriežas bez reloga.
- Atkārtot pēc servera restarta un ar spēlētāju, kam aura nekad nav bijusi, lai negatīvais nosacījums neradītu tukšu vai nepareizi nofāzētu zonu.

## Pakete 38 — nederīgā Stormwind phase avota dublikāta arhivēšana

Fails: `sql/updates/world/2026_09_18_33_archive_invalid_phase_source.sql`.

Stormwind phase definition 1519/8 “Stormwind. Legion quests” bija divas pilnīgi vienādas negatīvas quest 42740 pārbaudes. Derīgā rinda izmanto `CONDITION_SOURCE_TYPE_PHASE_DEFINITION=23`; otra rinda kļūdaini izmantoja neeksistējošu source type 41. Skaitlis 41 šajā kodolā ir quest objective condition tips, nevis condition avota tips, un tādēļ dublikāts nekad netika ielādēts.

Pilna nederīgā source-41 rinda saglabāta `_backup_20260918_invalid_phase_source` un izņemta no aktīvās tabulas. Identiskais funkcionālais source-23 nosacījums ar to pašu grupu, entry, ElseGroup, questu un negatīvo zīmi ir atstāts nemainīts, tāpat kā pati phase definition.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena nederīgā source-41 rinda, bet aktīvajos datos palikusi viena identiskā source-23 rinda.
- “Invalid ConditionSourceType 41” ziņojums pazuda; `DBErrors.log` skaits samazinājās no 459 līdz 458.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns 1519/8 phase definition validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Ar Alliance tēlu Stormwind pārbaudīt Legion sākuma ķēdes fāzes pirms, aktīva un pēc “The Battle for Broken Shore” (42740): phase 7714/7552 objektiem jāmainās atbilstoši questa statusam.
- Pārbaudīt gan ceļu, kur 42740 vēl nav pieņemts, gan completed/rewarded stāvokli un sekojošo “In the Blink of an Eye” (44663), lai Stormwind Legion objekti neparādītos dubulti un nepazustu par agru.

## Pakete 39 — “Enfilade” questa pieņemšanas priekšnosacījumi

Fails: `sql/updates/world/2026_09_18_34_fix_enfilade_prerequisites.sql`.

Frostfire Ridge questa “Enfilade” (32994) pieņemšanas nosacījumiem bija saglabāti pareizie prerequisite quest ID — “Karg Unchained” (33785) un “Where's My Wolf?!” (33826) — un komentārs “true if player reward quest”, bet abām rindām condition tips bija nulle. Tips 0 nav izpildāms nosacījums, tādēļ loaderis abas rindas izlaida un Enfilade varēja kļūt pieejams bez paredzētā ķēdes progresa.

Abām rindām atjaunots `CONDITION_QUESTREWARDED=8`. To apstiprina blakus esošais tās pašas zonas quests “The Master Siegesmith” (33828), kas izmanto tieši abus tos pašus quest ID, ElseGroup un derīgo tipu 8. Pilnas sākotnējās rindas saglabātas `_backup_20260918_enfilade_prerequisites`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir abas sākotnējās rindas, un abas aktīvās rindas tagad izmanto condition tipu 8.
- Abi “Invalid ConditionType 0 at SourceEntry 32994” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 458 līdz 456.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns 32994 prerequisite validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Ar Horde tēlu Frostfire Ridge mēģināt pieņemt “Enfilade” (32994), pirms pabeigti abi prerequisite questi; questam nav jābūt pieņemamam.
- Atsevišķi pabeigt tikai “Karg Unchained” un tikai “Where's My Wolf?!” (izmantojot testa tēlus/quest statusu); ar vienu rewarded priekšnosacījumu Enfilade joprojām nav jāatver.
- Kad abi 33785 un 33826 ir rewarded, “Enfilade” jāparādās un jābūt pieņemamam. Pārbaudīt arī blakus “The Master Siegesmith” (33828), kura esošā prerequisite uzvedība nedrīkst mainīties.

## Pakete 40 — Teron sakāves SmartAI auru dublikātu arhivēšana

Fails: `sql/updates/world/2026_09_18_35_archive_teron_aura_duplicates.sql`.

Tehniskā kill-credit creature 231022 SmartAI otrajam notikumam bija trīs nederīgi condition tipa 0 ieraksti, kuri komentārā prasīja, lai spēlētājam nebūtu “Scene: Teron Defeated” auru 182164, 182166 un 182167. Katrā gadījumā tajā pašā SourceGroup, SourceEntry un ElseGroup jau eksistē precīzs derīgs `CONDITION_AURA=1` ieraksts ar to pašu spell ID un negatīvo zīmi.

Trīs tipa-0 dublikāti saglabāti `_backup_20260918_teron_aura_duplicates` un izņemti no aktīvās tabulas. Trīs funkcionālie aura nosacījumi paliek nemainīti, tāpat kā SmartAI darbības, kas piešķir kill credit 91738 un liek invokerim castot 182164.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir trīs nederīgie dublikāti, bet aktīvajos datos palikuši tieši trīs derīgie type-1 aura nosacījumi.
- Visi trīs “Invalid ConditionType 0 at SourceEntry 231022” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 456 līdz 453.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns 231022 SmartAI condition ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Draenor Teron'gor/Teron sakāves ainā aktivizēt kill-credit 231022 bez nevienas no trim auras: credit 91738 jāpiešķiras un invokerim jāsākas 182164 ainai.
- Kamēr spēlētājam jau ir 182164, 182166 vai 182167, atkārtota aktivizācija nedrīkst vēlreiz sākt Teron Defeated ainu vai dubultot credit.
- Atsevišķi pārbaudīt Alliance/Yrel un Horde/Liadrin scenārija variantus, jo 182166 un 182167 aizsargā katras frakcijas summons no dublēšanās.

## Pakete 41 — loot-item questa nosacījumu parametru nobīde

Fails: `sql/updates/world/2026_09_18_36_fix_loot_quest_conditions.sql`.

Trīs `CONDITION_SOURCE_TYPE_LOOT_ITEM` ieraksti item 129747, 129928 un 143776 dropiem bija paredzēti kā “spēlētājs vēl nav nodevis questu” nosacījumi. To quest ID 40168, 40173 un 45563 kļūdaini atradās `ConditionValue2`, kamēr `CONDITION_QUESTREWARDED=8` quest ID lasa no `ConditionValue1`. Rezultātā kodols mēģināja pārbaudīt neeksistējošu questu 0 un loot ierobežojumu vispār neielādēja.

Quest ID pārvietoti uz `ConditionValue1`, `ConditionValue2` notīrīts, un negatīvais nosacījums saglabāts. Katrai rindai blakus paliek tās esošais timewalking nosacījums, tāpēc priekšmets var krist tikai atbilstošajā eventā un tikai līdz attiecīgā questa nodošanai. Pilnas sākotnējās rindas saglabātas `_backup_20260918_loot_quest_conditions`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir visas trīs sākotnējās rindas, un aktīvajos datos katram ierakstam quest ID tagad ir `ConditionValue1`, bet `ConditionValue2=0`.
- Trīs no četriem “Quest condition specifies non-existing quest (0)” ziņojumiem pazuda; atlikušais ir nesaistīts gossip ieraksts. `DBErrors.log` skaits samazinājās no 453 līdz 450.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns šo loot nosacījumu validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Timewalking laikā ar tēlu, kurš nav nodevis questu 40168, iegūt loot item 129747 no paredzētā avota; priekšmetam jābūt pieejamam, bet ārpus timewalking eventa tas nedrīkst krist.
- Tāpat pārbaudīt item 129928 / quest 40173 un item 143776 / quest 45563 kombinācijas.
- Pēc katra attiecīgā questa nodošanas atkārtot loot avotu: quest priekšmets vairs nedrīkst krist. Pārbaudīt arī tēlu, kurš questu tikai pieņēmis, bet vēl nav nodevis — negatīvais rewarded nosacījums vēl drīkst atļaut dropu.

## Pakete 42 — Fjorlin Frostbrow otrās gossip izvēles questa ID

Fails: `sql/updates/world/2026_09_18_37_fix_fjorlin_gossip_quest.sql`.

Fjorlin Frostbrow (29732) izvēlnes 9891 otrā opcija “I am ready to join the battle against the wyrms...” bija apzīmēta ar komentāru “Show gossip option 1 if player has quest 12869”, taču faktiskajā condition rindā quest ID bija 0. Tajā pašā izvēlnē opcija 0 jau pareizi izmanto 12869, un datubāzē tas ir quests “Pushed Too Far”.

Trūkstošais `ConditionValue1` atjaunots uz 12869; pārējie nosacījuma lauki un gossip izvēle nav mainīti. Pilna sākotnējā rinda saglabāta `_backup_20260918_fjorlin_gossip_quest`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir sākotnējā quest-0 rinda, un aktīvais nosacījums tagad izmanto questu 12869.
- Pēdējais “Quest condition specifies non-existing quest (0)” ziņojums pazuda; `DBErrors.log` skaits samazinājās no 450 līdz 449.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns Fjorlin gossip validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Bez aktīva “Pushed Too Far” (12869) sarunāties ar Fjorlin Frostbrow; nevienai no abām ar šo questu saistītajām izvēlēm nav jāparādās.
- Pieņemt 12869 un runāt ar Fjorlin: jāparādās gan testa izvēlei, gan iespējai pievienoties cīņai pret wyrm/eagle transporta izvēlei.
- Izmantot otro izvēli un pārbaudīt, ka tā sagatavo paredzēto ērgli/lidojuma cīņu; pēc questa pabeigšanas izvēlei atkal jāpazūd.

## Pakete 43 — “Kill Your Hundred” SmartAI questa nosacījums

Fails: `sql/updates/world/2026_09_18_38_fix_smart_quest_condition.sql`.

SmartAI nosacījums entry 600, event 10 bija pareizi definēts kā `CONDITION_QUESTTAKEN=9` questam “Kill Your Hundred” (34429), taču `ConditionValue2` bija palicis vecs boolean 1. Šis condition tips lasa tikai `ConditionValue1` kā quest ID; liekais lauks nemaina semantiku un loaderis par to ziņoja kā par nederīgiem datiem.

`ConditionValue2` notīrīts uz 0, saglabājot quest ID, source/event atslēgas un pozitīvo “has quest” pārbaudi. Pilna sākotnējā rinda saglabāta `_backup_20260918_smart_quest_condition`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir sākotnējā rinda, un aktīvajā rindā saglabāts quest 34429 ar `ConditionValue2=0`.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts. Šo konkrēto SmartAI SourceId loaderis arī pirms labojuma neizvadīja atsevišķā kļūdu rindā, tādēļ `DBErrors.log` kopskaits palika 449; jauni ziņojumi neradās.

### Spēlē vēlāk pārbaudāmais

- Bez aktīva “Kill Your Hundred” (34429) izraisīt ar entry 600/event 10 saistīto SmartAI notikumu; tā darbībai 165265 nav jāizpildās.
- Pieņemt 34429 un atkārtot notikumu: nosacījumam jāizpildās un paredzētajam invoker cast 165265 jānostrādā tieši vienreiz.
- Pēc questa nodošanas/izņemšanas atkārtot, lai “has quest” pārbaude vairs nebūtu patiesa.

## Pakete 44 — Rogue artifact phase liekā nosacījuma arhivēšana

Fails: `sql/updates/world/2026_09_18_39_archive_rogue_phase_duplicate.sql`.

Phase definition 5287/1 (phase 5709) vienā ElseGroup saturēja divus citādi identiskus `CONDITION_QUEST_NONE=14` ierakstus questam 40847. Derīgajai rindai visi neizmantotie lauki ir 0; otrai importa rindai bija lieks `ConditionValue3=1`, kuru quest nosacījums nelasa un par kuru loaderis ziņoja.

Pilna nederīgā dublikāta rinda saglabāta `_backup_20260918_rogue_phase_duplicate` un izņemta. Funkcionāli identiskais type-14 nosacījums ar `value3=0` paliek aktīvs, tādēļ Rogue artifact fāzes loģika netiek noņemta vai apklusināta.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir nederīgais dublikāts, bet aktīvajos datos palikusi viena derīgā `value3=0` rinda.
- Viens no diviem “Quest condition has useless data in value3 (1)” ziņojumiem pazuda; atlikušais pieder citam SmartAI objective ierakstam. `DBErrors.log` skaits samazinājās no 449 līdz 448.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns phase 5287/1 validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Ar Rogue, kuram quests 40847 vēl nav pieņemts, ieiet attiecīgajā Dalaran/zone 5287 artifact ķēdes vietā un pārbaudīt phase 5709 objektus.
- Pieņemt 40847 un pārbaudīt, ka šī “quest none” fāze tiek noņemta/pārslēgta; pēc questa pabeigšanas un reloga nevajadzētu rasties dubultiem NPC vai pazudušai videi.

## Pakete 45 — aktīvo questu nosacījumu lieko objective lauku tīrīšana

Fails: `sql/updates/world/2026_09_18_40_normalize_active_quest_conditions.sql`.

Divi `CONDITION_QUESTTAKEN=9` ieraksti saturēja pareizu quest ID, bet arī laukus, kurus šis condition tips nelasa. Margaux (109223) gossip 19908/0 questam 42833 `value2` atkārtoja tā pirmā objective ObjectID 109241; šai pašai izvēlei jau ir atsevišķs type-41 objective nosacījums. “Darkness Falls” (33837) SmartAI kill-credit 231013 aktīvā questa rindā bija ielikts cita mērķa ObjectID 82283 un boolean `value3=1`; turpat ir atsevišķs nosacījums, ka quests vēl nav completed.

Abām type-9 rindām atstāts tikai to izmantotais quest ID un notīrīti `value2/value3`. Objective pārbaudes, quest-completed pārbaude, gossip izvēle un SmartAI darbības nav mainītas. Pilnas sākotnējās rindas saglabātas `_backup_20260918_active_quest_conditions`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir abas sākotnējās rindas, un abiem aktīvajiem type-9 nosacījumiem `value2=0`, `value3=0`.
- Visi atlikušie “Quest condition has useless data” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 448 līdz 445.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauni abu questu nosacījumu validācijas ziņojumi neradās.

### Spēlē vēlāk pārbaudāmais

- Suramar ar questu 42833 pārbaudīt Margaux izvēli “There is still hope...”: tai jābūt pieejamai paredzētajā aktīvā questa/objective stāvoklī un jāatver menu 19905; bez questa vai pēc vajadzīgā progresa tā nedrīkst apiet atsevišķo objective nosacījumu.
- “Darkness Falls” (33837) laikā sasniegt kill-credit 231013: kamēr quests aktīvs un nepabeigts, jāpiešķiras credit 76450 un jāizpildās invoker cast 163805.
- Atkārtot pēc 33837 pabeigšanas un ar tēlu bez questa; SmartAI nedrīkst atkārtoti piešķirt credit vai sākt ainu. Objective “Enter the Waning Crescent” (ObjectID 82283) progresam jāturpinās no paša questa datiem, nevis no type-9 liekajiem laukiem.

## Pakete 46 — Coordinator SmartAI apakšzonu nosacījumi

Fails: `sql/updates/world/2026_09_18_41_fix_coordinator_area_conditions.sql`.

Trīs Coordinator SmartAI nosacījumi entry 123984, 123991 un 124246 pārbaudīja apgabalu ID 6457 vai 5287 ar `CONDITION_ZONEID=4`. 7.3.5.26972 `AreaTable` dati apstiprina, ka 6457 ir “New Tinkertown” apakšzona zem Dun Morogh (1), bet 5287 ir “The Cape of Stranglethorn” apakšzona zem Stranglethorn Vale (5339). Tādēļ loaderis pareizi noraidīja šos ID kā zonas.

Tikai šīs trīs rindas pārslēgtas uz `CONDITION_AREAID=23`; pārējie Coordinator nosacījumi, kuros tiešām lietoti augšējā līmeņa zone ID 1, 10, 1519 un 1537, nav mainīti. Pilnas sākotnējās rindas saglabātas `_backup_20260918_coordinator_area_conditions`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir trīs sākotnējās type-4 rindas, un aktīvajās rindas izmanto type 23 ar tiem pašiem area ID.
- Visi trīs “Zone ... does not exist (is a subzone)” ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 445 līdz 442.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauni Coordinator condition vai trūkstošu datu ziņojumi neradās.

### Spēlē vēlāk pārbaudāmais

- New Tinkertown apakšzonā pie Coordinator 123984 un 123991 pārbaudīt, ka tie izpilda paredzētos “Direct Runners” roku vizuālos efektus 246864/245979 un servera logā nav SmartAI condition kļūdu.
- Stormwind un Duskwood Coordinator 123984, kā arī Stormwind Coordinator 123991, jāsaglabā attiecīgais alternatīvais vizuālais efekts; labojums nedrīkst mainīt to zonas nosacījumus.
- The Cape of Stranglethorn pie Coordinator 124246 jānostrādā “Direct Runners Dance” 246780, bet Ironforge variantam jāsaglabājas 246862.

## Pakete 47 — Sack of Roasted Grain loot kolonnas labojums

Fails: `sql/updates/world/2026_09_18_42_fix_roasted_grain_loot.sql`.

`gameobject_loot_template` entry 40870 (to izmanto gameobject 210002 “Stolen Barley Sack”) ID 138623 bija ievietots `Currency` kolonnā. 7.3.5.26972 `ItemSparse` dati apstiprina, ka 138623 ir parasts priekšmets “Sack of Roasted Grain”, nevis valūta. Tādēļ loaderis rindu noraidīja ne tikai grupas dēļ, bet tā arī nevarēja piešķirt paredzēto priekšmetu.

Ierakstam atjaunots `Item=138623, Currency=0`; 100% iespēja, loot mode, grupa un daudzums nav mainīti. Pilna sākotnējā rinda saglabāta `_backup_20260918_roasted_grain_loot`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena sākotnējā rinda, bet aktīvajā loot rindā ir `Item=138623`, `Currency=0` un saglabāts `GroupId=1`.
- Ziņojums par grouped currency 138623 pazuda; `DBErrors.log` skaits samazinājās no 442 līdz 441.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns entry 40870 loot vai trūkstoša item ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Atrast un atvērt gameobject 210002 “Stolen Barley Sack”; atbilstošajā loot variantā jāsaņem viens “Sack of Roasted Grain” (138623), nevis nederīgas valūtas ieraksts.
- Pārbaudīt, ka otrs entry 40870 grupā neesošais loot “Sack of Grain” (77033) joprojām ir pieejams paredzētajā situācijā un abas alternatīvas netiek izsniegtas kā valūta.

## Pakete 48 — novecojušās Highmaul Honor Points atlīdzības arhivēšana

Fails: `sql/updates/world/2026_09_18_43_archive_obsolete_highmaul_honor.sql`.

`item_loot_template` entry 119000 “Highmaul Lockbox” saturēja 4% iespēju piešķirt 23 vienības currency 392. ID 392 ir vecā Honor Points valūta, kura tika izņemta, mainot Legion PvP atlīdzību sistēmu, un 7.3.5.26972 klienta `CurrencyTypes` datos tās vairs nav. Tādēļ serveris šo rindu nevar ielādēt vai korekti piešķirt; aizstāšana ar citu Legion valūtu mainītu oriģinālo atlīdzību bez pamatojuma.

Pilna novecojusī valūtas rinda saglabāta `_backup_20260918_obsolete_highmaul_honor` un izņemta no aktīvās loot tabulas. Visas Highmaul Lockbox priekšmetu atlīdzības ir saglabātas.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena pilna currency-392 rinda, aktīvajā tabulā tās vairs nav, un entry 119000 palikušas 54 priekšmetu loot rindas.
- Ziņojums “currency entry 392 not exists” pazuda; `DBErrors.log` skaits samazinājās no 441 līdz 440.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns Highmaul Lockbox vai trūkstošas valūtas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Atvērt vairākus “Highmaul Lockbox” (119000) un pārbaudīt, ka kastes atveras, izsniedz atlikušās Draenor/PvP priekšmetu atlīdzības un neizraisa klienta vai servera kļūdu par currency 392.
- Pārbaudīt Highmaul Coliseum zaudētāja pasta atlīdzību, ja šis saturs serverī ir pieejams: kastei jābūt saņemamai un atveramai; Legion versijā nav sagaidāmi vecie Honor Points.

## Pakete 49 — loot grupu iespējas validācija pa grūtības režīmiem

Fails: `src/server/game/Loot/LootMgr.cpp`.

Pieciem Siege of Orgrimmar bossu loot ID (71161, 71504, 71515, 71529 un 71865) un gameobject loot 221739 grupā bija pa trim 100% reference rindām. Tās nav vienlaikus aktīvas: rindas lieto atsevišķus `LootMode` bitus 1, 2 un 4 dažādiem raid grūtības režīmiem. Runtime jau filtrē loot pēc viena aktīvā `_DifficultyMask` bita, bet starta validācija saskaitīja visu režīmu iespējas kopā un nepareizi ziņoja 300%.

`LootGroup::Verify` tagad, tāpat kā runtime, aprēķina grupas summu katram no 16 difficulty bitiem atsevišķi un ziņo lielāko reāli vienlaikus aktīvo summu. `LootMode=0` joprojām tiek uzskatīts par aktīvu visos režīmos. Tā pati pārbaude attiecas uz explicit un zero/equal-chance rindu konfliktu. Datu rindas un to drop iespējas nav mainītas; īstās 140% un 1200% grupas validācija turpina atrast.

### Pārbaudes rezultāts

- Release `INSTALL` kompilācija pabeidzās bez kļūdām un uzstādīja jauno `worldserver.exe`.
- Seši nepatiesie 300% ziņojumi pazuda; `DBErrors.log` skaits samazinājās no 440 līdz 434. Īstās gameobject 210220 140% un reference 228138 1200% kļūdas joprojām tiek ziņotas, kā paredzēts.
- `worldserver` ar jauno bināru sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauna loot validācijas vai starta regresija neradās.

### Spēlē vēlāk pārbaudāmais

- Siege of Orgrimmar attiecīgajos grūtības režīmos nogalināt Kil'ruk the Wind-Reaver (loot 71161), Siegecrafter Blackfuse (71504), General Nazgrim (71515), Current Bloodthirsty (71529) un Garrosh Hellscream (71865); katrā režīmā jāizmanto tikai tā `LootMode` reference, bez dubultiem citu grūtību dropiem.
- Atvērt “Vault of Forbidden Treasures” loot 221739 katrā atbalstītajā grūtībā un pārbaudīt, ka tiek izvēlēta tikai attiecīgā 100% reference tabula.

## Pakete 50 — Elementium Fragment neatkarīgās loot iespējas

Fails: `sql/updates/world/2026_09_18_44_fix_elementium_fragment_loot.sql`.

Dragon Soul gameobject 210220 “Elementium Fragment” loot ID 210220 satur “Essence of Destruction” (71998, daudzums 1–3) un “Elementium Gem Cluster” (77952, daudzums 1), katru ar 70% iespēju. Abi priekšmeti ir dokumentēti kā šīs Deathwing lādes saturs, un tie var krist neatkarīgi. Kļūdains kopīgs `GroupId=2` tos padarīja savstarpēji izslēdzošus un grupas summu pacēla līdz 140%.

Abām rindām iestatīts `GroupId=0`, saglabājot 70% iespēju, loot mode un daudzumus. Tā abas tiek mestas neatkarīgi un vienā lādē drīkst parādīties arī kopā. Pilnas sākotnējās rindas saglabātas `_backup_20260918_elementium_fragment_loot`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir abas sākotnējās rindas, bet aktīvajām 71998 un 77952 rindām tagad ir `GroupId=0` ar saglabātu 70% iespēju un daudzumiem.
- Elementium Fragment 140% grupas ziņojums pazuda; `DBErrors.log` skaits samazinājās no 434 līdz 433.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; jauns 210220 loot vai abu priekšmetu validācijas ziņojums neradās.

### Spēlē vēlāk pārbaudāmais

- Pēc Madness of Deathwing uzvaras atvērt “Elementium Fragment” (210220) vairākos resetos: 71998 un 77952 katram jābūt neatkarīgai iespējai, tādēļ iespējams saņemt abus, vienu vai nevienu no tiem.
- Pārbaudīt 10 un 25 spēlētāju/LFR atbalstītos režīmus (`LootMode=15`), ka 71998 skaits paliek 1–3 un 77952 skaits paliek 1; pārējās ieroču, mount un quest loot grupas nedrīkst mainīties.

## Pakete 51 — Legion pasaules bonusu equal-chance reference grupa

Fails: `sql/updates/world/2026_09_18_45_fix_world_bonus_reference_chances.sql`.

`reference_loot_template` entry 228138 satur 12 Legion pasaules bonusu konteinerus, tostarp “Scavenged Cloth”, “Found Sack of Gems”, “Sprocket Container”, “Argunite Cluster” un “Light's Fortune”. Katra rinda bija norādīta ar `Chance=100` vienā `GroupId=1`, tādēļ vispārīgā grupu validācija pamatoti saskaitīja 1200%.

Šo reference izmanto tikai `world_loot_template` entry 6 rinda ar item 128554. Koda `ProcessWorld` grupas apstrāde visus grupas dalībniekus vispirms filtrē un tad izvēlas vienu ar vienādu varbūtību; tā neizmanto atsevišķo rindu `Chance` svarus. Tādēļ 12 rindām `Chance` nomainīts no maldinoša `100` uz korektu equal-chance vērtību `0`, saglabājot entry, item ID, `GroupId=1`, loot mode un daudzumus. Runtime izvēles mehānika nemainās. Pilnas sākotnējās rindas saglabātas `_backup_20260918_world_bonus_reference_chances`.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir visas 12 sākotnējās rindas, bet aktīvajām rindām tagad ir `Chance=0`, `GroupId=1` un saglabāti item ID, loot mode un daudzumi.
- Reference 228138 1200% ziņojums pazuda; `DBErrors.log` skaits samazinājās no 433 līdz 432. Neviens no 12 bonusu item ID jaunā kļūdā neparādījās.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts. `Server.log` ir iepriekš zināmās script-hook un ArenaSeason problēmas, bet nav jauna reference 228138 vai Legion bonusu loot ziņojuma.

### Spēlē vēlāk pārbaudāmais

- Ar katru atbalstīto Legion plecu enchant variantu nogalināt parastus Broken Isles/Argus pretiniekus un ilgākā paraugā pārbaudīt, ka attiecīgais bonusu konteiners joprojām var parādīties: 140220, 140221, 140222, 140224, 140225, 140226, 140227, 142259, 144330, 144345, 153202 vai 153248.
- Vienā reference izsaukumā drīkst tikt izvēlēts ne vairāk kā viens no 12 konteineriem; nedrīkst parādīties visu konteineru komplekts vai pazust Legion pasaules parastais loot.

## Pakete 52 — Enslaved Son of Arkkoroc kvesta loot atjaunošana

Fails: `sql/updates/world/2026_09_18_46_restore_arkkoroc_quest_loot.sql`.

Creature 36868 “Enslaved Son of Arkkoroc” korekti norāda `lootid=36868`, taču šāds `creature_loot_template` entry bija pilnīgi tukšs. Atsauces noņemšana tikai apklusinātu kļūdu un atstātu saturu salauztu: bāzes quest 14487 “Still Beating Heart” objektīvs prasa vienu item 49642 “Heart of Arkkoroc”, un publiskie spēles dati šo item dokumentē kā Enslaved Son of Arkkoroc dropu. Item 49642 eksistē arī 7.3.5.26972 `ItemSparse` datos.

Atjaunota viena 100% `QuestRequired=1` loot rinda item 49642 ar vienu eksemplāru. Tā ir redzama un izkrīt tikai spēlētājam ar atbilstošu aktīvu kvestu. Esošā creature template rinda pirms labojuma saglabāta `_backup_20260918_arkkoroc_loot_source`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updateris failu piemēroja bez kļūdām; backup tabulā ir viena pilna sākotnējā creature template rinda, un aktīvajā loot tabulā ir tieši viena paredzētā quest-only item 49642 rinda.
- Neesošā creature loot ID 36868 ziņojums pazuda; `DBErrors.log` skaits samazinājās no 432 līdz 431. Jauna item 49642 vai Arkkoroc loot kļūda neradās.
- `worldserver` sasniedza `ready` 12 sekundēs un tika korekti izslēgts; `Server.log` 36868/49642 kontekstā satur tikai SQL updatera informatīvo ierakstu.

### Spēlē vēlāk pārbaudāmais

- Pieņemt quest 14487 “Still Beating Heart”, Ruins of Eldarath nogalināt Enslaved Son of Arkkoroc (36868) un pārbaudīt, ka loot satur tieši vienu “Heart of Arkkoroc” (49642) un kvesta objektīvs tiek ieskaitīts.
- Bez aktīva quest 14487 nogalināt to pašu NPC un pārbaudīt, ka quest-only sirds nav redzama. Quest 14472 “In The Face!” nogalināšanas kredītam un pārējiem Azshara ķēdes posmiem jāturpina darboties.

## Pakete 53 — Outland un Northrend raktuvju gem reference atjaunošana

Fails: `sql/updates/world/2026_09_18_47_restore_mining_gem_references.sql`.

Fel Iron, Rich Adamantite, Khorium, Cobalt, Rich Cobalt, Saronite, Rich Saronite un Titanium loot rindas atsaucās uz neesošām reference grupām 12901, 12902, 12904, 12905 un 12906. Šī nebija novecojusi funkcija: tās ir klasiskās mining papildu gemu grupas, un identiskas references joprojām ir uzturētajā 3.3.5 datubāzē. Visi 24 unikālie gemu item ID eksistē 7.3.5.26972 klienta datos.

Atjaunotas piecas sešu gemu equal-chance grupas: Outland uncommon, Outland rare, Rich Outland uncommon ar daudzumu 1–2, Northrend uncommon un Northrend rare. Esošo depozītu ārējās 1%/5% iespējas nav mainītas, tādēļ reference tikai atkal nodrošina paredzēto gemu tad, kad depozīta rinda veiksmīgi izkrīt. Visas 13 avota `gameobject_loot_template` rindas saglabātas `_backup_20260918_mining_gem_reference_sources`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL piemērots `legion_world`; pilns `worldserver` starts pabeigts 12 sekundēs bez starta apstāšanās.
- `DBErrors.log` kļūdu skaits samazinājās no 431 līdz 418. Visi 13 atkārtotie brīdinājumi par reference 12901, 12902, 12904, 12905 un 12906 vairs neparādās.
- Serveris pēc pārbaudes korekti apturēts ar konsoles komandu `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Vairākkārt izrakt Fel Iron Deposit, Rich Adamantite Deposit un Khorium Vein: pamatmateriāliem jāsaglabājas, bet reizēm papildus jāizkrīt vienam no atbilstošajiem Outland uncommon/rare gemiem; Rich Adamantite common reference drīkst dot 1–2 gemus.
- Vairākkārt izrakt Cobalt, Rich Cobalt, Saronite, Rich Saronite un Titanium depozītus: pamatmateriāliem jāsaglabājas, bet 5% reference gadījumā jāparādās vienam Northrend uncommon vai rare gemam. Viena reference izsaukuma ietvaros nedrīkst izkrist visi seši grupas gemi.

## Pakete 54 — Blackfathom zema līmeņa world-loot reference atjaunošana

Fails: `sql/updates/world/2026_09_18_48_restore_blackfathom_world_loot_reference.sql`.

Blackfathom Tide Priestess (entry 4802) loot tabulā bija 5% atsauce uz neesošu grupu 24070. Tā ir viena no deviņām šī NPC zema līmeņa world-loot grupām; pārējās astoņas grupas datubāzē bija saglabātas. No uzturētās 3.3.5 bāzes atjaunots grupas pilnais saturs — 37 vienādas iespējas auduma, ādas, bruņu, ieroču, vairogu un Tigerseye varianti. Visi 37 item ID atsevišķi pārbaudīti 7.3.5.26972 `ItemSparse` datos.

Esošā avota rinda pirms labojuma saglabāta `_backup_20260918_blackfathom_reference_source`; nekas nav dzēsts. Reference saglabā sākotnējo 5% ārējo iespēju, un `GroupId=1` izvēlas ne vairāk kā vienu no 37 variantiem.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; datubāzē ir 37 grupas rindas un viena saglabāta avota backup rinda.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 418 līdz 417, un reference 24070 kļūda vairs neparādās.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Blackfathom Deeps atkārtoti nogalināt Blackfathom Tide Priestess (4802) un pārbaudīt parasto loot. Retajos 5% reference gadījumos jāizkrīt vienam, nevis visiem, grupas zema līmeņa world-drop priekšmetiem; esošajam quest un pamata loot jāsaglabājas.

## Pakete 55 — Cache of Eregos reward reference atjaunošana

Fails: `sql/updates/world/2026_09_18_49_restore_eregos_cache_reference.sql`.

Heroic Oculus beigās Cache of Eregos (gameobject 191349, loot entry 24462) ar 100% iespēju atsaucās uz neesošu reward grupu 35041 un prasīja no tās divus priekšmetus. No uzturētās bāzes atjaunoti visi astoņi paredzētie dungeon reward priekšmeti. Katrs item ID un nosaukums pārbaudīts 7.3.5.26972 `ItemSparse` datos.

Esošā reference avota rinda saglabāta `_backup_20260918_eregos_cache_reference_source`; nekas nav dzēsts. Atjaunotā equal-chance grupa saglabā esošo `MinCount=1`, `MaxCount=2` ārējā rindā, tāpēc lāde var izvēlēties līdz diviem dažādiem reward variantiem, bet turpina atsevišķi dot esošo recipe un Stone Keeper's Shard loot.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; datubāzē ir astoņas reward grupas rindas un viena saglabāta avota backup rinda.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 417 līdz 416, un reference 35041 kļūda vairs neparādās.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pabeigt The Oculus heroic režīmā un atvērt Cache of Eregos (191349). Lādē jābūt 1–2 dažādiem priekšmetiem no astoņu atjaunoto reward saraksta; papildus jāsaglabā Design: Bracing Earthsiege Diamond un trīs Stone Keeper's Shard rindas paredzētā darbība.

## Pakete 56 — Legion milling quest nosacījumu piesaiste herb loot tabulām

Fails: `sql/updates/world/2026_09_18_50_fix_legion_milling_conditions.sql`.

Sešiem quest-only Inscription atradumiem conditions tabulā `SourceGroup` kļūdaini bija 0, tādēļ serveris tos ignorēja kā neesošu `milling_loot_template` grupu. Katrs atraduma item jau eksistēja tieši vienas Legion herb milling tabulas saturā, tāpēc atjaunotas nepārprotamas saites: Aethril 124101→136909, Dreamleaf 124102→136912, Foxflower 124103→136915, Fjarnskaggl 124104→136916, Starlight Rose 124105→136917 un Felwort 124106→136918.

Visas sešas sākotnējās condition rindas saglabātas `_backup_20260918_legion_milling_conditions`; nekas nav dzēsts. Quest ID, nosacījuma tips un esošās milling drop iespējas nav mainītas.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; visas sešas rindas tagad norāda uz atbilstošajām 124101–124106 milling grupām, un backup tabulā ir sešas sākotnējās rindas.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 416 līdz 410; visi seši `milling_loot_template` SourceGroup 0 brīdinājumi pazuda.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Inscription tēlu, kuram attiecīgie profession questi vēl nav pabeigti, atsevišķi millot Aethril, Dreamleaf, Foxflower, Fjarnskaggl, Starlight Rose un Felwort. Katram herbam jāspēj dot tikai savu quest atradumu un jāaktivizē pareizais quests: 39942, 40062, 40064, 40065, 39951 vai 39952.
- Pēc katra attiecīgā questa pabeigšanas atkārtot milling un pārbaudīt, ka quest-only atradums vairs netiek piedāvāts; parastajam pigmentu loot jāturpina darboties.

## Pakete 57 — Pandaria Jewelcrafting research recepšu atjaunošana

Fails: `sql/updates/world/2026_09_18_51_restore_pandaria_jewelcrafting_discoveries.sql`.

Seši krāsu research spelli — River's Heart (131593), Primordial Ruby (131686), Wild Jade (131688), Vermilion Onyx (131690), Imperial Amethyst (131691) un Sun's Radiance (131695) — klienta datos ir explicit discovery darbības, bet `skill_discovery_template` nebija nevienas to receptes. No uzturētās 5.4.8 world DB atjaunots pilnais 68 superior Pandaria gem cut recepšu sadalījums pa krāsām.

Secrets of the Stone (131759) ir bez-cooldown universālais variants, kura spēles apraksts un profession dokumentācija paredz nejaušu recepti no visām krāsām. Tam izveidota visu sešu atjaunoto kopu apvienotā discovery grupa. SQL tikai pievieno trūkstošos ierakstus ar `INSERT IGNORE`; esošie discovery dati netiek pārrakstīti vai dzēsti.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`. Krāsu grupās ir attiecīgi 4, 5, 18, 22, 14 un 5 receptes, bet Secrets of the Stone grupā — visas 68 unikālās receptes.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 410 līdz 403; visu septiņu Jewelcrafting spellu discovery brīdinājumi pazuda.
- Palika pieci atsevišķi terminālu Blacksmithing/Alchemy spellu false-positive kandidāti; tie apzināti nav maskēti ar neīstām discovery rindām un tiks analizēti atsevišķi.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Pandaria Jewelcrafting tēlu, kurš vēl nezina visas receptes, vienu reizi izmantot katru no sešiem krāsu research spelliem. Jāizveido Facets of Research un jāiemācās viena vēl nezināma attiecīgās krāsas superior gem cut recepte; ja konkrētās krāsas receptes jau zināmas, jāpārbauda spēles paredzētā pāreja uz citas krāsas recepti.
- Izmantot Secrets of the Stone (131759) ar 3 Spirit of Harmony. Tam jāiemāca viena vēl nezināma recepte no kopējā 68 recepšu saraksta un nav jāuzliek sešu krāsu research dienas cooldown.
- Kad visas 68 receptes ir zināmas, research nedrīkst atkārtoti iemācīt jau zināmu spellu vai radīt servera kļūdu.

## Pakete 58 — terminālo profession recepšu false-positive discovery klasifikācija

Fails: `src/server/game/Spells/SpellInfo.cpp`.

`IsExplicitDiscovery()` līdz šim jebkuru tradeskill ar “create item + script effect” uzskatīja par recipe discovery. Tas pareizi atpazīst research spellus, bet kļūdaini klasificēja piecus spellus, kuriem `skill_discovery_template` saturs nav paredzēts:

- Wicked Edge of the Planes, Reborn (138880), Bloodmoon, Reborn (138881), Blazefury, Reborn (138892) un Lionheart Executioner, Reborn (138893) ir savu Blacksmithing upgrade ķēžu gala receptes; atšķirībā no iepriekšējiem ķēdes posmiem to aprakstos nav nākamās apgūstamās receptes.
- Alchemist's Cauldron (156586) izveido reagentu konteineru un tā script effect nav recipe discovery.

Šie pieci zināmie izņēmumi tagad atgriež `false` pirms formas pārbaudes. Tas nemaina item izveidi vai citu script-effect apstrādi: iepriekš discovery lookup tāpat neatrada nevienu rindu un turpināja izpildi; tagad tas vienkārši neveic nepiemērojamo lookup un neizvada maldinošu DB kļūdu.

### Pārbaudes rezultāts

- Pilns Release rebuild un install pabeigts sekmīgi; tika pārkompilēts `SpellInfo.cpp` un jaunais `worldserver.exe` uzstādīts `compiles` mapē.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 403 līdz 398; neviens no pieciem nepiemērojamajiem explicit-discovery brīdinājumiem vairs neparādās.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Izgatavot visus četrus terminālos “Reborn” ieročus un pārbaudīt, ka pareizais ierocis tiek izveidots, netiek mēģināts iemācīt neesošu nākamo recepti un iepriekšējie upgrade ķēdes posmi joprojām iemāca nākamo recepti.
- Izmantot Alchemist's Cauldron (156586): jāizveido paredzētais reagentu konteiners 111403, jānostrādā kopīgajam dienas cooldown, un spēlētājam nav jāmācās nejauša recepte.

## Pakete 59 — spell 213704 implicit-target condition mask

Fails: `sql/updates/world/2026_09_18_52_fix_spell_213704_condition_mask.sql`.

Spell 213704 klienta datos ir tikai viens efekts ar indeksu 0. Tā `CONDITION_SOURCE_TYPE_SPELL_IMPLICIT_TARGET` rinda nosacījumu piesaistīja neeksistējošai effect mask 0, tāpēc serveris rindu ignorēja. `SourceGroup` izlabots uz masku 1 (0. efekta bits), saglabājot esošo mērķa prasību — cast drīkst atlasīt tikai creature 107633.

Sākotnējā condition rinda pirms izmaiņas saglabāta `_backup_20260918_spell_213704_condition`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; condition rindai tagad ir `SourceGroup=1`, bet backup tabulā ir viena sākotnējā rinda.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 398 līdz 397, un spell 213704 nepareizās maskas kļūda vairs neparādās.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Atrast saturu, kas izmanto spell 213704, un izpildīt to pie creature 107633: spell 0. efektam jāizvēlas paredzētais mērķis.
- Atkārtot cast situācijā ar citu creature entry un pārbaudīt, ka nosacījums to nepieņem un nerada servera kļūdu.

## Pakete 60 — injured matriarch gossip nosacījumu option ID

Fails: `sql/updates/world/2026_09_18_53_fix_injured_matriarch_gossip_conditions.sql`.

Četriem Broken Isles pet-battle matriarhiem — Snowfeather (116131), Bloodgazer (116139), Direbeak (116140) un Sharptalon (116141) — katra creature entry ir arī tās `gossip_menu_id`. Katrā menu eksistē viena “bandage the wounded matriarch” darbība ar option ID 0, bet attiecīgā questa nosacījums kļūdaini norādīja uz neesošu option ID 1. Visām četrām condition rindām `SourceEntry` izlabots no 1 uz 0; questu ID un pārējā gossip darbība nav mainīta.

Visas četras sākotnējās rindas saglabātas `_backup_20260918_injured_matriarch_conditions`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; visām četrām condition rindām tagad ir `SourceEntry=0`, bet backup tabulā ir četras sākotnējās rindas.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 397 līdz 389: pazuda četras missing gossip-option kļūdas un četri to pašu nederīgo avotu grouped-condition brīdinājumi.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīviem questiem 44969, 45020, 44988 un 45019 apmeklēt attiecīgi ievainoto Snowfeather, Bloodgazer, Direbeak un Sharptalon matriarhu. Katram jāparādās vienīgajai pārsiešanas gossip izvēlei, un tās nospiešanai jāvirza pareizais quests.
- Bez attiecīgā questa pārsiešanas izvēle nedrīkst būt redzama; citu falcosaur dialogu un pet-battle darbībai jāpaliek nemainītai.

## Pakete 61 — trīs viena-option quest dialogu nosacījumi

Fails: `sql/updates/world/2026_09_18_54_fix_single_option_gossip_conditions.sql`.

Gossip menu 18944 (Lasan Skyhorn, quest 39387), 19555 (Kira Iresoul/Lulubelle Fizzlebang, quest 41796) un 19576 (Empyrean NPC, quest 42166) katrā DB ir tieši viena quest dialoga izvēle ar option ID 0. To nosacījumi kļūdaini norādīja uz neesošu option ID 1, tāpēc serveris tos ignorēja. Trīs condition rindām `SourceEntry` izlabots uz 0; dialogu teksts, quest ID un darbība nav mainīti.

Visas trīs sākotnējās rindas saglabātas `_backup_20260918_single_option_gossip_conditions`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; visām trim condition rindām tagad ir `SourceEntry=0`, bet backup tabulā ir trīs sākotnējās rindas.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 389 līdz 383: pazuda trīs missing gossip-option kļūdas un trīs saistītie grouped-condition brīdinājumi.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 39387 runāt ar Lasan Skyhorn: “Yes!” izvēlei jābūt redzamai un jāvirza questa dialogs; bez questa tai jābūt paslēptai.
- Atkārtot atbilstošo dialogu ar Kira Iresoul vai Lulubelle Fizzlebang questā 41796 un ar Empyrean NPC questā 42166. Vienīgajai quest izvēlei jāparādās tikai aktīva questa laikā un jāturpina paredzētā ķēde.

## Pakete 62 — Lunar Festival elder dialogu game-event ID

Fails: `sql/updates/world/2026_09_18_55_fix_lunar_festival_gossip_event.sql`.

Gossip menu 21072 septiņi jautājumi par Lunar Festival elderu atrašanās vietām izmanto `CONDITION_ACTIVE_EVENT`. Nosacījumos bija arī vērtība 327 — tas ir klienta `Holiday.db2` Lunar Festival ID, nevis servera `game_event.eventEntry`, ko šis condition tips sagaida. Katram no septiņiem option jau eksistē atsevišķs, pareizs condition ar event entry 7 (“Lunar Festival”), tādēļ nederīgās 327 rindas bija dublikāti un pēc backup tiek izņemtas no aktīvās tabulas.

Visas septiņas nepareizās rindas saglabātas `_backup_20260918_lunar_festival_gossip_event` pirms izņemšanas. Pareizie event 7 nosacījumi un visi gossip option paliek neskarti; backup ļauj pilnībā atjaunot arhivētos dublikātus.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja koriģēto failu `legion_world`; septiņas nederīgās 327 rindas ir backupā un aktīvajā tabulā katrai izvēlei palicis viens pareizais event 7 nosacījums.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 383 līdz 376; visi septiņi “non existing event id (327)” brīdinājumi pazuda, un updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Kamēr Lunar Festival game event 7 ir aktīvs, atvērt gossip menu 21072 un pārbaudīt, ka redzami visi septiņi jautājumi par elderiem un katrs atver savu pareizo norāžu dialogu.
- Kad event 7 nav aktīvs, jautājumiem par elderiem jābūt paslēptiem; pārējām menu izvēlēm (“I'm ready” un transports uz Exodar) jāpaliek neatkarīgām no festivāla nosacījuma.

## Pakete 63 — Borrowed Time pareizais quest objective NPC

Fails: `sql/updates/world/2026_09_18_56_fix_borrowed_time_objective.sql`.

Quest 41784 “Borrowed Time” uzdevums ir iedot Shimmering Elixir of Suspension Shinfel Blightsworn. Šim nolūkam DB jau ir Shinfel entry 104820 gossip menu 19438 ar izvēli “Give Shinfel a slowing elixir” un condition, kas pārbauda objective 104820. Taču `quest_objectives` kļūdaini saturēja ObjectID 104824 — tas šajā buildā ir Ernest Carlisle, nevis Shinfel. Objective ObjectID izlabots uz Shinfel 104820, nemainot objective ID, skaitu vai flagus.

Sākotnējā objective rinda saglabāta `_backup_20260918_borrowed_time_objective`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; quest 41784 objective tagad norāda uz 104820, un sākotnējā 104824 rinda ir backup tabulā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 376 līdz 375; quest 41784/objective 104820 kļūda vairs neparādās, un updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Warlock pieņemt quest 41784 “Borrowed Time”, iegūt Shimmering Elixir of Suspension un runāt ar Shinfel Blightsworn (entry 104820) Dreadscar Rift.
- Kamēr objective nav izpildīts, jāparādās eliksīra nodošanas izvēlei. To nospiežot, objective jākļūst izpildītam un dialoga izvēlei jāpazūd; Ernest Carlisle (104824) vairs nedrīkst būt saistīts ar šo objective.

## Pakete 64 — Chilled to the Core Cedric objective condition

Fails: `sql/updates/world/2026_09_18_57_fix_chilled_to_core_gossip_objective.sql`.

Quest 45846 “Chilled to the Core” menu 7593 pieder Archmage Cedric entry 18165, kurš šajā 7.3.5 world DB ir reāli spawnots pie Dalaran Crater. Questa creature objective arī ir 18165. Menu nosacījums savukārt bija pārnests no cita patch varianta un pārbaudīja Cedric entry 125016, kuram šajā world DB nav ne `creature_template`, ne spawna. Condition `ConditionValue2` izlabots uz esošo questa objective 18165; menu teksts, quests un objective dati nav mainīti.

Sākotnējā condition rinda saglabāta `_backup_20260918_chilled_to_core_condition`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; menu condition tagad pārbauda objective 18165, un sākotnējā 125016 rinda ir backup tabulā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 375 līdz 374; quest 45846/objective 125016 kļūda vairs neparādās, un updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Mage pieņemt quest 45846 “Chilled to the Core” un pie Dalaran Crater runāt ar Archmage Cedric (entry 18165).
- Kamēr Cedric objective nav izpildīts, jāparādās dialogam par Antonidas disc fragmentu; pēc izvēles objective jākreditējas un šai izvēlei jāpazūd, ļaujot turpināt Ok'rok Icetouch posmu.

## Pakete 65 — When All Is Aligned Ka'alu objective

Fails: `sql/updates/world/2026_09_18_58_fix_when_all_aligned_kaalu_objective.sql`.

Quest 35704 “When All Is Aligned” paredz lidot uz Ka'alu un papildus 80 Adherent pretiniekiem saņemt Ka'alu noslēguma kredītu. World DB reāli spawnots Ka'alu ir entry 77857 phase 3474, un tā SmartAI pēc vehicle dialoga izsauc `SMART_ACTION_CALL_KILLEDMONSTER` ar credit 77857. `quest_objectives` savukārt kļūdaini gaidīja entry 82651, kuram šajā DB nav ne spawna, ne AI. Objective ObjectID izlabots uz skripta faktiski piešķirto 77857; ar to sakrīt arī esošā phase condition.

Sākotnējā objective rinda saglabāta `_backup_20260918_when_all_aligned_objective`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; quest 35704 objective tagad gaida SmartAI piešķirto credit 77857, un sākotnējā 82651 rinda ir backup tabulā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 374 līdz 373; quest 35704/objective 77857 kļūda vairs neparādās, un updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pieņemt quest 35704 “When All Is Aligned”, uzkāpt uz Ka'alu (entry 77857) un ar vehicle spēju nogalināt 80 Adherent pretiniekus.
- Pabeidzot vehicle secību vai izkāpjot paredzētajā brīdī, jāsaņem Ka'alu 77857 objective kredīts, jāaktivizē pareizā phase 3474 pāreja un questam jākļūst nododamam.

## Pakete 66 — quest 42053 Yvelyn phase pārejas objective

Fails: `sql/updates/world/2026_09_18_59_fix_yvelyn_phase_objective.sql`.

Drūda class-hall zonas 8180 phase 7543 ir paredzēta quest 42053 laikā, kamēr Yvelyn objective 107166 nav pabeigts; blakus phase 7544 ir tās pēctecis pēc objective pabeigšanas. Pēcteces condition kļūdaini pārbaudīja Glimmer of Aessina entry 112549, kas atrodas citā mapē/zonā un quest 42053 objective sarakstā neeksistē. `ConditionValue2` izlabots uz to pašu Yvelyn objective 107166, saglabājot pretējo `NegativeCondition` un pārējos phase nosacījumus.

Sākotnējā condition rinda saglabāta `_backup_20260918_yvelyn_phase_condition`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; phase 7544 condition tagad pārbauda objective 107166, un sākotnējā 112549 rinda ir backup tabulā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 373 līdz 372; quest 42053/objective 112549 kļūda vairs neparādās, un updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Druid pieņemt quest 42053 un zonā 8180 pārbaudīt phase 7543: pirms sarunas/darbības ar Yvelyn (107166) jābūt redzamai questa sākuma fāzei un gossip izvēlei “Can you help me?”.
- Pabeidzot Yvelyn objective, pasaulei jāpārslēdzas uz phase 7544 bez relog; pēc questa pabeigšanas vai nodošanas fāzei jāsaglabājas atbilstoši esošajiem complete/reward nosacījumiem.

## Pakete 67 — Ooka Dooker neesošā gossip option dublikāts

Fails: `sql/updates/world/2026_09_18_60_archive_duplicate_ooka_gossip_condition.sql`.

Ooka Dooker menu 17264 satur quest food dialogu option 1 un vendor dialogu option 2. Quest 37536 nosacījums pareizi eksistē option 1, taču DB bija arī identiska condition rinda neesošam option 0. Tā nevarēja ietekmēt nevienu dialogu un tikai dublēja reālās izvēles nosacījumu, tāpēc nederīgā option 0 rinda pēc backup izņemta no aktīvās tabulas. Abi faktiskie gossip option un option 1 pareizais quest nosacījums paliek neskarti.

Sākotnējā dublikāta rinda saglabāta `_backup_20260918_ooka_duplicate_condition`, no kurienes to var pilnībā atjaunot.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; neesošās option 0 dublikāts ir backupā, bet option 1 nosacījums un abi reālie menu option paliek aktīvi.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 372 līdz 370: pazuda missing gossip-option kļūda un tās grouped-condition brīdinājums; updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 37536 runāt ar Ooka Dooker (90086): option 1 par fighting food jābūt redzamai un jāvirza quests, bet vendor option 2 joprojām jāatver preču logs.
- Bez quest 37536 food dialogam jābūt paslēptam, bet vendor izvēlei jāpaliek pieejamai; tukša vai dubulta option 0 nedrīkst parādīties.

## Pakete 68 — orphan gossip menu 737 condition arhivēšana

Fails: `sql/updates/world/2026_09_18_61_archive_orphan_gossip_737_condition.sql`.

Condition tabulā bija quest 26703 nosacījums gossip menu 737 option 0, taču šajā world DB menu 737 neeksistē ne `gossip_menu`, ne `gossip_menu_option`, un nevienam creature tas nav piešķirts kā `gossip_menu_id`. Tātad rinda ir no izņemtas/vecākas menu implementācijas un tai nav iespējama runtime patērētāja. Ar backup arhivēta tikai nepieejamā condition rinda; quest 26703 un tā pārējie dati netiek mainīti.

Sākotnējā rinda saglabāta `_backup_20260918_orphan_gossip_737_condition`, no kurienes to var pilnībā atjaunot, ja menu kādreiz tiek restaurēts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; orphan condition ir backupā, bet quest 26703 un visi citi tā dati paliek neskarti.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 370 līdz 368: pazuda menu 737 missing-option kļūda un tās grouped-condition brīdinājums; updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Quest 26703 iziet pa tā pašreizējo spēles ceļu un pārliecināties, ka tā pieņemšana, mērķi un nodošana darbojas bez menu 737; spēlē nedrīkst parādīties tukšs dialogs.
- Ja nākotnē tiek atjaunots gossip menu 737, vispirms atjaunot arī backup condition un pārbaudīt tā option ID pret jaunā menu saturu.

## Pakete 69 — Randall Goldsprocket neesošā option dublikāts

Fails: `sql/updates/world/2026_09_18_62_archive_duplicate_randall_gossip_condition.sql`.

Randall Goldsprocket menu 20426 satur vienu dialogu option 0 “And what did he take with him?”. Tam jau ir abi vajadzīgie nosacījumi: aktīvs quest 45413 un nepabeigts “Stolen Item Identified” credit 116890. Neesošajai option 1 bija vēl viens `quest taken` nosacījums, kas tikai dublēja option 0 pirmo pārbaudi un nevarēja tikt izmantots. Šī viena nederīgā rinda pēc backup izņemta; option 0, abi tā nosacījumi un Randall SmartAI paliek neskarti.

Sākotnējā dublikāta rinda saglabāta `_backup_20260918_randall_duplicate_condition`, no kurienes to var atjaunot.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; neesošās option 1 dublikāts ir backupā, bet option 0 un abi tā nosacījumi paliek aktīvi.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 368 līdz 366: pazuda menu 20426 missing-option kļūda un tās grouped-condition brīdinājums; updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 45413 un vēl nesaņemtu credit 116890 runāt ar Randall Goldsprocket (113812): jāparādās vienīgajai izvēlei “And what did he take with him?”.
- Izvēlei jāpalaiž Randall SmartAI action list un jāpiešķir paredzētais credit; pēc credit saņemšanas dialogam jāpazūd. Neaktīva questa laikā dialogam nav jābūt redzamam.

## Pakete 70 — Inkrot un Harold Winston gossip SourceGroup

Fails: `sql/updates/world/2026_09_18_63_fix_creature_entry_gossip_groups.sql`.

Trīs gossip conditions rindās `SourceGroup` kļūdaini saturēja creature entry, nevis menu ID. Inkrot entry 97756 izmanto menu 18883 ar option 0 “I'm here for your head, Inkrot!”, bet Harold Winston entry 100671 izmanto menu 19103 ar option 0 “I'm here to reclaim the jewels you stole.”. Divi Inkrot questu 39949/40539 nosacījumi pārcelti uz SourceGroup 18883, bet Harold quest 40531 nosacījums — uz 19103. Option ID, quest ID, ElseGroup un SmartAI nav mainīti.

Visas trīs sākotnējās rindas saglabātas `_backup_20260918_creature_entry_gossip_groups`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; divi Inkrot nosacījumi tagad piesaistīti menu 18883 un Harold nosacījums — menu 19103, bet trīs sākotnējās rindas ir backupā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 366 līdz 360: pazuda trīs missing-option kļūdas un trīs saistītie grouped-condition brīdinājumi; updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 39949 vai 40539 runāt ar Inkrot (97756): option “I'm here for your head, Inkrot!” jāparādās, jāpalaiž menu 18883 SmartAI un jāsāk paredzētā cīņa. Bez abiem questiem tai jābūt paslēptai.
- Ar aktīvu quest 40531 runāt ar Harold Winston (100671): dārgakmeņu atgūšanas option jāparādās, jāpalaiž menu 19103 SmartAI un jāsāk paredzētā cīņa; bez questa izvēlei jābūt paslēptai.

## Pakete 71 — Grom'kar Grimshot orphan gossip condition

Fails: `sql/updates/world/2026_09_18_64_archive_grimshot_orphan_gossip_condition.sql`.

SourceGroup 88879 nav gossip menu — tas ir WoD Grom'kar Grimshot creature entry. Šim NPC ir `npcflag=0`, `gossip_menu_id=0`, nav gossip SmartAI, un DB neeksistē ne menu, ne option ar ID 88879. Vienīgā condition rinda turklāt pārbaudīja nesaistītu vecā satura quest 11221. Tā nevarēja būt runtime sasniedzama, tāpēc pēc backup izņemta no aktīvās conditions tabulas; Grom'kar NPC, quests 11221 un visi to pārējie dati paliek neskarti.

Sākotnējā rinda saglabāta `_backup_20260918_grimshot_orphan_gossip`, no kurienes to var atjaunot, ja tiek restaurēta trūkstoša implementācija ar pierādāmu menu ID.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; orphan condition ir backupā, bet NPC 88879 un quest 11221 dati nav mainīti.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 360 līdz 358: pazuda pēdējā missing gossip-option kļūda un tās grouped-condition brīdinājums; `addToGossipMenuItems` kļūdu vairs nav.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Draenor zonā ar Grom'kar Grimshot (88879) pārbaudīt parasto combat uzvedību un loot; NPC nedrīkst atvērt tukšu gossip logu.
- Quest 11221 pārbaudīt tā faktiskajā zonā/ķēdē; tā pieņemšana un pabeigšana nedrīkst būt atkarīga no WoD Grom'kar Grimshot.

## Pakete 72 — PhaseMgr map un equipped-item condition atbalsts

Faili: `src/server/game/Maps/PhaseMgr.cpp` un `src/server/game/Entities/Player/Player.cpp`.

Phase definition datos ir divas derīgas `CONDITION_MAPID` pārbaudes Shadowmoon Valley map 1158 fāzēm un trīs `CONDITION_ITEM_EQUIPPED` pārbaudes quest 42429 phase 7435 ar item 128862. Vispārējais ConditionMgr abus tipus prot novērtēt, taču PhaseMgr atļauto tipu saraksts tos noraidīja, tāpēc serveris šīs fāžu prasības ignorēja.

PhaseMgr tagad pieņem abus condition tipus. Map nosacījums tiek pārrēķināts jau esošajā zonas/mapes ieiešanas pilnajā phase pārrēķinā; item nosacījumam `_ApplyItemMods` pēc equip vai unequip nosūta mērķētu `CONDITION_ITEM_EQUIPPED` izmaiņas paziņojumu. DB rindas netiek mainītas vai dzēstas.

### Pārbaudes rezultāts

- Pilns Release rebuild un install pabeigts sekmīgi; tika pārkompilēti `Player.cpp` un `PhaseMgr.cpp`, un jaunais `worldserver.exe` uzstādīts `compiles` mapē.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 358 līdz 353; visas piecas “PHASE_DEFINITION does not support condition type 3/22” kļūdas pazuda.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Shadowmoon Valley zonā 7078 uz mapes 1158 iziet quest 34646 pāreju: quest complete/reward stāvokļiem jāpārslēdz phase 3666 un 3464, un šīs fāzes nedrīkst aktivizēties citā mapē.
- Ar aktīvu, pabeigtu un nodotu quest 42429 katrā no trim stāvokļiem uzvilkt item 128862: phase 7435 jāaktivizējas uzreiz bez relog. Novelkt item — phase jānoņemas uzreiz; citi equipment un phase stāvokļi nedrīkst mainīties.

## Pakete 73 — quest 42537 objective condition lauku secība

Fails: `sql/updates/world/2026_09_18_65_fix_defending_broken_isles_objective_conditions.sql`.

Trīs `CONDITION_QUEST_OBJECTIVE_DONE` rindas kļūdaini glabāja objective ObjectID `108260` vai `108261` laukā `ConditionValue1`, ko kodols interpretē kā QuestID. Abi ir īsti quest 42537 mērķi (`quest_objectives` ID 284571 un 284572). Rindām tagad `ConditionValue1=42537`, bet attiecīgais ObjectID pārcelts uz `ConditionValue2`; pozitīvo un negatīvo nosacījumu nozīme nav mainīta.

Visas trīs sākotnējās rindas saglabātas `_backup_20260918_defending_broken_isles_objectives`; nekas nav dzēsts.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; DB pārbaudē visām trim rindām ir pareizais quest/objective pāris.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 353 līdz 350; visas `108260/108261` kā neesoša questa kļūdas pazuda, un jaunu updatera kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 42537 secīgi izpildīt mērķus 108260 un 108261: ar scene 1 saistītajām pārejām jāaktivizējas tikai pie paredzētās pirmā/otrā mērķa izpildes kombinācijas.
- Atkārtot pārbaudi pirms abu mērķu izpildes un pēc questa pabeigšanas; scene nedrīkst sākties priekšlaicīgi vai iestrēgt pēc otrā mērķa.

## Pakete 74 — nederīgie quest 45406 invasion phase sentinel nosacījumi

Fails: `sql/updates/world/2026_09_18_66_archive_invalid_invasion_phase_objectives.sql`.

Zone 7541 phase 142 un 143 katrai bija viena `CONDITION_QUEST_OBJECTIVE_DONE` rinda ar `QuestID=-45406` un `ObjectID=-1`. Šis condition tips pieņem unsigned QuestID un īstu objective ObjectID; jau sākotnējā 2020 DB dumpā esošo negatīvo sentinel formu kodols nekad nav atbalstījis, tādēļ abas rindas tika noraidītas startā un runtime nebija sasniedzamas.

Derīgā trīs posmu loģika paliek neskarta: phase 4000 izmanto quest 45406 objective 116868, phase 4001 pārbauda vēl neizpildītu 118566, bet phase 4002 — izpildītu 118566 vai nodotu quest. Abas nederīgās rindas saglabātas `_backup_20260918_invalid_invasion_phase_objectives` un tikai tad izņemtas no aktīvās tabulas.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; backupā ir tieši divas rindas, bet piecas derīgās phase 142–144 condition rindas paliek aktīvas.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 350 līdz 348; abas unsigned `4294921890` objective kļūdas pazuda, jaunu kļūdu nebija.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Zone 7541 iziet quest 45406: pēc objective 116868 jāaktivizējas phase 4000, pirms objective 118566 pabeigšanas jābūt phase 4001, bet pēc tā pabeigšanas — phase 4002.
- Nodot quest 45406 un atkārtoti ieiet zonā; jāpaliek pēdējam paredzētajam phase stāvoklim un nedrīkst parādīties iepriekšējie invasion posmi.

## Pakete 75 — Missing Vrykul Champion sarunas quest nosacījums

Fails: `sql/updates/world/2026_09_18_67_fix_missing_vrykul_champion_condition.sql`.

Creature credit 96255 (`Credit - Vrykul Champion Missing`) ir quest 39590 “Ahead of the Game” objective, bet linked SmartAI sarunas condition kļūdaini atsaucās uz quest 39595 “Blood and Gold”, kuram ir tikai item 128511 mērķis. Arī condition tips neatbilda rindas komentāram: `OBJECTIVE_DONE` ar negatīvu rezultātu pēc SmartAI credit piešķiršanas sarunu nobloķētu, lai gan komentārs prasa aktīvu questu.

Condition tagad ir `QUEST_TAKEN` questam 39590. SmartAI event 0 turpina piešķirt credit 96255, un linked event 1 palaiž conversation 215006 tikai spēlētājam ar aktīvu pareizo questu. Sākotnējā rinda saglabāta `_backup_20260918_missing_vrykul_champion_condition`.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; aktīvā condition rinda ir `type=9`, `quest=39590`, un backupā ir viena oriģinālā rinda.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 348 līdz 347; pēdējā neesoša quest objective kļūda pazuda, un `objective_done` validācijas kļūdu vairs nav.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pieņemt quest 39590 “Ahead of the Game” un pieiet credit triggerim 96255 Valley of the Sword: jāsaņem trešais champion credit un vienreiz jāpalaižas conversation 215006.
- Bez aktīva quest 39590 pieiet tam pašam triggerim: credit un saruna nedrīkst ietekmēt spēlētāju. Quest 39595 “Blood and Gold” item 128511 vākšanai jāpaliek neatkarīgai.

## Pakete 76 — phase definition condition SourceId

Fails: `sql/updates/world/2026_09_18_68_fix_phase_condition_source_ids.sql`.

Trīs derīgām `PHASE_DEFINITION` condition rindām bija `SourceId=1`, lai gan šis papildu identifikators paredzēts tikai SmartEvent avotam un phase avotam obligāti jābūt `0`. Divas rindas attiecas uz Frostfire Ridge zone 6720 phase entry 4 (scene 594 vai reward quest 34402), viena — uz zone 7814 phase entry 3 (kamēr nav quest 38689). Zone, entry, condition tips un to vērtības netika mainītas.

Visas trīs sākotnējās rindas saglabātas `_backup_20260918_phase_condition_source_ids`, pēc tam tikai `SourceId` nomainīts no 1 uz 0.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; abas phase definitions atrastas un visas trīs condition rindas tagad tiek ielādētas ar `SourceId=0`.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 347 līdz 344; visas trīs “Condition type 23 has not allowed value of SourceId” kļūdas pazuda.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Frostfire Ridge zone 6720 pabeigt scene 594 un atsevišķā mēģinājumā nodot quest 34402; abos alternatīvajos gadījumos jāaktivizējas phase 3331.
- Zone 7814 pārbaudīt phase 5495/5494 pirms un pēc quest 38689 pieņemšanas/pabeigšanas; phase entry 3 jābūt aktīvam tikai paredzētajā “quest nav” stāvoklī.

## Pakete 77 — novecojušais Toss Crystals spell DB-script

Fails: `sql/updates/world/2026_09_18_69_archive_obsolete_toss_crystals_dbscript.sql`.

`spell_scripts` saturēja septiņas identiskas rindas spell 179915 “Toss Crystals” effect 0, kas mēģināja izpildīt creature 90315 summon komandu. Legion 7.3.5.26972 SpellEffect datos effect 0 ir `REMOVE_AURA`, bet effect 1 ir `APPLY_AURA` ar periodic trigger; spellam nav `SCRIPT_EFFECT` vai `DUMMY` effect, kas varētu izsaukt DB-script. Tāpēc kodols visas septiņas rindas ignorēja kā no vecāka patcha palikušu, neizsaucamu implementāciju.

Visas septiņas rindas saglabātas `_backup_20260918_toss_crystals_spell_scripts`, pēc tam izņemtas no aktīvās tabulas. Quest 37853 objective 90315, creature credit un creature 89975 SmartAI hit-credit paliek neskarti.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; backupā ir septiņas rindas un aktīvajā `spell_scripts` spellam 179915 vairs nav neizsaucamu rindu.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 344 līdz 343; effect-type kļūda spellam 179915 pazuda un jauna kļūda neradās.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 37853 “Tossing Crystals” izmantot paredzēto kristālu pie Senegos baseina: spell 179915 jānoņem/iedarbina paredzētās auras un quest credit 90315 jāsaņem pa pašreizējo spell/SmartAI ceļu.
- Atkārtot darbību pēc credit saņemšanas un bez aktīva questa; nedrīkst parādīties lieki credit NPC vai vairākkārtējs progress.

## Pakete 78 — event script runas BroadcastText validācija

Fails: `src/server/game/Globals/ScriptsData.cpp` (un novecojušā `db_script_string` ielādētāja deklarācijas/izsaukumu noņemšana).

Event script 13021 izsauc spell 28700 “Disperse Neutralizing Agent” un liek mērķa būtnei parādīt BroadcastText 17912. Izpildes ceļš `WorldObject::Talk(uint32)` jau interpretē `dataint` kā BroadcastText ID, un Legion hotfix datubāzē ID 17912 ir pareizais teksts par neutralizējošā līdzekļa sajaukšanos ezera ūdenī. Tomēr vecā ielādes validācija vēl pieprasīja vēsturiskā `db_script_string` rezervēto diapazonu, tāpēc derīgais ieraksts tika noraidīts.

Validācija tagad, tāpat kā aktuālajā TrinityCore realizācijā, pārbauda `sBroadcastTextStore`. Tukšās un ar runtime semantiku vairs nesaderīgās `db_script_string` ielādes funkcijas starta un `reload all scripts` izsaukumi noņemti; pats event script un tā dati nav mainīti vai dzēsti.

### Pārbaudes rezultāts

- `Release` konfigurācijas pilna kompilācija un instalēšana pabeigta sekmīgi; `worldserver.exe` atjaunināts.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 343 līdz 341: pazuda gan 13021 ārpus diapazona kļūda, gan novecojušā `db_script_string` ielādētāja kļūda; jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116, tātad šī pakete nepievienoja jaunu servera kļūdu. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pie Irradiated Power Crystal (GO 181433) Silverline Lake izmantot spell 28700 “Disperse Neutralizing Agent”; mērķa būtnei apkārtnē jāparāda BroadcastText 17912 par ūdens attīrīšanos.
- Pārbaudīt, ka ziņojumu redz tuvumā esošie spēlētāji paredzētajā `TEXT_EMOTE` formā un ka spell ārpus atļautā 15 jardu attāluma joprojām nav izmantojams.

## Pakete 79 — Cache of Tsulong nederīgā loot atsauce

Fails: `sql/updates/world/2026_09_18_70_archive_orphan_tsulong_loot_reference.sql`.

`Cache of Tsulong` (GO 212922, loot entry 43568) saturēja 1% atsauci uz neesošu `reference_loot_template` entry 86279. Tā nav pārvietošanas atjauninājuma radīta kļūda: nederīgā `-86279` rinda jau ir LegionCore 2024 bāzes dumpā, savukārt ne 2024, ne 2020 dumpā atsauces saturs nav atrodams. Arī pārbaudītajos Legion/Pandaria forkos atjaunojams references saturs netika atrasts.

Lādes 93 derīgās tiešās loot rindas visām grūtības pakāpēm, tostarp item 86279, netika mainītas. Vienīgā neatrisināmā un izpildes laikā neko dot nespējīgā references rinda saglabāta `_backup_20260918_orphan_tsulong_loot_reference`, pēc tam izņemta no aktīvās tabulas.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; backup tabulā ir viena precīza sākotnējā rinda, aktīvajā loot tabulā references 86279 vairs nav.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 341 līdz 340; trūkstošās `reference_loot_template` entry 86279 kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Terrace of Endless Spring nogalināt Tsulong un atvērt Cache of Tsulong (GO 212922) katrā pieejamajā raid grūtības pakāpē; lādei jāatveras un jādod attiecīgās grūtības tiešais loot.
- Īpaši pārbaudīt, ka joprojām var izkrist item 86279 “Pattern: Liferuned Leather Gloves” un ka heroic/raid-finder loot mode rindas nav ietekmētas.

## Pakete 80 — Kadrak Secret Signal Powder item ID

Fails: `sql/updates/world/2026_09_18_71_fix_kadrak_signal_powder_item.sql`.

Kadrak (creature 33837) gossip opcija “Give me new Secret Signal Powder” izpildīja `SMART_ACTION_ADD_ITEM` ar neesošu septiņu ciparu item ID 1212331. Quest 13808 “Mission Improbable” `StartItem` ir 45710 — tieši Secret Signal Powder, kuru šai nomaiņas opcijai jāizsniedz.

Sākotnējā SmartAI rinda saglabāta `_backup_20260918_kadrak_signal_powder_item`; mainīts tikai izsniedzamā item ID uz 45710 un precizēts komentārs. Saistītā gossip aizvēršanas darbība netika mainīta.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; aktīvajā SmartAI rindā ir `action_param1=45710`, backupā saglabāts sākotnējais `1212331` ieraksts.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 340 līdz 339; neesošā item kļūda creature 33837 vairs netiek reģistrēta.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar Horde tēlu, kuram aktīvs quest 13808 “Mission Improbable” un nav Secret Signal Powder, pie Kadrak Splintertree Post izvēlēties nomaiņas dialogu; jāsaņem viens item 45710 un dialogam jāaizveras.
- Izmantot Secret Signal Powder pie Smoldering Brazier Satyrnaar; jāsummonē Krokk un quest ķēdei “Making Stumps”/“Wet Work” jābūt turpināmai.
- Atkārtot dialogu, kamēr unikālais priekšmets jau ir somā; nedrīkst rasties dublikāts vai servera kļūda.

## Pakete 81 — Bonegrim Catriona's Jewel item ID

Fails: `sql/updates/world/2026_09_18_72_fix_bonegrim_jewel_item.sql`.

Bonegrim (creature 97863) quest 40863 maiņas dialogā pareizi pievienoja Fel-Infused Core 133881, bet mēģināja noņemt neesošu item 133885. Gossip condition, quest objective un Legion 7.3.5.26972 klienta `ItemSparse` dati apstiprina, ka vajadzīgais priekšmets ir 133895 “Catriona's Jewel” — liels rubīns, kuru NPC prasa apmaiņā pret core.

Sākotnējā timed action list rinda saglabāta `_backup_20260918_bonegrim_jewel_item`; mainīts tikai noņemamā item ID no 133885 uz 133895 un pievienots paskaidrojošs komentārs.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; Bonegrim action 2 tagad noņem item 133895, un sākotnējā rinda ar 133885 ir backupā.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 339 līdz 338; neesošā item kļūda timed action list 9786301 pazuda.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Quest 40863 laikā iegūt Catriona's Jewel (item 133895), pie Bonegrim izvēlēties “Here is your gem. Give me the core.”; rubīnam jāpazūd un somā jāparādās vienam Fel-Infused Core (133881).
- Bez Catriona's Jewel maiņas opcija nedrīkst būt pieejama; ar jewel somā tai jābūt redzamai un pēc veiksmīgas maiņas atkārtoti jāpaslēpjas.

## Pakete 82 — novecojis Keeper Remulos gossip atzars

Fails: `sql/updates/world/2026_09_18_73_archive_obsolete_remulos_gossip.sql`.

Keeper Remulos SmartAI saturēja saistītu darbību pāri menu 10215 neesošai opcijai 2: NPC izrunātu svētku tekstu un dotu item 90001. Legion 7.3.5 DB menu 10215 ir tikai derīgās opcijas 0 un 1 questiem 13074/13075; 7.3.5.26972 klienta `ItemSparse` nav item 90001, un tas nav neviena Remulos questa starta, reward, drop vai objective priekšmets. 2024 bāzes dumpā šis SmartAI pāris jau ir bez atbilstošas gossip opcijas, kamēr 2020 dumpā tā nav.

Tātad atzaru spēlē nav iespējams izsaukt un tā saturs šajā klienta versijā nav atjaunojams. Abas precīzās rindas (event 4 un linked event 5) saglabātas `_backup_20260918_obsolete_remulos_gossip`, pēc tam izņemtas no aktīvās SmartAI tabulas. Derīgie Remulos Emerald Dream teleporti un quest accept darbība netika mainīti.

### Pārbaudes rezultāts

- SQL updater sekmīgi piemēroja failu `legion_world`; backupā ir abas novecojušās rindas, bet aktīvajā SmartAI paliek eventi 0–3 derīgajām menu opcijām un questam 13074.
- Pilns `worldserver` starts pabeigts 13 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 338 līdz 337; neesošā item 90001 kļūda pazuda.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 13074 pie Keeper Remulos izvēlēties “Please send me into the Emerald Dream…”; jānostrādā spell 57413 un spēlētājs jānosūta paredzētajā quest fāzē.
- Pabeigt quest 13075 nosacījumu un izmantot atgriešanās opciju pie Remulos; jānostrādā spell 57670, dialogam jāaizveras un jānotiek atgriešanai pie Arch Druid Lilliandra.
- Pārliecināties, ka Remulos izvēlnē nav tukšas trešās opcijas un ka viņa pārējie quest 7066, 8446–8447, 8734–8736 un 40962 paliek pieejami paredzētajos stāvokļos.

## Pakete 83 — SmartAI WHILE_CHARMED flag validācija

Fails: `src/server/game/AI/SmartScripts/SmartScriptMgr.h`.

Kodā `SMART_EVENT_FLAG_WHILE_CHARMED` ir korekti deklarēts kā `0x200`, tomēr iepriekšējā SmartAI enum migrācijā tas netika pievienots `SMART_EVENT_FLAGS_ALL`. Tādēļ loaderis divas derīgas rindas ar flag 512 noraidīja kā nezināmas: Fel Reaver (18733) respawn `Set Active` darbību un Wooly Mammoth Bull (25743) nāves `Animal Blood` spell 46221 darbību.

Atļauto flagu maskā tagad iekļauts jau eksistējošais `SMART_EVENT_FLAG_WHILE_CHARMED`. DB rindas un to flagi netika mainīti vai dzēsti.

### Pārbaudes rezultāts

- `Release` konfigurācijas pilna kompilācija un instalēšana pabeigta sekmīgi; `worldserver.exe` atjaunināts.
- Pilns `worldserver` starts pabeigts 35 sekundēs (šajā startā sistēma bija noslogotāka). `DBErrors.log` kļūdu skaits samazinājās no 337 līdz 335; abas `invalid event flags (512)` kļūdas pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Hellfire Peninsula atrast vai respawnēt Fel Reaver (18733); tam jābūt aktīvam arī bez tuvumā esoša spēlētāja, jābūt warning aura 34623 un dubultā uzbrukuma spell 19818.
- Borean Tundra nogalināt Wooly Mammoth Bull (25743) parasti un situācijā, kur tas ir transporta/charm mijiedarbībā; nāves brīdī uz spēlētāju jānostrādā triggered spell 46221 “Animal Blood”.

## Pakete 84 — Gordok Brute Enrage SmartAI ķēde

Fails: `sql/updates/world/2026_09_18_74_fix_gordok_brute_enrage_link.sql`.

Gordok Brute (11441) 30% veselības Enrage darbība norādīja pati uz sevi (`id=5, link=5`), tāpēc SmartAI loaderis to noraidīja. AzerothCore etalona dati apstiprina paredzēto ķēdi `id 5 -> id 6`: vispirms spell 15716 “Enrage”, pēc tam linked enrage emote. Sākotnējās SmartAI rindas saglabātas `_backup_20260918_gordok_brute_enrage_link`.

Saite izlabota uz `link=6`. Tā kā Legion datubāzē pašas linked darbības komentārs bija saglabāts, bet tai paredzētais `creature_text` trūka, atjaunota atsevišķa teksta grupa 1 ar abiem kanoniskajiem enrage emotiem un linked darbība pārslēgta uz šo grupu.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backup tabulā ir abas sākotnējās SmartAI rindas, aktīvā ķēde ir `5 -> 6`, un teksta grupā 1 ir abi atjaunotie emoti.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 335 līdz 334; Gordok Brute pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Dire Maul North atrast Gordok Brute (11441), sākt cīņu un samazināt tā veselību zem 30%; vienu reizi jānostrādā spell 15716 “Enrage” un jāparādās vienam no enrage emotiem.
- Ļaut radījumam iziet no cīņas un sākt cīņu atkārtoti; zem 30% ķēdei atkal jānostrādā tikai vienu reizi, bez atkārtotas emote vai SmartAI iestrēgšanas.

## Pakete 85 — Stomper Kreeg Drunken Rage SmartAI saite

Fails: `sql/updates/world/2026_09_18_75_fix_stomper_kreeg_drunken_rage_link.sql`.

Stomper Kreeg (14322) 15% veselības spell 22835 “Drunken Rage” darbībai bija pašsaite `id=8, link=8`. Tai uzreiz seko paredzētā linked darbība `id=9`, tādēļ saite izlabota uz `link=9`. Abas sākotnējās rindas saglabātas `_backup_20260918_stomper_kreeg_rage_link`; pārējais Kreeg SmartAI nav mainīts.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backupā ir abas sākotnējās rindas, bet aktīvā Drunken Rage ķēde ir `8 -> 9`.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 334 līdz 333; Stomper Kreeg pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Dire Maul North cīnīties ar Stomper Kreeg (14322) un samazināt veselību zem 15%; spell 22835 “Drunken Rage” jānostrādā vienu reizi, SmartAI nedrīkst iestrēgt vai atkārtot ķēdi bezgalīgi.
- Pārbaudīt arī 30% HP spell 8269 “Frenzy” un Kreeg parastās spējas Booze Spit, War Stomp un Whirlwind, lai pārliecinātos, ka mazā saites korekcija nav skārusi citas darbības.

## Pakete 86 — Nomi Pickled Stormray apmaiņas SmartAI saite

Fails: `sql/updates/world/2026_09_18_76_fix_nomi_pickled_stormray_link.sql`.

Nomi (101846) gossip menu 19241 opcijai 33 pēc spell 305089 bija paredzēts atņemt 5 Stormray (124110) un pēc tam 3 Flaked Sea Salt (133588), taču pirmā linked darbība norādīja pati uz sevi (`id=18, link=18`). Tā paša piedāvājuma otrā kopija opcijā 41 satur pilnu pareizo ķēdi `45 -> 46 -> 10`, kas droši apstiprina pirmajai kopijai paredzēto `18 -> 19 -> 10`.

Saite izlabota uz `link=19`; sākotnējās darbības 18 un 19 saglabātas `_backup_20260918_nomi_pickled_stormray_link`. Recepšu spelli, priekšmetu daudzumi un pārējās Nomi apmaiņas nav mainītas.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backupā ir abas sākotnējās rindas, un pirmā aktīvā ķēde `18 -> 19 -> 10` tagad precīzi atbilst otrajai `45 -> 46 -> 10`.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 333 līdz 332; Nomi pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Pie Nomi (101846) atvērt menu 19241 un ar vismaz 5 Stormray (124110) un 3 Flaked Sea Salt (133588) izvēlēties “Learn recipes for pickled storm rays.”; jānostrādā spell 305089, jāatņem tieši abi norādītie materiālu daudzumi un jāparādās noslēguma tekstam.
- Atkārtot piedāvājumu no otrās menu lapas/opcijas 41 un salīdzināt rezultātu: abām Pickled Stormray apmaiņas ķēdēm jāuzvedas vienādi, bet citām Nomi recepšu izvēlēm jāpaliek neskartām.

## Pakete 87 — Deathguard Simmer DataTrigger ķēde

Fails: `sql/updates/world/2026_09_18_77_fix_deathguard_simmer_data_chain.sql`.

Deathguard Simmer (1519) īpašā `source_type=13` notikuma trīs secīgās SetData darbības uz DataTriggerDSDeathwing (600034) bija veidotas kā vērtību secība 3, 2, 4. Pirmā rinda korekti veda uz `id=1`, pēdējā beidza ķēdi, bet vidējā linked rinda norādīja pati uz sevi (`id=1, link=1`). Saite izlabota uz vienīgo loģisko turpinājumu `id=2`, izveidojot ķēdi `0 -> 1 -> 2`.

Visas trīs sākotnējās rindas saglabātas `_backup_20260918_deathguard_simmer_data_chain`; darbību tipi, vērtības un mērķis nav mainīti.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backupā ir visas trīs sākotnējās rindas, un aktīvā ķēde DB ir `0 -> 1 -> 2` ar nemainītām vērtībām 3, 2 un 4 uz mērķi 600034.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 332 līdz 331; Deathguard Simmer pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Izsaukt saturu, kurā Deathguard Simmer (1519) `source_type=13` notikums sazinās ar DataTriggerDSDeathwing (600034), un pārbaudīt, ka mērķis saņem SetData vērtības 3, 2 un 4 pareizā secībā bez SmartAI cikla.
- Pārbaudīt ar šo datu trigeri saistīto Deathwing scenārija/fāzes pāreju līdz galam; servera logā nedrīkst parādīties jaunas SmartAI runtime kļūdas.

## Pakete 88 — area trigger 6854 auras darbība

Fails: `sql/updates/world/2026_09_18_78_fix_areatrigger_6854_aura_action.sql`.

SmartTrigger 6854 satur divas neatkarīgas `ON_TRIGGER` darbības uz spēlētāju: quest spell 99424 un auras 99435 noņemšanu. Pirmā darbība neuzsāk linked ķēdi (`id=0, link=0`), bet otrajai kļūdaini bija pašsaite `id=1, link=1`, tādēļ loaderis to noraidīja. Otrā darbība izlabota uz patstāvīgu `link=0`, nemainot spellus, mērķi vai notikuma parametrus.

Abas sākotnējās rindas saglabātas `_backup_20260918_areatrigger_6854_aura_action`. Šis trigeris ir saistīts ar quest 39272 un tā mērķiem 99433–99436.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backupā ir abas sākotnējās rindas, bet abas aktīvās `ON_TRIGGER` darbības tagad ir patstāvīgas ar `link=0`.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 331 līdz 330; trigger 6854 pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar aktīvu quest 39272 ieiet area trigger 6854; jānostrādā quest spell 99424 un spēlētājam jānoņemas aura 99435.
- Pārbaudīt visus četrus quest mērķus 99433–99436 un atkārtotu ieiešanu triggerī; progress nedrīkst dubultoties neparedzēti, un aura 99435 nedrīkst palikt uz spēlētāja.

## Pakete 89 — Kalecgos dublētā SetData darbība

Fails: `sql/updates/world/2026_09_18_79_archive_duplicate_kalecgos_dataset_action.sql`.

Kalecgos (38017) SmartAI saturēja divas vienādas `SetData(0,2)` darbības ar `id=1`, kas abas palaida timed action list 3801701; vienīgā atšķirība bija `link`. Funkcionālā sākotnējā rinda ar `link=0` jau ir aktīva, bet vēlāk pievienotā kopija ar `link=1` sasaistīja pati sevi un loaderī tika noraidīta. 2020 dumpa rindu secība arī parāda, ka pašsaistes kopija pievienota vēlāk par pilno funkcionālo Kalecgos bloku.

Precīzā nederīgā kopija saglabāta `_backup_20260918_duplicate_kalecgos_dataset_action` un arhivēta no aktīvās tabulas. Aktīvā `id=1, link=0` darbība, action list 3801700/3801701 un waypoint ķēdes nav mainītas.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; precīzā dublētā rinda ir backupā, bet aktīvajā tabulā paliek viena funkcionāla `id=1, link=0` darbība uz action list 3801701.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 330 līdz 329; Kalecgos pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Quel'Delar notikuma laikā nosūtīt Kalecgos (38017) `SetData(0,1)` un `SetData(0,2)`; abām attiecīgi jāpalaiž timed action list 3801700 un 3801701 tikai vienu reizi.
- Pārbaudīt Kalecgos waypoint 1 pauzi un pagriešanos pret Krasus (27990), pēc tam waypoint 2 apstāšanos un pagriešanos mājas virzienā.

## Pakete 90 — Alexstrasza dublētās SetData darbības

Fails: `sql/updates/world/2026_09_18_80_archive_duplicate_alexstrasza_dataset_actions.sql`.

Alexstrasza the Life-Binder (26917) SmartAI saturēja pa divām kopijām abiem pagriešanās notikumiem `SetData(0,1)` un `SetData(0,2)`. Sākotnējās rindas ar `link=0` jau ir aktīvas un attiecīgi pagriež NPC uz norādītu pozīciju vai pret sevi/mājas orientāciju. Vēlāk 2020 dumpā pievienotajām identiskajām kopijām bija `link=1`; otrā kopija tādēļ sasaistīja pati sevi un loaderī tika noraidīta, bet abas kopā nevajadzīgi dublēja vienu un to pašu uzvedību.

Abas precīzās vēlākās kopijas saglabātas `_backup_20260918_duplicate_alexstrasza_dataset_actions` un arhivētas. Divas sākotnējās funkcionālās `link=0` rindas nav mainītas.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; abas vēlākās kopijas ir backupā, bet aktīvajā tabulā paliek tieši divas sākotnējās `link=0` darbības — viena katrai datu vērtībai.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 329 līdz 328; Alexstrasza pašsaites kļūda pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Quel'Delar notikuma laikā nosūtīt Alexstrasza (26917) `SetData(0,1)`; viņai vienu reizi jāpagriežas uz skriptā norādīto orientāciju.
- Nosūtīt `SetData(0,2)` un pārbaudīt atgriešanos paredzētajā pašas/mājas orientācijā; neviena pagriešanās nedrīkst dublēties vai izraisīt SmartAI ciklu.

## Pakete 91 — Ancient Drakkari ierašanās ķēde

Fails: `sql/updates/world/2026_09_18_81_fix_ancient_drakkari_arrival_chain.sql`.

Ancient Drakkari Warmonger (26811) un Ancient Drakkari Soothsayer (26812) pēc spell 47778 nejauši palaiž vienu no sešām timed action list 2681100–2681105, kas aizved radījumu uz point 1–6. Katram no sešiem `POINT_REACHED` eventiem ir `link=7`, tātad kopīgajai `id=7` rindai jābūt linked darbībai: izveidot GO 188525 “Drakkari Spirit Particles” un turpināt uz `id=8`, kas despawn'o radījumu.

`id=7` abiem NPC kļūdaini bija saglabāta kā vēl viens point-6 events ar pašsaiti. Tā izlabota uz `event_type=61 (LINK)` un `link=8`; spell, particles GO un despawn darbības nav mainītas. Sākotnējās `id=7` un `id=8` rindas abiem NPC saglabātas `_backup_20260918_ancient_drakkari_arrival_chain`.

### Pārbaudes rezultāts

- SQL updateris sekmīgi piemēroja migrāciju; backupā ir četras sākotnējās rindas. Abiem NPC aktīvā kopīgā ķēde tagad ir `id 7 (LINK, spawn GO) -> id 8 (LINK, despawn)`.
- Pilns `worldserver` starts pabeigts 12 sekundēs. `DBErrors.log` kļūdu skaits samazinājās no 328 līdz 326; abas Ancient Drakkari pašsaites kļūdas pazuda un jauna kļūda neradās.
- `Server.log` kļūdu skaits palika 116. Serveris korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Uz Ancient Drakkari Warmonger (26811) un Soothsayer (26812) izmantot spell 47778; katram jāizvēlas viens no sešiem kustības galapunktiem, galā jānostrādā spell 47798, jāparādās Drakkari Spirit Particles (188525) un NPC jādespawn'o.
- Testu atkārtot vairākas reizes, lai nosegtu dažādus point 1–6; īpaši point 6 nedrīkst izveidot particles divreiz vai izraisīt bezgalīgu linked ķēdi.

## Pakete 92 — Captured Vile Fin atgriešanas ķēde

Fails: `sql/updates/world/2026_09_18_82_fix_captured_vile_fin_return_chain.sql`.

Quest 24974 “Ever So Lonely” Captured Vile Fin Puddlejumper (38923) `SetData(1,1)` plūsmai jāizpilda trīs darbības: piešķirt owner spēlētājam Murloc Return KC (38887), pārslēgt sagūstīto murloc uz phase mask 2 un despawn'ot to. Datu komentāri šo secību saglabāja, bet otrā darbība kļūdaini bija atkārtots `SetData` events ar `id=1, link=1`, savukārt despawn rinda izmantoja to pašu `id=1`.

Ķēde normalizēta uz `id 0 -> id 1 -> id 2`: phase darbība tagad ir īsts `LINK` events ar `link=2`, bet despawn darbībai piešķirts `id=2`. Sākotnējās trīs plūsmas rindas saglabātas `_backup_20260918_captured_vile_fin_return_chain`; kill credit, phase mask un despawn darbības nav dzēstas.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir visas 3 sākotnējās plūsmas rindas.
- Datubāzē apstiprināta aktīvā `id 0 -> id 1 -> id 2` ķēde: kill credit, phase mask 2, despawn.
- Pilns `worldserver` starts pabeigts 13 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 326 uz 325, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Tirisfal Glades pieņemt quest 24974 “Ever So Lonely”, ar Murloc Leash sagūstīt Vile Fin Puddlejumper un aizvest Captured Vile Fin Puddlejumper (38923) pie Sedrick Calston (38925).
- Atgriešanas brīdī jāieskaitās mērķim “Vile Fin returned” (38887), murloc jāmaina fāze un jāpazūd; credit jāpiešķir tieši vienu reizi un murloc nedrīkst palikt sekojam spēlētājam.

## Pakete 93 — Corrupted Blackwood attīrīšanas plūsma

Fails: `sql/updates/world/2026_09_18_83_fix_corrupted_blackwood_cleanse_chain.sql`.

Corrupted Blackwood (33044) quest 13545 “Cleansing the Afflicted” `SpellHit` rindām bija pašsaite un trīs atkārtoti eventi ar vienu `id`. Turklāt kļūdainās rindas tūlīt pēc Blessed Herb Bundle (62092) izmantošanas padarīja furbolg draudzīgu, piešķīra Spirit of Corruption (33000) kill credit un despawn'oja furbolg, apejot paredzēto cīņu.

Esošais spirita izsaukums saglabāts kā patstāvīgs `SpellHit` events. Spirit of Corruption nāves SmartAI jau nosūta `SetData(0,1)` tuvākajam 33044, tādēļ bijušās priekšlaicīgās darbības pārveidotas par funkcionālu nāves pabeigšanas ķēdi: `SetData -> faction 35 -> thank-you teksts -> despawn pēc 5,5 s`. Manuāls priekšlaicīgs credit vairs netiek dots; objective 33000 credit rodas no paša spirita nogalināšanas. Pievienota hostile faction 2319 atjaunošana respawn brīdī. Visas 5 sākotnējās SmartAI rindas saglabātas `_backup_20260918_corrupted_blackwood_cleanse_chain`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir visas 5 sākotnējās SmartAI rindas.
- Datubāzē apstiprināts patstāvīgs `SpellHit` spirita izsaukums un atsevišķa `SetData(0,1) -> faction 35 -> text -> despawn` pabeigšanas ķēde; respawn rinda atjauno faction 2319.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 325 uz 324, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Darkshore pieņemt quest 13545 “Cleansing the Afflicted” un uz Corrupted Blackwood (33044) izmantot Blessed Herb Bundle (item 44889 / spell 62092).
- Jāparādās vienam Spirit of Corruption (33000); furbolg pirms spirita nāves nedrīkst kļūt draudzīgs, dot credit vai pazust.
- Pēc spirita nogalināšanas jāieskaitās vienam “Blackwood Furbolg Cleansed”, furbolg jākļūst draudzīgam, jāpasaka pateicība un pēc aptuveni 5,5 sekundēm jāpazūd. Pēc respawn tam atkal jābūt hostile.

## Pakete 94 — Grasping Earth SetData despawn

Fails: `sql/updates/world/2026_09_18_84_fix_grasping_earth_data_cooldown.sql`.

Grasping Earth (77893) `SetData(1,1)` despawn handlerim repeat intervāls bija apgriezts (`min=1`, `max=0`), tādēļ SmartAI visu rindu izlaida. Maksimālā vērtība izlīdzināta uz 1 ms, saglabājot sākotnējo tūlītējo reakciju un despawn darbību. Sākotnējā rinda saglabāta `_backup_20260918_grasping_earth_data_cooldown`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir sākotnējā rinda un aktīvajam handlerim apstiprināts repeat intervāls `1..1` ms.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 324 uz 323, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Notikumā, kurā parādās Grasping Earth (77893), izpildīt mehāniku, kas tam nosūta `SetData(1,1)`; objekta NPC jādespawn'o bez aizķeršanās un pārējām spell/timed-action rindām jāturpina darboties.

## Pakete 95 — Roakk the Zealot home-position events

Fails: `sql/updates/world/2026_09_18_85_fix_roakk_home_event.sql`.

Roakk the Zealot (79490) `Set Home Position` rindā event tipa vērtība `4` (`AGGRO`) bija nobīdīta uz `event_param1`, atstājot `event_type=0` (`UPDATE_IC`) ar nederīgu taimeri `4..0`. Event tips pārvietots pareizajā kolonnā un parametri atiestatīti uz nulli. Sākotnējā rinda saglabāta `_backup_20260918_roakk_home_event`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir sākotnējā rinda un aktīvajā rindā apstiprināts `event_type=4` ar nulles parametriem.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 323 uz 322, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Draenor notikumā iesaistīt cīņā Roakk the Zealot (79490); aggro brīdī tam jāsaglabā pašreizējā vieta kā home position, jāizpilda kaujas spell/dialoga rindas un evade gadījumā tas nedrīkst mēģināt atgriezties uz nederīgu sākotnējo punktu.

## Pakete 96 — Summoning Portal periodiskais taimeris

Fails: `sql/updates/world/2026_09_18_86_fix_summoning_portal_timer.sql`.

Summoning Portal (105038) spell 208041 periodiskajai rindai repeat intervāls bija `120000..12000` ms. Maksimālajai robežai trūka viena nulle; tā izlīdzināta uz determinētu `120000..120000` ms jeb divām minūtēm. Sākotnējā rinda saglabāta `_backup_20260918_summoning_portal_timer`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir sākotnējā rinda un aktīvajā rindā apstiprināts repeat intervāls `120000..120000` ms.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 322 uz 321, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Vietā/notikumā ar Summoning Portal (105038) novērot to ilgāk par divām minūtēm; spell 208041 jāizpildās sākumā un pēc tam atkārtoti ik pēc aptuveni 120 sekundēm, neradot strauju spell spam.

## Pakete 97 — Malevolent Hatchling izsaucēja mērķis

Fails: `sql/updates/world/2026_09_18_87_fix_malevolent_hatchling_target.sql`.

Quest 10861 “Veil Lithic: Preemptive Strike” Cursed Egg izsauktais Malevolent Hatchling (22337) bija konfigurēts uz `CLOSEST_PLAYER` ar nederīgu nulles radiusu, lai gan rindas komentārs un pārējie `Just Summoned - Attack Invoker` skripti norāda uz event izraisītāju. Mērķis nomainīts uz `ACTION_INVOKER` (7), kam distance nav vajadzīga. Sākotnējā rinda saglabāta `_backup_20260918_malevolent_hatchling_target`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir sākotnējā rinda un aktīvajā rindā apstiprināts `target_type=7` bez distances parametra.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 321 uz 320, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Blade's Edge Mountains pieņemt quest 10861 “Veil Lithic: Preemptive Strike” un aktivizēt Cursed Egg (185211), kas izsauc Malevolent Hatchling (22337).
- Hatchling uzreiz jāuzbrūk spēlētājam, kurš aktivizēja olu; tam jāturpina lietot Charge (36140) 8–25 yd attālumā un Terrifying Screech (38021).

## Pakete 98 — Kormrok hand nāves mērķa radius

Fails: `sql/updates/world/2026_09_18_88_fix_kormrok_hand_death_target.sql`.

Kormrok Grasping Hand (93838) un Dragging Hand (93839) nāves rindas lietoja `CLOSEST_PLAYER`, bet obligātais max distance bija 0, tādēļ abas darbības tika izlaistas. Tā kā roka parādās pie satvertā spēlētāja un šīs pašas mehānikas tuvuma pārbaude ir 5 jardi, abām spell 181321 rindām uzstādīts 5 yd radius. Abas sākotnējās rindas saglabātas `_backup_20260918_kormrok_hand_death_target`.

### Pārbaudes rezultāts

- SQL updateris izpildījās sekmīgi; backup tabulā ir abas sākotnējās rindas un abām aktīvajām rindām apstiprināts `CLOSEST_PLAYER` radius 5 yd.
- Pilns `worldserver` starts pabeigts 12 sekundēs; `DBErrors.log` kļūdu skaits samazinājās no 320 uz 318, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Hellfire Citadel Kormrok cīņā izraisīt Grasping Hands un Dragging Hands mehānikas; iznīcinot 93838/93839, nāves aura 181321 jāattiecas uz pie rokas esošo satverto spēlētāju, nevis tālu esošu reida biedru.
- Atbrīvotajam spēlētājam jākļūst kustīgam, un citu roku satvertie spēlētāji nedrīkst tikt ietekmēti no vairāk nekā 5 jardu attāluma.

## Pakete 99 — Halls of Valor apkārtējās emote taimeri

Fails: `sql/updates/world/2026_09_18_89_fix_halls_of_valor_emote_timers.sql`.

Trīs Halls of Valor NPC (95834, 95842 un 97087) periodiskajām `RANDOM_EMOTE` rindām sākuma intervāla maksimums bija `0`, lai gan minimumi bija attiecīgi 1000, 3000 un 500 ms. SmartAI šos notikumus tādēļ pilnībā izlaida. Maksimumi pielīdzināti jau definētajiem minimumiem. Pirmais pārbaudes starts atklāja arī iepriekš aiz pirmās validācijas kļūdas paslēpto 97087 atkārtojuma intervālu `5000..4300`; arī tam saglabāts definētais piecu sekunžu minimums kā determinēts `5000..5000` ms intervāls. Visas trīs sākotnējās rindas saglabātas `_backup_20260918_halls_of_valor_emote_timers`.

### Pārbaudes rezultāts

- Backup tabulā ir visas trīs sākotnējās rindas; aktīvajās rindās apstiprināti derīgi sākuma intervāli `1000..1000`, `3000..3000`, `500..500` ms un 97087 atkārtojums `5000..5000` ms.
- Pēc papildlabojuma veikts otrs pilns `worldserver` starts, kas pabeigts 12 sekundēs; visu trīs taimeru kļūdas pazuda un `DBErrors.log` kļūdu skaits samazinājās no 318 uz 315. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Atsevišķā 95834 spell rindā vēl paliek nesaistīta neatbalstīta `target_type=28` kļūda; tā nav apklusināta un tiks labota savā paketē.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Halls of Valor vietās atrast NPC 95834, 95842 un 97087 un novērot tos ārpus kaujas vismaz 20–30 sekundes.
- NPC periodiski jāizpilda viena no emote 15, 22, 274 vai 71; 97087 nedrīkst spamot emotes biežāk par aptuveni reizi piecās sekundēs, un uzvedība nedrīkst traucēt to kaujas spell rindām.

## Pakete 100 — SmartAI FARTHEST mērķa atbalsts

Faili: `src/server/game/AI/SmartScripts/SmartScriptMgr.h`, `src/server/game/AI/SmartScripts/SmartScriptMgr.cpp` un `src/server/game/AI/SmartScripts/SmartScript.cpp`.

Valarjar Mystic (95834) Healing Light (198931) rinda lieto `target_type=28` ar 40 jardu robežu. Šī nav patvaļīga DB vērtība: TrinityCore SmartAI definīcijā un LegionCore priekšteča galvenē 28 ir `SMART_TARGET_FARTHEST` (`maxDist`, `playerOnly`, `isInLos`). Šajā zarā definīcija un izpilde bija izkritusi, lai gan zemākā `SELECT_TARGET_FARTHEST` atlase joprojām bija kodolā. Atjaunots trūkstošais enum, parametru izkārtojums, validācijas atļauja un tālākā derīgā threat-list mērķa atlase ar distances, player-only un line-of-sight filtriem. DB rinda nav mainīta, tādēļ datu backup šai koda paketei nav nepieciešams.

### Pārbaudes rezultāts

- Pilna Release kompilācija un instalēšana pabeigta sekmīgi; pēc gala atlases filtra izmaiņas veikta atkārtota kompilācija.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 95834 `target_type(28)` kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 315 uz 314. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Halls of Valor Normal vai Heroic grūtībā iesaistīt kaujā Valarjar Mystic (95834) kopā ar vairākiem citiem Valarjar; ļaut kādam sabiedrotajam saņemt bojājumus un neinterruptēt pirmo Healing Light (198931).
- Mystic jāuzsāk pārtraucama Healing Light buršana, un dziedināšanai jānonāk pie draudzīga NPC, nevis spēlētāja. Ar izretinātu grupu papildus jāpārbauda, ka 40 jardu robeža tiek ievērota. Ja spell klienta/DBC implicitā mērķēšana nepārvērš tālāko threat-list atskaites mērķi par draudzīgu heal mērķi, šī konkrētā DB rinda būs atsevišķi jāpārveido uz sabiedrotā atlasi; pašreiz ir atjaunota tieši tās sākotnēji deklarētā SmartAI semantika.

## Pakete 101 — Hillsbrad boss cīņu mērķa radiusi

Fails: `sql/updates/world/2026_09_18_90_fix_hillsbrad_boss_attack_targets.sql`.

Magistrate Burnside (47790) un Warden Stillwater (48080) timed-action sarakstu noslēguma `ATTACK_START` rindas bija paredzētas tuvākajam spēlētājam, bet abām obligātais `CLOSEST_PLAYER` radius bija 0, tādēļ paredzētās kvestu boss cīņas sākšana tika izlaista. Sarakstus izsauc pašu NPC `DATA_SET` notikumi, tāpēc tajos nav uzticama spēlētāja `ACTION_INVOKER`; saglabāts `CLOSEST_PLAYER` un noteikta 100 jardu lokālā encounter robeža. Sākotnējās rindas saglabātas `_backup_20260918_hillsbrad_boss_attack_targets`.

### Pārbaudes rezultāts

- Backup tabulā ir abas sākotnējās rindas; 4779000/6 un 4808000/22 aktīvajās rindās apstiprināts `CLOSEST_PLAYER` radius 100 jardi.
- Pilns `worldserver` starts pabeigts 12 sekundēs; abas `maxDist 0` kļūdas pazuda un `DBErrors.log` kļūdu skaits samazinājās no 314 uz 312. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Hillsbrad Foothills/The Sludge Fields kvestu ķēdē izspēlēt Magistrate Burnside sastapšanos; pēc dialoga un aura 89161/89159 noņemšanas viņam jāpārvietojas uz kaujas punktu, jānoņem neuzbrūkamības flags un pēc aptuveni trim sekundēm jāuzbrūk tuvākajam grupas spēlētājam.
- Izspēlēt quest 28237 “A Blight Upon the Land” Warden Stillwater daļu kopā ar Master Apothecary Lydon un Johnny Awesome. Pēc dialoga, Sludge Guard izsaukšanas un frakcijas maiņas Stillwater jāsāk kauja ar klātesošo spēlētāju; NPC nedrīkst izvēlēties spēlētāju ārpus aptuveni 100 jardu notikuma zonas.

## Pakete 102 — Foreman Thazz'ril quest sveiciens

Fails: `sql/updates/world/2026_09_18_91_fix_foreman_thazzril_quest_greeting.sql`.

Foreman Thazz'ril (80140) quest 34818 “They Call Him Lantresor of the Blade” pieņemšanas rindai komentārs un tekstu dati paredzēja dialogu, bet `action_type` bija atstāts kā nederīgs `NONE` (0). NPC ir derīga `creature_text` grupa 0, un blakus quest 34899 rinda tādā pašā veidā lieto `TALK` grupu 1. Pirmajai rindai atjaunota `TALK` (1) darbība ar grupu 0. Sākotnējā rinda saglabāta `_backup_20260918_foreman_thazzril_quest_greeting`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai quest 34818 rindai apstiprināts `action_type=1`, `action_param1=0`, un abi Thazz'ril quest dialogi tagad atsaucas uz eksistējošām teksta grupām 0 un 1.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 80140 nederīgās darbības kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 312 uz 311. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Horde garrison/Nagrand ķēdē pieņemt no Foreman Thazz'ril quest 34818 “They Call Him Lantresor of the Blade”; viņam nekavējoties jāpasaka teksta grupas 0 replika (par būvniecības plāniem), neveidojot dubultu dialogu.
- Vēlāk pieņemt quest 34899 “A Choice to Make” un pārbaudīt, ka saglabājusies atsevišķā grupas 1 replika par staļļu vai tanku darbnīcas izvēli.

## Pakete 103 — Asha Ravensong čempiones atbilde

Fails: `sql/updates/world/2026_09_18_92_fix_asha_ravensong_champion_reply.sql`.

Asha Ravensong (108326) quest 42697 “Champion: Asha Ravensong” nodošanas rinda bija atstāta ar nederīgu `NONE` (0) darbību, lai gan tās komentārs, blakus esošie čempionu skripti un NPC teksta dati paredz atbildes repliku. Ashai ir tieši viena `creature_text` grupa — grupa 3 ar pateicību spēlētājam un BroadcastText 115838 —, tādēļ rindai atjaunota `TALK` (1) darbība ar grupu 3. Sākotnējā rinda saglabāta `_backup_20260918_asha_ravensong_champion_reply`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai quest 42697 rindai apstiprināts `action_type=1` un `action_param1=3`.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 108326 nederīgās darbības kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 311 uz 310. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Demon Hunter Class Hall kampaņā pabeigt un nodot Ashai Ravensong quest 42697 “Champion: Asha Ravensong”.
- Quest nodošanas brīdī Ashai vienreiz jāpasaka pateicības replika “Thank you, $n. I will not let you down.”; čempionei jākļūst pieejamai paredzētajā follower sistēmā, un replika nedrīkst atkārtoties bez atkārtotas quest nodošanas.

## Pakete 104 — Warden Stillwater transformācija

Fails: `sql/updates/world/2026_09_18_93_fix_warden_stillwater_morph.sql`.

Warden Stillwater (48080) pie zema veselības līmeņa paredzētajai transformācijai vienlaikus bija aizpildīts gan `CreatureId=48103`, gan `ModelId=30993`, lai gan SmartAI `MORPH_TO_ENTRY_OR_MODEL` pieņem tikai vienu no tiem un tādēļ visu darbību izlaida. Creature veidne 48103 ir eksistējošā Warden Stillwater transformētā forma un pati jau izmanto tieši display 30993. Saglabāta datu vadītā veidnes atsauce 48103, bet liekais modeļa parametrs iestatīts uz 0. Sākotnējā rinda saglabāta `_backup_20260918_warden_stillwater_morph`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 48080/3 rindai apstiprināts `action_param1=48103`, `action_param2=0`, un 48103 WDB veidnes display ir 30993.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 48080 dubultā morph parametra kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 310 uz 309. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Hillsbrad Foothills quest 28237 “A Blight Upon the Land” sastapšanās laikā cīnīties ar Warden Stillwater (48080) un samazināt viņa veselību zem 50%.
- Vienreiz jāatskaņojas grupas 6 tekstam par “other form”, Stillwater vizuāli jāpārvēršas par veidnes 48103 formu ar display 30993, pēc tam jāizpilda piesaistītais knockback 67605; NPC nedrīkst pazust vai zaudēt notiekošās cīņas stāvokli.

## Pakete 105 — Seirdr kaujas spell taimeris

Fails: `sql/updates/world/2026_09_18_94_fix_seirdr_combat_timer.sql`.

Seirdr (115751) pirmajai kaujas spell 186327 rindai sākuma laiks bija derīgs, bet atkārtojuma intervāla abi autora norādītie gali bija apgriezti (`6000..2000` ms), tādēļ SmartAI rindu pilnībā izlaida. Abi laiki saglabāti un sakārtoti kā derīgs nejaušs `2000..6000` ms intervāls. Sākotnējā rinda saglabāta `_backup_20260918_seirdr_combat_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 115751/0 rindai apstiprināts sākuma intervāls `1000..1000` ms un atkārtojuma intervāls `2000..6000` ms.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 115751 apgriezto robežu kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 309 uz 308. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Stormheim zonā 7334, apgabalā 8297, atrast kādu no Seirdr (115751) spawn un uzsākt kauju.
- Aptuveni vienu sekundi pēc kaujas sākuma Seirdr jālieto spell 186327 uz nejaušu naidīgu mērķi un pēc tam tas jāatkārto ar mainīgu 2–6 sekunžu intervālu; atsevišķajam spell 186338 jāturpina darboties ik pēc 20 sekundēm.

## Pakete 106 — T’paartos un Exodar emote ciklu taimeri

Fails: `sql/updates/world/2026_09_18_95_fix_exodar_emote_cycle_timers.sql`.

T’paartos (128562) un divu Exodar Citizen veidņu (128656, 128657) secīgo ārpus-kaujas emote ciklu pirmajās rindās pilna cikla ilgums bija ievietots sākuma maksimuma kolonnā, bet atkārtojuma maksimums bija 0. To apstiprina tās pašas grupas nākamās rindas: T’paartos emotes sākas ar 0/4/8/12 sekunžu nobīdi un atkārtojas ik pēc 16 sekundēm, bet abu iedzīvotāju emotes — ar 0/8/16/24 sekunžu nobīdi un 32 sekunžu ciklu. Pirmajām rindām atjaunots precīzs nulles sākums un attiecīgi `16000..16000` vai `32000..32000` ms atkārtojums. Visas trīs sākotnējās rindas saglabātas `_backup_20260918_exodar_emote_cycle_timers`.

### Pārbaudes rezultāts

- Backup tabulā ir visas trīs sākotnējās rindas; 128562/0 ir `0..0` ms sākums un 16 sekunžu atkārtojums, bet 128656/0 un 128657/0 — `0..0` ms sākums un 32 sekunžu atkārtojums.
- Pilns `worldserver` starts pabeigts 12 sekundēs; visas trīs nederīgo taimeru kļūdas pazuda un `DBErrors.log` kļūdu skaits samazinājās no 308 uz 305. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Lightforged Draenei/T’paartos notikuma vietā vērot T’paartos (128562) ārpus kaujas vismaz 20 sekundes: emote 11, 23, 15 un 17 jāizpildās secīgi ik pēc aptuveni četrām sekundēm un ciklam jāsākas no jauna pēc 16 sekundēm.
- Tajā pašā notikuma ainā vērot Exodar Citizen 128656 un 128657 vismaz 40 sekundes: katram jāizpilda savs četru emote komplekts ar astoņu sekunžu nobīdi un 32 sekunžu atkārtojumu, bez emote spama uzreiz pēc spawn.

## Pakete 107 — Legion Portal izsaukšanas taimeris

Fails: `sql/updates/world/2026_09_18_96_fix_legion_portal_summon_timer.sql`.

Legion Portal (111357) timed-action saraksts secīgi izsauc trīs Greater Imp (111424), Felhound (111422), Felguard Invader (111242) un Abyssal Shard (111576). Visām sešām darbībām ir precīzs 10 sekunžu solis, bet trešā Greater Imp rinda bija bojāta kā `10000..1000` ms. Tai atjaunots ar pārējo sarakstu saskanīgs `10000..10000` ms intervāls. Sākotnējā rinda saglabāta `_backup_20260918_legion_portal_summon_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 111357 timed-action id 2 rindai apstiprināts `10000..10000` ms intervāls un Greater Imp 111424 izsaukšana ar despawn tipu 8.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 111357 nederīgā taimera kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 305 uz 304. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ar attiecīgā Legion notikuma palīdzību vai GM testa vidē izsaukt Legion Portal (111357) un ļaut tam nodzīvot vismaz 70 sekundes.
- Portālam ik pēc aptuveni 10 sekundēm secīgi jāizsauc trīs Greater Imp, tad Felhound, Felguard Invader un Abyssal Shard; trešais imp nedrīkst tikt izlaists, un visi summon jāparādās pie portāla.

## Pakete 108 — Gallywix “You’re Fired” sākuma taimeris

Fails: `sql/updates/world/2026_09_18_97_fix_gallywix_youre_fired_timer.sql`.

Trade Prince Gallywix (395820) “Final Confrontation” cīņas četras sākuma spējas datubāzē ir apzināti sadalītas secīgos logos: Revenue Stream 1–3 s, Unload Toxic Assets 4–5,8 s, Downsizing 6–7 s un You’re Fired no 8 sekundēm. Pēdējās rindas maksimums bija `1000` ms, acīmredzami pazaudējot vienu nulli un radot nederīgu `8000..1000` intervālu. Atjaunots paredzētais `8000..10000` ms sākuma logs, nemainot tās 17–20 sekunžu atkārtojumu. Sākotnējā rinda saglabāta `_backup_20260918_gallywix_youre_fired_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 395820/4 rindai apstiprināts `8000..10000` ms sākums, `17000..20000` ms atkārtojums, spell 74004 un nejaušs naidīgs mērķis.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 395820 nederīgā taimera kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 304 uz 303. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Goblinu sākuma ķēdē Lost Isles izspēlēt quest 25251 “Final Confrontation” un sākt Trade Prince Gallywix cīņu.
- Pirmajās desmit sekundēs jāparādās visām četrām spējām to sadalītajos laika logos; You’re Fired (74004) jānotiek aptuveni 8–10 sekundes pēc kaujas sākuma, jāatskaņo piesaistītā grupas 6 replika un vēlāk jāturpina atkārtoties ik pēc 17–20 sekundēm.

## Pakete 109 — Thalyssra Vanthir vīzijas dialoga taimeris

Fails: `sql/updates/world/2026_09_18_98_fix_thalyssra_echo_dialogue_timer.sql`.

First Arcanist Thalyssra (115557) gossip izvēle 20576 iedarbina Vanthir meklēšanas vīzijas timed-action sarakstu 11555700. Pēc pirmās trīs sekunžu pauzes teksta grupām 1–8 ir precīzas astoņu sekunžu pauzes, bet grupas 6 rindā maksimums bija bojāts uz 5000 ms, radot nederīgu `8000..5000` intervālu un pārtraucot saraksta ielādi. Atjaunots ar pārējo dialogu saskanīgs `8000..8000` ms intervāls. Sākotnējā rinda saglabāta `_backup_20260918_thalyssra_echo_dialogue_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 11555700/8 rindai apstiprināts `8000..8000` ms intervāls, `TALK` grupa 6 un pašas Thalyssra mērķis.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 11555700 nederīgā taimera kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 303 uz 302. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Suramar zonā 7637, apgabalā 8487, pie First Arcanist Thalyssra (115557) izvēlēties gossip “I’m ready, Talisra.” un noskatīties visu Vanthir vīziju.
- Pēc sākuma teksta dialogam jāturpinās ar aptuveni astoņu sekunžu pauzēm līdz grupai 8; īpaši jāpārbauda, ka pēc grupas 5 seko grupas 6 replika par Vanthir izsīkumu, tad “Found him!”, un beigās tiek piešķirti visu Echo of Vanthir vienību kredīti un noņemta aura 229713.

## Pakete 110 — Kirin Tor Tavern Crawl ambientie taimeri

Fails: `sql/updates/world/2026_09_18_99_fix_tavern_crawl_action_timers.sql`.

Četru konkrētu Tavern Enthusiast (119047/119046; GUID 373305, 373335, 373350 un 373373) “Kirin Tor Tavern Crawl” ambientajos timed-action sarakstos bija 13 rindas ar apgrieztām min/max paužu robežām. Sarakstos ir arī derīgi nejauši intervāli, tāpēc neviens ilgums nav aizstāts vai vienādots: katrā nederīgajā pārī saglabāti abi autora skaitļi un tikai samainīta to secība. Visas 13 sākotnējās rindas saglabātas `_backup_20260918_tavern_crawl_action_timers`.

### Pārbaudes rezultāts

- Backup tabulā ir 13 sākotnējās rindas; visos četros sarakstos pēc labojuma `event_param1 <= event_param2`, un arī iepriekš derīgās rindas palika nemainītas.
- Pilns `worldserver` starts pabeigts 12 sekundēs; visas 13 Tavern Crawl taimeru kļūdas pazuda un `DBErrors.log` kļūdu skaits samazinājās no 302 uz 289. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Aktīva Kirin Tor Tavern Crawl notikuma laikā novērot Tavern Enthusiast ar GUID 373305, 373335, 373350 un 373373 attiecīgi Kalimdorā, Northrendā, Outlandā un Pandaria tavernu vietās.
- Katram NPC pēc iesaistīšanās kaujā jāizpilda viss tā ambientais emote/spell saraksts: animācijām, spell 236747/105590 un noslēguma spell 35517 jānotiek ar mainīgām, bet vienmēr derīgām pauzēm; saraksts nedrīkst apstāties pie pirmās rindas.

## Pakete 111 — Archivist Mechaton uzbrukuma taimeris

Fails: `sql/updates/world/2026_09_18_100_fix_archivist_mechaton_attack_timer.sql`.

Archivist Mechaton (29775) drošības sekvences visas darbības izmanto precīzas, vienādas min/max pauzes. Pēc teksta grupas 6 viņam pēc divām sekundēm jānoņem `UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE` (768) un vēl pēc 1,5 sekundēm jāuzbrūk notikumu izraisījušajam spēlētājam. Flags rindas maksimums bija `200` ms, tātad tam bija pazudusi viena nulle un nederīgā `2000..200` intervāla dēļ pāreja uz kauju tika izlaista. Atjaunota precīza `2000..2000` ms pauze. Sākotnējā rinda saglabāta `_backup_20260918_archivist_mechaton_attack_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 2977500/10 rindai apstiprināts `2000..2000` ms intervāls, flags maska 768 un paša Archivist Mechaton mērķis.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 2977500 nederīgā taimera kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 289 uz 288. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Inventor’s Disk/Archivist notikumā vai GM testa vidē izsaukt Archivist Mechaton (29775) un ļaut tam izpildīt visu aptuveni 49 sekunžu drošības dialogu un scan spell 55224.
- Divas sekundes pēc pēdējās grupas 6 replikas NPC jākļūst atlasāmam un uzbrūkamam, bet vēl aptuveni pēc 1,5 sekundēm tam jāsāk uzbrukums spēlētājam, kurš izraisīja notikumu; pāreja nedrīkst iestrēgt neuzbrūkamā stāvoklī.

## Pakete 112 — Agatha kustības sekvences taimeris

Fails: `sql/updates/world/2026_09_18_101_fix_agatha_movement_timer.sql`.

Lady Sylvanas Windrunner (44365) notikuma timed-action saraksts beigās nosūta `DATA_SET 2,2` tuvumā esošajai Agatha (44608), kas iedarbina viņas divu punktu kustību, spell 83173 un abu Fallen Human auru noņemšanu. Pirmās kustības rindai bija divas jēgpilnas, bet apgrieztas robežas `2000..1000` ms. Abi autora ilgumi saglabāti un sakārtoti kā derīga nejauša `1000..2000` ms aizture. Sākotnējā rinda saglabāta `_backup_20260918_agatha_movement_timer`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 4460800/0 rindai apstiprināts `1000..2000` ms intervāls un nemainīts pirmais galamērķis `(1364.32, 1028.81, 67.111)`.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 4460800 nederīgā taimera kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 288 uz 287. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Silverpine Forest zonā 130, apgabalā 5369 pie koordinātēm ap `(1364, 1029, 56)`, izspēlēt Lady Sylvanas (44365) un Agatha (44608) saistīto notikumu līdz Sylvanas nosūta `DATA_SET 2,2`.
- Agatha pēc 1–2 sekundēm jāpārvietojas uz pirmo punktu, vēl pēc vienas sekundes uz otro punktu, pēc 2,5 sekundēm jālieto spell 83173, pēc tam jānoņem aura 29266 abiem Fallen Human (44592/44593) un jāiziet no kaujas; sekvence nedrīkst apstāties pirms pirmās kustības.

## Pakete 113 — timed-action grūtību flagu migrācija

Fails: `sql/updates/world/2026_09_18_102_migrate_timed_action_difficulties.sql`.

Repozitorija vēsturiskā SmartAI migrācija `sql/old/world/0020_creature_updates.sql` vecos difficulty bitus pārvietoja uz `Difficulties` kolonnu, taču apzināti atlasīja tikai `source_type=0`. Tādēļ sešas saistītās `source_type=9` timed-action rindas palika vecajā formātā: viena Fiery Vortex rinda ar flagiem 30, četras Majordomo Staghelm dialoga rindas ar flagiem 31 un viena Air flow rinda ar flagu 16. Izmantota tieši repozitorijā dotā karte: biti 2+4+8+16 kļuva par grūtībām `1,2,3,4,5,6`, bet bits 16 viens pats — par grūtību `6`; Staghelm rindās saglabāts neatkarīgais `NOT_REPEATABLE` bits 1. Visas sešas sākotnējās rindas saglabātas `_backup_20260918_timed_action_difficulties`.

### Pārbaudes rezultāts

- Backup tabulā ir sešas sākotnējās rindas; 53693/9/0 un 5410100/9/0–3 tagad lieto `Difficulties='1,2,3,4,5,6'`, bet 60005200/9/1 lieto `Difficulties='6'`; nevienā nav palikuši deprecated biti.
- Pilns `worldserver` starts pabeigts 12 sekundēs; visas sešas deprecated event flag kļūdas pazuda un `DBErrors.log` kļūdu skaits samazinājās no 287 uz 281. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Firelands sastapšanās vietā pārbaudīt Fiery Vortex (53693) un Fiery Tornado (53698): Vortex pēc izsaukšanas lieto 99793, piesaista Tornado auru 99817 un pēc aptuveni 27 sekundēm to noņem visās datubāzē norādītajās grūtībās.
- Majordomo Staghelm (54101/52571) notikumā pienākt 50 jardu robežās: viņa trīs dialoga grupām jāatskaņojas vienreiz ar 0, 11 un 6 sekunžu secīgām pauzēm, tad pēc 10 sekundēm jānoņem flags 832.
- Grūtībā 6 pārbaudīt Air flow (600052) sekvenci pie Master Snowdrift satura: abām action 205 rindām jāielādējas, bet citās grūtībās tikai kopīgajai id 0 rindai; vizuālajai/kustības sekvencei nedrīkst būt dublikātu.

## Pakete 114 — Thunderlord Beast-Tender Bestial Wrath mērķis

Fails: `sql/updates/world/2026_09_18_103_fix_beast_tender_wrath_target.sql`.

Thunderlord Beast-Tender (80423) Blackrock Foundry kaujā spell 162606 jeb Bestial Wrath bija `CREATURE_DISTANCE` mērķis ar parametriem `(20, 0)`. Šā mērķa formāts ir `(CreatureEntry, maxDist)`, tāpēc kodols mēģināja atrast neeksistējošu creature entry 20 un visu darbību izlaida. Pašā encounter datu vietā 12,8 un 15,3 jardu attālumā atrodas Ornery Ironhoof (80534) un Stubborn Ironhoof (80526), kas atbilst Beast-Tender spējai stiprināt tuvumā esošu zvēru. Parametri pārvietoti uz `(0, 20)`: jebkurš radījums 20 jardos, pašu caster kodols no saraksta izslēdz. Sākotnējā rinda saglabāta `_backup_20260918_beast_tender_wrath_target`.

### Pārbaudes rezultāts

- Backup tabulā ir sākotnējā rinda; aktīvajai 80423/1 rindai apstiprināts `target_type=11`, `target_param1=0`, `target_param2=20`, bet spell un 10/36 sekunžu taimeri nav mainīti.
- Pilns `worldserver` starts pabeigts 12 sekundēs; 80423 neeksistējošā creature mērķa kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 281 uz 280. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Blackrock Foundry Workshop daļā iesaistīt kaujā Thunderlord Beast-Tender (80423), kam blakus atrodas Ornery Ironhoof (80534) un Stubborn Ironhoof (80526).
- Aptuveni 10 sekundes pēc kaujas sākuma Beast-Tender jālieto Bestial Wrath (162606) uz vienu vai abiem tuvumā esošajiem Ironhoof 20 jardu robežās, palielinot zvēra nodarīto bojājumu; spell nedrīkst tikt lietots uz paša Beast-Tender vai tāla, nesaistīta trash NPC.

## Pakete 115 — Thane Irglov Bull Rush darbība

Fails: `sql/updates/world/2026_09_18_104_fix_thane_irglov_bullrush_action.sql`.

Thane Irglov the Merciless (91892) kaujas rindai bija neatbalstīts `action_type=255`, tāpēc kodols visu Bull Rush darbību izlaida. Ne lokālajā kodolā, ne pārbaudītajās vēsturiskajās datubāzēs šāds darbības tips nav definēts. Rindas pārējie dati precīzi atbilst standarta `SMART_ACTION_CAST`: `action_param1=187406` ir Bull Rush spell, `target_type=2` ir pašreizējais upuris, un komentārā jau bija norādīts “cast - bullrush”. Darbības tips atjaunots uz `11`, nemainot spell, mērķi vai 2/7–19 sekunžu taimerus. Sākotnējā rinda saglabāta `_backup_20260918_thane_irglov_bullrush_action`.

### Pārbaudes rezultāts

- Backup tabulā ir viena sākotnējā rinda; aktīvajai 91892/1 rindai apstiprināts `action_type=11`, `action_param1=187406`, `target_type=2` un nemainīti taimeri.
- Pilns `worldserver` starts pabeigts 12 sekundēs; neatbalstītā darbības tipa kļūda pazuda un `DBErrors.log` kļūdu skaits samazinājās no 280 uz 279. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Stormheim kartē 1220, zonā 7541/apgabalā 7608 pie koordinātēm aptuveni `(2376, 2674, 301)` iesaistīt kaujā Thane Irglov the Merciless (91892).
- Aptuveni divas sekundes pēc kaujas sākuma un pēc tam ar 7–19 sekunžu intervālu viņam jālieto Bull Rush (187406) uz savu pašreizējo upuri; paralēli jāturpina darboties Sweeping Blade (186365) un Horn of Hrydshal (187429).

## Pakete 116 — novecojis Broken Shore quest spell loader

Fails: `src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp`.

`spell_q42740` loader bija reģistrēts, lai spell 227058 pēc quest 42740 spēlētāju ieliktu Broken Shore LFG 908. Taču pats avota komentārs norāda, ka spell šajā 7.3.5 klienta versijā neeksistē, bet `sql/old/world/0010_broken_shore_scenario.sql` tā DB piesaisti apzināti dzēš: otrā LFG pievienošana nav vajadzīga, jo to jau izdara ainas beigu `enterBrockenShores` triggeris. Klase nav dzēsta un paliek avotā kā vēsturiska atsauce; atslēgta tikai tās reģistrācija, lai kodols vairs negaidītu apzināti neesošo DB piesaisti.

### Pārbaudes rezultāts

- Release `worldserver` pilnībā pārbūvēts un uzinstalēts; kompilācija pabeigta sekmīgi (palika viena iepriekšēja, ar šo labojumu nesaistīta C4805 warning 2161. rindā).
- Pilns `worldserver` starts pabeigts 12 sekundēs; `spell_q42740` trūkstošās DB piesaistes kļūda pazuda un `DBErrors.log` skaits samazinājās no 279 uz 278. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Izspēlēt Broken Shore ievada quest 42740 līdz ainas beigām. `enterBrockenShores` ainas triggerim spēlētājs vienreiz jāpievieno LFG scenārijam 908 un jāpārved uz nākamo posmu.
- Pārbaudīt, ka nenotiek dubulta LFG pievienošana, atkārtots queue paziņojums vai iestrēgšana pēc ainas; neesošā spell 227058 izslēgšanai nav jāmaina pārējā scenārija gaita.

## Pakete 117 — Dark Shaman Koranthal Shadow Storm piesaiste

Fails: `sql/updates/world/2026_09_18_105_bind_koranthal_shadow_storm.sql`.

Dark Shaman Koranthal (61412) AI lieto Shadow Storm (119971), kura palīgspell 119973 ir dummy efekts. Kodola `spell_dark_shaman_koranthal_shadow_storm` loaderis šo efektu pārvērš par spell datos norādītā efekta lietošanu uz trāpīto mērķi, bet `spell_script_names` piesaiste bija iztrūkusi. Tā pati `(119973, spell_dark_shaman_koranthal_shadow_storm)` piesaiste ir atrodama ArgusCore 7.3.5 labojumā `2024_05_16_00_world.sql`, tāpēc tā nav minēta pēc nosaukuma vien. Pirms ievietošanas visas iespējamās esošās rindas tika atlasītas `_backup_20260918_koranthal_shadow_storm_script`; tabula ir tukša, kas apstiprina, ka iepriekš nebija ko pārrakstīt.

### Pārbaudes rezultāts

- Datubāzē apstiprināta viena precīza spell 119973 piesaiste loaderim; citas šī spell vai loadera rindas netika aizstātas.
- Pilns `worldserver` starts pabeigts 12 sekundēs; Koranthal loadera trūkstošās DB piesaistes kļūda pazuda un `DBErrors.log` skaits samazinājās no 278 uz 277. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Ragefire Chasm kartē 389 iesaistīt kaujā Dark Shaman Koranthal (61412; aptuvenā atrašanās vieta `(-117, 71, -21)`).
- Ap 20,5 sekundēm pēc kaujas sākuma bossam jāizsauc Shadow Storm (119971), jāparāda raid brīdinājums un spell 119973 dummy efektam korekti jālieto tā datos norādītais papildu efekts uz trāpītajiem mērķiem; atkārtojumam jānotiek aptuveni ik pēc 47,2 sekundēm.

## Pakete 118 — neaktīvs BattlePay level-100 produkta skripts

Fails: `src/server/scripts/BattlePay/battlepay_services.cpp`.

Kodols reģistrēja gan `battlepay_service_level90`, gan `battlepay_service_level100`, bet šīs datubāzes apzināti sakoptajā BattlePay katalogā ir tikai viens produkts: ID 109 “Level 90 Character Boost” ar `battlepay_service_level90`. To tieši nosaka arī repozitorija `sql/old/world/0003_battlepay_cleanup.sql`; level-100 produkta, veikala ieraksta vai attēlojuma datu nav. Tāpēc level-100 skriptu nedrīkstēja piesaistīt esošajam level-90 produktam. Generic `BattlePay_Level<100>` implementācija nav dzēsta un paliek pieejama nākotnes DB produktam; atslēgta tikai šobrīd neizmantotā reģistrācija.

### Pārbaudes rezultāts

- Pirms izmaiņas datubāzē apstiprināts tieši viens BattlePay produkts un veikala ieraksts, abi ID 109, ar nemainītu `battlepay_service_level90` piesaisti.
- Release `worldserver` pilnībā pārbūvēts un uzinstalēts bez kompilācijas kļūdām.
- Pilns `worldserver` starts pabeigts 12 sekundēs; nepiesaistītā `battlepay_service_level100` kļūda pazuda un `DBErrors.log` skaits samazinājās no 277 uz 276. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Atvērt BattlePay veikala “Services” grupu un pārbaudīt, ka tajā joprojām redzams vienīgais produkts “Level 90 Character Boost”, nevis neesošs level-100 produkts.
- Ar testa kontu un pietiekamu tokenu atlikumu nopirkt produktu ID 109 tēlam zem 90. līmeņa: tam jāsasniedz 90. līmenis, jāsaņem paredzētā nauda un ekipējums (vai pilnas somas gadījumā ekipējuma vēstule). Pirkums tēlam 90. līmenī vai augstāk jāatsaka.

## Pakete 119 — spell script rank piesaistes

Fails: `sql/updates/world/2026_09_18_106_fix_spell_script_rank_bindings.sql`.

`spell_script_names` negatīvs spell ID nozīmē “piesaistīt visiem rankiem” un ir derīgs tikai rank ķēdes pirmajam spell. Trīs ieraksti neatbilda 7.3.5 klienta datiem: Mother’s Embrace (219045) ir nerankots un tam jau bija pareiza pozitīva rinda līdzās liekai negatīvai kopijai; Ardent Defender (31850) ir nerankots, tāpēc tā `-31850` mainīts uz `31850`; Teleport: Moonglade (18960) ir rankots, tāpēc pozitīvā piesaiste mainīta uz `-18960`, aptverot visu ķēdi. Visas četras sākotnējās rindas saglabātas `_backup_20260918_spell_script_rank_bindings`.

### Pārbaudes rezultāts

- Aktīvajā tabulā palika pa vienai nepārprotamai piesaistei: `219045` Mother’s Embrace, `31850` Ardent Defender un `-18960` Teleport: Moonglade; backup tabulā ir visas četras sākotnējās rindas.
- Pilns `worldserver` starts pabeigts 12 sekundēs; visas trīs rank validācijas kļūdas pazuda un `DBErrors.log` skaits samazinājās no 276 uz 273. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Suramar saturā uzlikt Mother’s Embrace (219045) dzīvam mērķim un ļaut aurai beigties vai to noņemt nevis ar pretinieka dispel: dzīvam caster un mērķim jāaktivizējas 219068. Pretinieka dispel gadījumā papildu efektam nav jānotiek.
- Protection Paladin tēlam lietot Ardent Defender (31850): parastiem trāpījumiem jāsamazina bojājums par spell norādīto procentu, bet pirmajam nāvējošajam trāpījumam jāaktivizē dzīvības glābšanas heal; atkārtota glābšana 120 sekunžu cooldown laikā nedrīkst notikt.
- Ar druīdu pārbaudīt visus pieejamos Teleport: Moonglade (18960) rankus: pirmajai lietošanai jāsaglabā iepriekšējā vieta un jāpārved uz Moonglade `(7964, -2491, 488)`, bet atkārtotai lietošanai Moonglade 100 jardu robežās jāatgriež saglabātajā recall vietā.

## Pakete 120 — Twilight Correspondence loot nosacījums

Fails: `sql/updates/world/2026_09_18_107_fix_twilight_correspondence_loot_condition.sql`.

Twilight Correspondence (35277) quest “Unusual Activity” (11886) nosacījums bija piesaistīts `WORLD_LOOT_TEMPLATE` grupai 1, kas nav eksistējusi ne 2020., ne 2024. gada LegionCore 7.3.5 datubāzē. Žurnāla teksts kļūdaini nosauc `spell_loot_template`, taču kodā šis pats paziņojums tiek izmantots world-loot avotam. Pats items nav pazudis: tas ar 100% iespēju atrodas trīs konkrētās loot tabulās — Twilight Firesworn (25863), Twilight Flameguard (25866) un Twilight Speaker Viktor (25924). Quest prasība pārvietota no neesošās globālās grupas uz šīm trim īstajām creature-loot rindām. Sākotnējais nosacījums saglabāts `_backup_20260918_twilight_correspondence_condition`.

### Pārbaudes rezultāts

- Backup tabulā ir viena sākotnējā world-loot rinda; aktīvajā tabulā ir trīs `CREATURE_LOOT_TEMPLATE` nosacījumi ar item 35277 un quest 11886, pa vienam katram faktiskajam loot entry.
- Pilns `worldserver` starts pabeigts 12 sekundēs; neesošās loot grupas kļūda pazuda un `DBErrors.log` skaits samazinājās no 273 uz 272. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Midsummer notikuma laikā bez aktīva quest 11886 nogalināt Twilight Firesworn (25863), Twilight Flameguard (25866) un Twilight Speaker Viktor (25924) Ashenvale: Twilight Correspondence (35277) nedrīkst būt pieejams lootā.
- Pieņemt “Unusual Activity” (11886) un atkārtot testu ar visiem trim NPC; itemam 35277 jāparādās to lootā un jāļauj turpināt quest. Pēc quest pabeigšanas vai atmešanas items vairs nedrīkst krist.

## Pakete 121 — battleground skriptu reģistrācija

Fails: `src/server/scripts/World/world_script_loader.cpp`.

Datubāze korekti atsaucās uz desmit Seething Shore, Temple of Kotmogu, Shado-Pan Showdown un Warsong Gulch skriptiem. Visas desmit implementācijas un to četras `AddSC_battleground_*` reģistrācijas funkcijas jau bija kodola `game` modulī, bet starta skriptu ielādētājs nevienu no šīm funkcijām neizsauca. Tāpēc neko no datubāzes neizņēmu: `AddWorldScripts()` tagad izsauc visas četras esošās reģistrācijas funkcijas, padarot pieejamas trīs Seething Shore spell/aura implementācijas, trīs Seething Shore NPC skriptus, Kotmogu orb piespēli, Shado-Pan bosu un abus Warsong spell skriptus.

### Pārbaudes rezultāts

- Release `worldserver` pilnībā pārbūvēts un uzinstalēts bez kompilācijas kļūdām.
- Pilns `worldserver` starts pabeigts 12 sekundēs; visas desmit “referenced by the database, but does not exist in the core” kļūdas pazuda un `DBErrors.log` skaits samazinājās no 272 uz 262. `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Seething Shore: sākt battleground, pārbaudīt kapteiņu un vizuālo NPC darbību, buff kastes, Azerite mezglu aktivizēšanu un abu raķetes izpletņa auru korektu darbību pēc izlēkšanas no kuģa.
- Temple of Kotmogu “Pass the Orb” brawl režīmā paņemt orbu un lietot orb piespēles spēju uz derīgu komandas biedru; orbam jāpāriet mērķim, bet nederīgam, pārāk tālam vai orbu jau nesošam mērķim cast jāatsaka.
- Shado-Pan Showdown brawl režīmā pārbaudīt, ka abu komandu bosi iesaistās kaujā, lieto Brutal Slash, Firestorm/Thunderstorm un zem 50% dzīvības lieto heal.
- Warsong Gulch pārbaudīt Gripping Chain trāpījumu un Discombobulator auras uzlikšanu: pēdējai mērķis jānomontē un jānoņem mounted aura.

## Pakete 122 — spell implicit-target maskas un alternatīvie mērķi

Fails: `sql/updates/world/2026_09_18_108_fix_spell_implicit_target_conditions.sql`.

Sešu spellu `SPELL_IMPLICIT_TARGET` nosacījumos bija sajauktas efekta bitmaskas ar efekta numuriem. Spell 181293 trešā mērķa nosacījumam maska `3` (efekti 0+1) izlabota uz `4` (efekts 2). Spell 43178, 62092 un 85478 creature filtri no pārklājošās maskas `7` pārcelti uz efektu 0 (`SourceGroup=1`), un katrs atļautais NPC ievietots savā `ElseGroup`, lai tie būtu OR alternatīvas, nevis neizpildāma prasība vienam mērķim vienlaikus būt vairākiem dažādiem creature entry. Spell 45323 un 47374 pareizās maskas-1 rindas jau eksistēja; to vecās maskas-7 kopijas pēc arhivēšanas izņemtas kā nederīgi dublikāti. Visas 22 sākotnējās rindas saglabātas `_backup_20260918_spell_implicit_target_conditions`.

### Pārbaudes rezultāts

- Aktīvajās rindās spell 181293 tagad ir atsevišķas maskas `1`, `2`, `4`; pārējiem pieciem spell ir tikai creature mērķim derīgā maska `1`, bet alternatīvie mērķi sadalīti secīgos `ElseGroup`.
- Pilns `worldserver` starts pabeigts 12 sekundēs; pazuda visas 12 pārklājošo masku un 12 tām sekojošās “Not handled grouped condition” kļūdas. `DBErrors.log` skaits samazinājās no 262 uz 238, bet `Server.log` palika 116 iepriekš zināmās kļūdas.
- Serveris pēc pārbaudes korekti apturēts ar `server shutdown 1`.

### Spēlē vēlāk pārbaudāmais

- Lietot Plant Forsaken Banner (43178) uz Ulf the Bloodletter (24016), Oric the Baleful (24161) un Gunnar Thorvardsson (24162): visiem trim derīgajiem mērķiem jāpiešķir attiecīgais credit; uz citiem NPC lietošana jāatsaka.
- Lietot spell 45323 uz Fengir the Disgraced (24874), Windan of the Kvaldir (24875), Rodin the Reckless (24876) un Isuldof Iceheart (24877), kā arī spell 47374 uz The Focus on the Beach Kill Credit Bunny (26773): katrai derīgajai alternatīvai jāiedarbina tās quest credit/skripts, bet citi mērķi jāatfiltrē.
- Lietot Blessed Herb Bundle (62092) uz Maddened Blackwood (33043) un Corrupted Blackwood (33044): abiem jāpieņem spell hit un jāturpina jau salabotā attīrīšanas ķēde; citu creature nedrīkst izvēlēties par derīgu mērķi.
- Quest “Call in the Artillery” laikā lietot spell 85478 uz visiem četriem ēku triggeriem 45862–45865: katrai ēkai jāpiešķir savs credit, un viens triggeris nedrīkst bloķēt pārējos kā AND prasība.
- Draenor saturā pie NPC 90435 palaist spell 181293 un pārbaudīt, ka tā trīs efekti izvēlas attiecīgi Runic Pool (90440), Swelling Pool (90439) un Undulating Pool (90441), nevis pārklāj pirmo divu efektu filtrus.
