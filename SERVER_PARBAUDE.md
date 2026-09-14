# LegionCore kompilēšanas un palaišanas pārbaude

Datums: 2026-09-08, Europe/Riga. Projekts: `C:/wamp64/www/LegionCore-7.3.5`.

## Rezultāts

- Windows x64 Release kompilēšana un uzstādīšana izdevās; rezultāts atrodas `compiles`.
- Abi serveri tika palaisti **bez `-c` un no projekta saknes**, nevis no izpildfailu mapes. Konfigurācijas tika atrastas automātiski blakus izpildfailiem.
- `bnetserver` sasniedza `ready` 05:23:27; `worldserver` — 05:24:02. Pasaules inicializācija aizņēma 30 sekundes.
- Klausījās `127.0.0.1:1119`, `:8081`, `:8085`, `:8086`.
- `GET https://127.0.0.1:8081/bnetserver/login/` atgrieza **HTTP 200** un `LOGIN_FORM` JSON. Lokālā testa pašparakstītajam sertifikātam izmantots `curl -k`.
- Pēc aptuveni 3 minūtēm gatavā stāvoklī `worldserver` tika izslēgts ar `server shutdown 180` un beidzās ar **exit code 0** 05:27:04.
- `bnetserver` līdz testa beigām darbojās; pēc tam testa vadības skripts to apzināti terminēja. Tā exit code `-1` šajā testā nav patvaļīga avārija.
- Pēc pārbaudes abi procesi ir apturēti un minētie porti vairs neklausās.
- **Avārija vai FATAL ieraksts šajā pārbaudē netika novērots.** Tas ir palaišanas un īsas dīkstāves tests; spēlētāja pieslēgšanās, cīņas, uzdevumi un visu karšu apmeklēšana netika pārbaudīti.

## Konfigurācija un automātiskā kompilēšana

`compiles/worldserver.conf` un `compiles/bnetserver.conf` satur atsevišķā vietējā DB lietotāja `legion` paroli. Parole šajā pārskatā nav dublēta. Lietotājam ir piekļuve tikai četrām `legion_` datubāzēm:

| Loma | Datubāze |
|---|---|
| Autentifikācija | legion_auth |
| Personāži | legion_characters |
| Hotfixes | legion_hotfixes |
| Pasaule | legion_world |

Savienojums: `127.0.0.1:3307`, MariaDB 11.4.9; kompilēšanai izmantots MySQL 5.7.44 klienta SDK. `DataDir` ir absolūtais `compiles` ceļš; žurnāli ir `compiles/logs`. `SourceDirectory` norāda projekta sakni, un DB atjauninājumu klients ir atrasts WAMP MariaDB mapē. Automātiska tukšas datubāzes izveide ir atslēgta; pasaules servera SQL atjauninājumu pārbaude saglabāta ieslēgta. Serveri pašlaik ir piesaistīti tikai lokālajai adresei.

Nākamajai kompilēšanai palaist **`build.cmd`**. Tas pats atrod Visual Studio komplektā esošo CMake, izmanto `default` preset, kompilē Release un uzstāda failus `compiles`. Esošie `.conf` faili netiek pārrakstīti; instalācija atjauno `.conf.dist` paraugus. Datubāzes lietotājs un vietējie `.conf` jau ir sagatavoti šajā datorā; pilnīgi jaunā datorā tikai pirmkoda klonēšana tos neizveidos.

CMake automātiskā atkarību noteikšana pārbaudīta arī jaunā `build-autodetect-check` mapē, atslēdzot `BOOST_ROOT`, `OPENSSL_ROOT_DIR` un `MYSQL_ROOT_DIR` vides mainīgos. Atrasti Boost 1.85.0, OpenSSL-Win64 un MySQL 5.7.44. Instalācijas noklusējums: projekta `compiles`. Kopā ar serveri uzstādās MySQL/OpenSSL DLL un OpenSSL legacy modulis. Lietotāja skaidri norādītie CMake ceļi joprojām ir prioritāri.

Palaišanai vispirms atvērt `compiles/bnetserver.exe`, pēc tam `compiles/worldserver.exe`. Konfigurācijas automātiski tiek meklētas blakus izpildfailam, ja pašreizējā mapē noklusējuma faila nav; skaidri norādīts `-c` ceļš saglabā savu nozīmi. Bnet sertifikātu ceļi tiek rēķināti attiecībā pret konfigurācijas mapi.

## Palikušās kļūdas

Atrasti **883 ERROR ieraksti**: 767 `sql.sql`, 115 `scripts`, 1 `gameevent`. Bnet kļūdu nav. Skaits attiecas uz žurnāla ierakstiem, nevis 883 unikālām programmas kļūdām; viena spējas neatbilstība var radīt vairākus paziņojumus. Satura problēmas šajā darbā ir klasificētas, nevis masveidā labotas vai paslēptas.

P2 — funkcionāla satura problēma, kuru pārbaudīt pirms attiecīgā satura izmantošanas. P3 — zemāka prioritāte, pārsvarā attēlošana vai diagnostika. Palaišanu bloķējošs P0/P1 defekts šajā testā nav apstiprināts.

| Prioritāte | Kategorija | Ieraksti |
|---|---|---:|
| P2 | LFG ieejas un teleporti | 187 |
| P2 | Uzdevumu dati | 165 |
| P2 | Nosacījumi | 123 |
| P2 | SmartAI darbības | 122 |
| P2 | Spēju skriptu un DB2 efektu neatbilstības | 108 |
| P2 | NPC, modeļi un objekti | 34 |
| P2 | Laupījuma tabulas | 27 |
| P2 | Spēju SQL iestatījumi | 23 |
| P2 | NPC dialogu izvēlnes | 20 |
| P2 | Skriptu reģistrācija | 16 |
| P2 | NPC kustības ceļi | 15 |
| P2 | Spēju skriptu validācija | 7 |
| P2 | Objektu un NPC grupu izloze | 6 |
| P2 | Arēnas sezonas stāvoklis | 1 |
| P3 | Teksti un lokalizācijas sasaistes | 28 |
| P3 | Notikuma teksta diapazons | 1 |

### P2: LFG ieejas un teleporti — 187

Trūkst ieejas koordinātu vai areatrigger sasaistes. Attiecīgās instances automātiskais LFG teleports var nedarboties.

Ieteicamais labojums: Salīdzināt lfg_entrances, instance entrance un areatrigger ierakstus ar katras instances īsto ieeju; īpaši pārbaudīt Hellfire Citadel un Antorus.

Piemērs: `DBErrors.log:187`:

```text
table `lfg_entrances` contains coordinates for wrong dungeon 852
```

### P2: Uzdevumu dati — 165

Uzdevumos ir nederīgi rasu ierobežojumi vai neatbilstoši izpildes karodziņi. Daļu vērtību serveris koriģē atmiņā; tas nenozīmē, ka SQL dati ir salaboti.

Ieteicamais labojums: Pārbaudīt quest_template un quest_start_scripts ierakstus pa uzdevumu ID, īpaši 60028 un 14395; salāgot atļautās rases un mērķus.

Piemērs: `DBErrors.log:50`:

```text
LoadQuests() >> Quest 60028 does not contain any playable races in `AllowableRaces` (0), value set to 0 (all races).
```

### P2: Nosacījumi — 123

Nederīgi nosacījumu tipi, atsauces, avota grupas vai mērķi tiek ignorēti. Tas var mainīt atlīdzību, uzdevumu un objektu pieejamību.

Ieteicamais labojums: Pārbaudīt condition tabulas SourceType/SourceGroup/SourceEntry un saistītos objektus, nevis masveidā dzēst nosacījumus.

Piemērs: `DBErrors.log:428`:

```text
SourceGroup 3640 in `condition` table, does not exist in `gameobject_loot_template`, ignoring.
```

### P2: SmartAI darbības — 122

Daļa AI skriptu netiek ielādēta, jo tie atsaucas uz neesošiem NPC/GUID, nederīgām darbībām vai veciem karodziņiem.

Ieteicamais labojums: Salāgot smart_scripts ierakstus ar creature/creature_template un aktuālajām SmartAI enum vērtībām.

Piemērs: `DBErrors.log:646`:

```text
SmartAIMgr::LoadSmartAIFromDB: Creature guid (25354082) does not exist, skipped loading.
```

### P2: Spēju skriptu un DB2 efektu neatbilstības — 108

108 paziņojumi norāda, ka konkrēts skripta apstrādātājs neatbilst DB2 efektam un netiks izpildīts. Tas nav pierādījums bojātiem izvilktajiem failiem: klients ir pareizā būvējumā, bet skriptu sasaistes jāpielāgo.

Ieteicamais labojums: Pārbaudīt katra norādītā spell ID EffectIndex, AuraName un skripta reģistrēto hook; sākt ar spell_dru_incarnation un spell_gen_clone_weapon_aura.

Piemērs: `Server.log:481`:

```text
Spell `33891` Effect `Index: EFFECT_0 AuraName: 332` of script `spell_dru_incarnation` did not match dbc effect data - handler bound to hook `AfterEffectApply` of AuraScript won't be executed
```

### P2: NPC, modeļi un objekti — 34

Ir nederīgas dzimuma, ātruma, objekta tipa vai aksesuāra vērtības. Daļa tiek aizvietota ar noklusējumiem vai izlaista.

Ieteicamais labojums: Pārbaudīt norādītos creature_model_info, creature_template, gameobject_template un vehicle_template_accessory ierakstus.

Piemērs: `DBErrors.log:11`:

```text
Table `creature_model_info` has wrong gender (4294967295) for display id (20688).
```

### P2: Laupījuma tabulas — 27

Grupu kopējā varbūtība pārsniedz 100%, ir nederīgas valūtas vai neesošas atsauces. Piemēram, creature 71515 grupā norādīti 300%, reference 228138 — 1200%.

Ieteicamais labojums: Pārbaudīt grupu modeli un varbūtības; nedalīt procentus automātiski bez konkrētā laupījuma paredzētās loģikas pārbaudes.

Piemērs: `DBErrors.log:374`:

```text
Table 'creature_loot_template' entry 71515 group 4 has total chance > 100% (300.000000)
```

### P2: Spēju SQL iestatījumi — 23

Trūkst spell_proc phase mask un skill_discovery datu; daži proc efekti tādēļ netiks aktivizēti.

Ieteicamais labojums: Pārbaudīt spell_proc, skill_discovery_template un spell_scripts atbilstoši norādītajiem spēju ID.

Piemērs: `DBErrors.log:1`:

```text
`spell_proc` table entry for spellId 164545 doesn't have `spellPhaseMask` value defined, but it's required for defined `typeMask` value, proc will not be triggered
```

### P2: NPC dialogu izvēlnes — 20

Izvēlnes atsaucas uz neesošām opcijām vai POI. Attiecīgās dialoga funkcijas var nebūt pieejamas.

Ieteicamais labojums: Salāgot gossip_menu_option, izvēlņu ID un ActionPoiID ar points_of_interest datiem.

Piemērs: `DBErrors.log:413`:

```text
Table `gossip_menu_option` for menu 19634, id 0 use non-existing ActionPoiID 19637, ignoring
```

### P2: Skriptu reģistrācija — 16

Datubāzē norādīts skripts, kura kodolā nav, vai kodolā esošam skriptam nav sasaistes. Ir arī nepareiza spējas ranga atsauce.

Ieteicamais labojums: Pārbaudīt ScriptName un spell_script_names; piemērs: npc_bgss_visual_npcs. Jāizlemj, vai trūkst koda, vai SQL atsauce ir novecojusi.

Piemērs: `DBErrors.log:605`:

```text
Scriptname: `spell_mothers_embrace` spell (Id: -219045) has no ranks of spell.
```

### P2: NPC kustības ceļi — 15

NPC norādīta kustība pa ceļu, bet ceļš nav piešķirts, vai waypoint move_type ir nederīgs.

Ieteicamais labojums: Pārbaudīt creature GUID, creature_addon path_id un waypoint_data kustības tipus.

Piemērs: `DBErrors.log:37`:

```text
Creature (GUID 264517) has movement type set to WAYPOINT_MOTION_TYPE but no path assigned
```

### P2: Spēju skriptu validācija — 7

7 skripti neiziet Validate() pārbaudi un netiek piesaistīti attiecīgajai spējai.

Ieteicamais labojums: Pārbaudīt Validate() prasīto spēju esamību un efektus; piemērs: spell_magic_broom pie spējas 47977.

Piemērs: `Server.log:493`:

```text
Spell `47977` did not pass Validate() function of script `spell_magic_broom` - script will be not added to the spell
```

### P2: Objektu un NPC grupu izloze — 6

Ir neesoši pakārtoto pool ID un nepareizas iespēju summas. Tas var radīt nepareizu objektu/NPC parādīšanos.

Ieteicamais labojums: Pārbaudīt pool_pool un pool_template hierarhiju un varbūtības, īpaši 5132 un 10150.

Piemērs: `DBErrors.log:180`:

```text
`pool_pool` included pool_id (5132) is out of range compared to max pool id in `pool_template`, skipped.
```

### P2: Arēnas sezonas stāvoklis — 1

StartArenaSeason saņem sezonu 255, kurai nav atbilstoša ieraksta. game_event_arena_seasons šajā datubāzē satur tikai sezonas 3–8; noklusējuma konfigurācija norāda 14. Arēnas sezonas notikuma sākšana tiek pārtraukta.

Ieteicamais labojums: Jāsaskaņo WorldState 3901 inicializācija, Arena.ArenaSeason.ID un game_event_arena_seasons dati. Nejauša vecākas sezonas izvēle nav korekts Legion satura labojums.

Piemērs: `Server.log:656`:

```text
ArenaSeason (255) must be an existant Arena Season
```

### P3: Teksti un lokalizācijas sasaistes — 28

Dublēti teksti, neatļauti čata tipi, neatbilstoši BroadcastTextID un rezervēti teksta diapazoni. Serveris var izlaist konkrētus NPC tekstus.

Ieteicamais labojums: Salāgot script_texts, creature_text/creature_texts un db_script_string identifikatorus un to diapazonus.

Piemērs: `DBErrors.log:18`:

```text
CreatureTextMgr:  Entry 23141, Group 0 in table `creature_texts` has Type 100 but this Chat Type does not exist.
```

### P3: Notikuma teksta diapazons — 1

event_scripts ierakstam 13021 norādīts teksta ID 17912 ārpus šīs komandas sagaidītā diapazona.

Ieteicamais labojums: Pārbaudīt SCRIPT_COMMAND_TALK teksta atsauci un tai paredzēto tabulu.

Piemērs: `DBErrors.log:602`:

```text
Table `event_scripts` has out of range text id (dataint = 17912 expected 2000000000-2000010000) in SCRIPT_COMMAND_TALK for script id 13021
```

## Kompilatora brīdinājumi

Pirmajā pilnajā abu serveru būvējumā bija 264 brīdinājumu ieraksti un 0 kompilēšanas/linkošanas kļūdu. Otrā noklusējuma darbplūsmas būvēšana arī pabeigta ar kodu 0. Skaits zemāk attiecas uz `server-build.log`, nevis abu būvējumu summu.

| Kods | Skaits | Nozīme / rīcība |
|---|---:|---|
| C4305 | 121 | Skaitļa pārveidošana uz bool; pārbaudīt izsaukumu argumentus, lai nezaudētu paredzēto vērtību. |
| C5055 | 95 | Novecojusi enum un peldošā komata aritmētika; izmantot apzinātu tipa pārveidi. |
| C4018 | 36 | Signed/unsigned salīdzinājumi; pārbaudīt robežas un negatīvu vērtību apstrādi. |
| C4477 | 5 | `PlayerDump.cpp` izmanto `%lu` 64 bitu GUID/vērtībām. Prioritāri pārbaudīt eksportēto personāžu dump ID korektumu. |
| C4146 | 2 | Mīnuss unsigned vērtībai; pārbaudīt paredzēto matemātiku Player.cpp un BattlePayHandler.cpp. |
| C4101 | 1 | Neizmantots mainīgais DBUpdater.cpp. |
| C4309 | 1 | Konstantes saīsināšana zone_zuldrak.cpp; pārbaudīt parametra tipu. |
| C4805 | 1 | Nedroša bool un cita skaitliska tipa kombinācija; pārskatīt nosacījumu. |
| C4834 | 1 | Ignorēts nodiscard rezultāts boss_morchok.cpp. |
| C4858 | 1 | boss_sha_of_fear.cpp:1734 izsauc `empty()` un ignorē rezultātu. Ja mērķis ir iztukšot kolekciju, jālieto `clear()`. |

Šie brīdinājumi nav šajā testā apstiprinātas avārijas; daļa norāda uz iespējamiem loģikas defektiem. Pilnas failu un rindu norādes saglabātas būvēšanas žurnālā.

CMake vēl izvada izstrādātāja brīdinājumu par CMP0144 un Boost ziņojumu par neatpazītu jaunā MSVC toolset versiju. Atrastās bibliotēkas veiksmīgi salinkojās, un abi serveri palaidās; tas nav atkarību atrašanas bloķētājs.

## Diagnostikas īpatnība

Bnet starta tekstā ir `http://127.0.0.1:8081`, taču LoginRESTService šim savienojumam izmanto TLS (`soap_ssl_accept`). HTTP mēģinājums tādēļ tiek pārtraukts; pareizais pārbaudes URL ir **HTTPS**, kas atgrieza 200. Šis maldinošais žurnāla teksts palicis pirmkodā un ir atsevišķs P3 diagnostikas labojums.

## Pierādījumi un atjaunošanas kopija

- `compiles/logs/Server.log`, `DBErrors.log`, `Bnet.log` — pilnie serveru žurnāli.
- `build-extractors/runtime-test/errors.csv` — visi 883 kļūdu ieraksti ar grupu, oriģinālo tekstu un avota rindas numuru.
- `build-extractors/runtime-test/result.json` — procesu izbeigšanās rezultāti.
- `build-extractors/runtime-test/rest-headers.txt`, `login-form.json` — veiksmīgā HTTPS pieprasījuma atbilde.
- `build-extractors/server-build.log`, `build-default.log` — būvējumu žurnāli.
- `build-extractors/autodetect-configure.log` — automātiskās atkarību noteikšanas pārbaude bez vides norādēm.
- `build-extractors/runtime-test/databases-before-test.sql` — četru `legion_` datubāžu kopija pirms servera palaišanas. Tā netika automātiski atjaunota; servera parastie stāvokļa ieraksti pēc testa ir saglabāti.

Citas iepriekš esošās spēļu datubāzes netika pārkonfigurētas vai pārrakstītas. Iegūtās klienta datu mapes ir saglabātas.