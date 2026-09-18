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
