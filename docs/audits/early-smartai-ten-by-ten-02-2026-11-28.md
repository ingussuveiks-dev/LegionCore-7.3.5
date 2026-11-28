# 2. 10×10 SmartAI pāreju audits

2026. gada 28. novembrī pārbaudītas 100 unikālas SmartAI pārejas 35 agrīno zonu tēliem. No atlases izslēgti visi 32 tēli, kas aplūkoti pirmajā auditā. Atlasei atbilda 658 pārejas. Visām atrasts strukturāli derīgs turpinājums. Padziļināti pārbaudītas tabulās nosauktās 100 pārejas. [Pirmais audits](early-smartai-ten-by-ten-2026-11-28.md)

## Konstatētās problēmas

- **Runaway Shredder! (14129).** Tēla 35111 veselības notikums pie 0–10% pārslēdz to uz draudzīgu frakciju un pa 1 → 2 → 3 → 7 izsauc sagūstīšanas kredītu. ProcessAction saglabā agresijas izraisītāju, un SMART_TARGET_ACTION_INVOKER to izmanto, ja veselības taimeris nav saņēmis savu izraisītāju. Tādējādi kredīta ceļš var sasniegt spēlētāju pirms iekāpšanas. Klikšķim ir sava ķēde 4 → 5 ar burvestību 66778 un tēla pazušanu pēc vienas sekundes. Pirms remonta jāpārbauda faktiskais iekāpšanas vai burvestības saņemšanas apliecinājums. Jāpārbauda arī vairāku spēlētāju, mājdzīvnieka un neveiksmīga klikšķa gadījumi.
- **Fourth and Goal (24503 un 28414). Salabots.** Mērķim 37203 bija divi SPELLHIT 70052 notikumi ar ID 0. Pirmais derīgais trāpījums pēc atiestatīšanas divreiz izsauca CALL_KILLEDMONSTER 37203. Tagad katrs derīgais trāpījums izsauc kredītu vienreiz. Pirmais trāpījums atsevišķi izsauc to pašu ainas tēlu 423070. [Labojums un pārbaudes](#fourth-and-goal-labojums)
- **Bad Science! Bad! (14322).** Static-Charged Hippogryph 36147 pēc burvestības 8599 pie 0–30% veselības izsauc teksta grupu 0, kuras datubāzē nav.
- **The Terrible Tinkers of the Ruined Reaches (14383).** Zapper Gnome 36384 un Netgun Gnome 36385 pēc burvestībām 68664 un 68658 katrs izsauc savu teksta grupu 0. Trūkst abu grupu. Burvestības ir atrodamas 26972 DB2 datos.
- **Ace (38441).** Atlasītā pāreja 1 → 2 mēģina atskaņot teksta grupu 1. Trūkst arī pārējo septiņu grupu, uz kurām atsaucas tā paša tēla pilnā secība. Sākuma signālu DATA 1,1 dod tēls 38432, pieņemot kvestu 24868. Ace ir arī kvesta 25122 mērķis. Maršrutam 38441 ir 20 punkti. Serveris iestata dialoga taimeri pirms teksta meklēšanas un pēc termiņa izsauc TEXT_OVER arī bez teksta. Jāatgūst teksta rindas un jāpārbauda to parādīšanās kopā ar secības sākumu un beigām.

Četras trūkstošā teksta atsauces attiecas uz atlasītajām pārejām. Ace pilnajā secībā konstatētas vēl septiņas. Dialogu un pārējo kvestu problēmas paliek atvērtas remontam.

## Fourth and Goal labojums

Migrācija `2026_11_28_18_fourth_and_goal_credit.sql` atstāj atkārtojamo kredīta notikumu ar tā sākotnējo saņēmēju. Bijušais saites notikums kļūst par atsevišķu, vienreizēju reakciju uz `70052`. Tā saglabā izsaukšanas veidu, koordinātas un 300 sekunžu dzīves ilgumu. Mērķa īpašie notikuma nosacījumi vai atšķirīgas SmartAI rindas pasargā vietējo versiju no aizvietošanas.

`Test-FourthAndGoalCredit.ps1` pārbaudīja sākotnējās un labotās rindas pagaidu MariaDB tabulās. Atkārtota migrācija, cita avota rindas, vietējs laika parametrs un notikuma nosacījums deva paredzēto rezultātu. Pirmajā testa mēģinājumā MariaDB noraidīja pagaidu tabulu ar avota tabulas vārdu. Pēc atsevišķu shēmas tabulu ieviešanas pārbaude izdevās. Saglabāti abi testa rezultāti.

Kanoniskais worldserver ielādēja migrāciju ar statusu `RELEASED` un SHA-1 `BA64C8FBEAC69416FC4BD8687C66776FFF94090F`. Tas sasniedza gatavību un korekti izslēdzās ar kodu 0. Dzīvo datu pārbaude apstiprināja abas SmartAI rindas, abu kvestu `37203` mērķi ar nepieciešamo skaitu 1 un septiņus ainas maršruta punktus. Jaunu avāriju, kļūdu vai brīdinājumu nav. Sākotnējā testa diagnostika un veiksmīgās pārbaudes saglabātas vietējā `build-extractors/bin/Release/fixed/2026-11-28-fourth-and-goal/` ar pārbaudītām SHA-256 kontrolsummām. Pēc 26972 burvestību kataloga pārbaudes izdzēsti divi pagaidu būvējuma faili, atbrīvojot 256 619 baitus. Kanonisko bināro failu un konfigurācijas kontrolsummas pārbaudītas pēc tīrīšanas.

Spēles klientā vēl jāpārbauda pats Footbomb sitiens no transportlīdzekļa, ainas izskats, atkārtots trāpījums un abi kvesta varianti. Kredīta saņēmēju joprojām nosaka oriģinālā SmartAI darbība, kas izmanto spēlētāju vai atbilstošo izraisītāja īpašnieku un native grupas atbilstības pārbaudi.

## Papildu pārbaudes

- **Ice Cold (14261).** Burvestība 69230 ierobežota uz Sable Drake 36640. Darbību saraksts 3664000 vada septiņu sekunžu aizkavi līdz draka pašnāvei, kam seko atsevišķs kredīta izsaukums. Kvesta aprakstā prasīta sasaldēšana un nogalināšana. Jāpārbauda, kurš saņem aizkavēto kredītu pie atkārtota trāpījuma, nāves pirms taimera un divu spēlētāju darbībām.
- **The Proving Pit.** Izvēlnes 10974 opcija 1 dod sarunas mērķi 39062. Pēc četrām sekundēm atsevišķa darbība izsauc 38142. Visos deviņos dzīvajos kvesta variantos abi mērķi ir nodalīti.
- **Kezan Partygoer (35175).** Piecas 66909–66913 trāpījumu rindas pāriet uz kopīgo auru 66916. Trāpījumu nosacījumi nepieņem mērķi ar šo auru. Atsevišķais taimeris to noņem. Pārbaudīti abu kvestu 14113 un 14153 mērķu ieraksti.
- **Archaeologist Hollee (33232) un Lilian Voss (38910).** Pārbaudīti 3323200, 3891000, 3891001 darbību saraksti un trīs SmartAI maršruti ar 11, 8 un 9 punktiem.

## Pārbaudes tvērums

- Dzīvā MariaDB 11.4.9 tika lasīta read-only transakcijās. Atlasē iekļauti pozitīvi source_type=0 SmartAI šabloni ar minimālo līmeni līdz 20 un vismaz vienu novietojumu kartēs 0, 1, 654, 648, 860 vai 1116. Priekšroka dota tiešajiem kvestu mērķiem, ja kvesta minimālais un norādītais līmenis nepārsniedz 20. Iekļauti arī dinamiska līmeņa kvesti. Tabulā redzami kvesti, kuru mērķis ir norādītais tēls. Šablona līmenis pats par sevi nenosaka kvesta pieejamību.
- Saites pārbaudītas WORLD_SEL_SMART_SCRIPTS secībā un ar SmartScript::FindLinkedEvent pirmā atbilstošā notikuma principu. Izmantoti izlaides būves filtri un atklātās pasaules grūtība 0. Pārbaudītas arī 34 pārejas, kas sākas ķēdes vidū. Visām 36 fāžu ierobežotajām ķēdēm atrasta saderīga sākuma fāze. Faktiska šīs fāzes sasniegšana klientā paliek atsevišķa pārbaude.
- Atlasītajās ķēdēs un sasniedzamajos laika sarakstos pārbaudīti 182 atšķirīgi rindas un izpildītāja pāri, 31 burvestība pret 26972 DB2 katalogu, 26 tēlu atsauces, 22 kredīta mērķi, 5 darbību saraksti un 3 maršruti. No 12 pārbaudītajām dialoga grupām trūkst četru.
- LegionCore DB2 lasītājs pabeidza kataloga pārbaudi. Diagnostikā pirms un pēc pārbaudes nekonstatēja jaunus vai mainītus avāriju failus, jaunus servera brīdinājumus, kļūdas vai DBErrors.log papildinājumus. Pārbaude darbojās bez worldserver un bnetserver palaišanas. Spēles klientā kvesti netika izieti.

- Pagaidu kataloga EarlySpellCatalog.exe un EarlySpellCatalog.obj izdzēsti pēc pārbaudes. Atbrīvoti 228 577 baiti. Vietējie pārbaužu ieraksti un dzēšanas saraksts saglabāti ignorētajā .codex/ten-ten-02-20261128 mapē. Pirms un pēc tīrīšanas pārbaudīta kanoniskā worldserver.exe kontrolsumma.

## 100 pāreju saraksts

### Partija 1

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 33232 Archaeologist Hollee | 1:148 | 13605 | 1 → 2 | Saite derīga |
| 33767 Demon Gate Master Bunny | 1:331 | 13766 | 0 → 1 → 2 | Saite derīga |
| 33767 Demon Gate Master Bunny | 1:331 | 13766 | 1 → 2 | Saite derīga |
| 33913 Shatterspear Hut Fire Bunny | 1:148 | 13514 | 0 → 1 | Saite derīga |
| 34285 Trapped Wolf | 1:17 | 13878 | 0 → 1 → 2 → 3 | Saite derīga |
| 34285 Trapped Wolf | 1:17 | 13878 | 1 → 2 → 3 | Saite derīga |
| 34285 Trapped Wolf | 1:17 | 13878 | 2 → 3 | Saite derīga |
| 34316 Twilight Portal | 1:148 | 13588 | 0 → 1 → 2 | Saite derīga |
| 34316 Twilight Portal | 1:148 | 13588 | 1 → 2 | Saite derīga |
| 34503 Razormane Pillager | 1:17 | 13973 | 0 → 1 | Saite derīga |

### Partija 2

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 34782 Alicia Cuthbert | 1:17 | 14063 | 1 → 2 | Saite derīga |
| 34830 Defiant Troll | 648:4737 | 14069 | 0 → 1 → 2 → 3 | Saite derīga |
| 34830 Defiant Troll | 648:4737 | 14069 | 1 → 2 → 3 | Saite derīga |
| 34830 Defiant Troll | 648:4737 | 14069 | 2 → 3 | Saite derīga |
| 35111 Runaway Shredder | 1:16 | 14129 | 1 → 2 → 3 → 7 | Kredīta ceļš pirms klikšķa |
| 35111 Runaway Shredder | 1:16 | 14129 | 2 → 3 → 7 | Turpina 14129 kredīta ceļu pirms klikšķa |
| 35111 Runaway Shredder | 1:16 | 14129 | 3 → 7 | Turpina 14129 kredīta ceļu pirms klikšķa |
| 35111 Runaway Shredder | 1:16 | 14129 | 4 → 5 | Atsevišķa klikšķa ķēde |
| 35175 Kezan Partygoer | 648:4737 | 14113, 14153 | 0 → 6 | Saite derīga |
| 35175 Kezan Partygoer | 648:4737 | 14113, 14153 | 1 → 6 | Saite derīga |

### Partija 3

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 35175 Kezan Partygoer | 648:4737 | 14113, 14153 | 2 → 6 | Saite derīga |
| 35175 Kezan Partygoer | 648:4737 | 14113, 14153 | 3 → 6 | Saite derīga |
| 35175 Kezan Partygoer | 648:4737 | 14113, 14153 | 4 → 6 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 0 → 1 → 2 → 3 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 1 → 2 → 3 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 2 → 3 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 4 → 5 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 6 → 7 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 8 → 9 → 10 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 9 → 10 | Saite derīga |

### Partija 4

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 35177 Talrendis Raider | 1:16 | 14146 | 11 → 12 → 13 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 12 → 13 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 14 → 15 | Saite derīga |
| 35177 Talrendis Raider | 1:16 | 14146 | 18 → 19 | Saite derīga |
| 35526 Clubnik's Dozer | 1:16 | 14423 | 0 → 1 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 0 → 2 → 3 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 2 → 3 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 4 → 5 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 6 → 7 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 8 → 9 → 10 | Saite derīga |

### Partija 5

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 35880 Lady Silisthra | 1:16 | 14295 | 9 → 10 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 11 → 12 → 13 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 12 → 13 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 14 → 15 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 18 → 19 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 21 → 5 | Saite derīga |
| 35880 Lady Silisthra | 1:16 | 14295 | 23 → 5 | Saite derīga |
| 36025 Secret Lab Fires | 1:16 | 14308 | 0 → 1 | Saite derīga |
| 36061 Research Intern | 1:16 | 14308 | 1 → 2 → 3 → 4 | Saite derīga |
| 36061 Research Intern | 1:16 | 14308 | 2 → 3 → 4 | Saite derīga |

### Partija 6

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 36061 Research Intern | 1:16 | 14308 | 3 → 4 | Saite derīga |
| 36147 Static-Charged Hippogryph | 1:16 | 14322 | 3 → 4 | Trūkst teksta grupas 0 |
| 36384 Zapper Gnome | 1:16 | 14383 | 0 → 1 | Trūkst teksta grupas 0 |
| 36385 Netgun Gnome | 1:16 | 14383 | 0 → 1 | Trūkst teksta grupas 0 |
| 36640 Sable Drake | 1:16 | 14261 | 1 → 2 → 3 | Kredīta saņēmējs jāpārbauda pēc aizkaves |
| 36640 Sable Drake | 1:16 | 14261 | 2 → 3 | Turpina aizkavēto draka ķēdi |
| 36722 Bilgewater Laborer | 1:16 | 14297 | 0 → 1 → 2 | Saite derīga |
| 36722 Bilgewater Laborer | 1:16 | 14297 | 1 → 2 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 0 → 1 → 2 → 3 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 1 → 2 → 3 | Saite derīga |

### Partija 7

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 2 → 3 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 4 → 5 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 6 → 7 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 8 → 9 → 10 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 9 → 10 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 11 → 12 → 13 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 12 → 13 | Saite derīga |
| 36914 Talrendis Sentinel | 1:16 | 24433 | 14 → 15 | Saite derīga |
| 36922 Wounded Soldier | 1:16 | 24436 | 0 → 1 | Saite derīga |
| 37114 Steamwheedle Shark | 648:4737 | 24502 | 0 → 1 | Saite derīga |

### Partija 8

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 37203 Fourth And Goal Target | 648:4737 | 24503, 28414 | 0 → 1 | Salabota. Kredīts un ainas izsaukšana ir atsevišķi trāpījuma notikumi. |
| 38035 Chemical Wagon | 0:10,1:14,1:15,1:5034 | 24638, 24645, 24658, 24659, 44546, 44558 | 0 → 1 → 2 | Saite derīga |
| 38035 Chemical Wagon | 0:10,1:14,1:15,1:5034 | 24638, 24645, 24658, 24659, 44546, 44558 | 1 → 2 | Saite derīga |
| 38306 Zar'jira | 1:6453 | 24814 | 0 → 1 | Saite derīga |
| 38441 Ace | 648:4720 | 25122 | 1 → 2 | Trūkst teksta grupas 1 |
| 38560 Spitescale Flag Bunny | 1:6453 | 24813 | 0 → 1 | Saite derīga |
| 38895 Lilian Voss | 0:6454 | 24960 | 0 → 1 | Saite derīga |
| 38910 Lilian Voss | 0:6454 | 24961 | 1 → 2 → 5 | Saite derīga |
| 38910 Lilian Voss | 0:6454 | 24961 | 2 → 5 | Saite derīga |
| 38910 Lilian Voss | 0:6454 | 24961 | 3 → 4 → 5 | Saite derīga |

### Partija 9

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 38910 Lilian Voss | 0:6454 | 24961 | 4 → 5 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 0 → 1 → 2 → 3 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 1 → 2 → 3 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 2 → 3 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 8 → 9 → 10 → 11 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 9 → 10 → 11 | Saite derīga |
| 38999 Lilian Voss | 0:85 | 24979 | 10 → 11 | Saite derīga |
| 39062 Darkspear Jailor | 1:6453 | 24642, 24754, 24762, 24768, 24774, 24780, 24786, 26276, 31161 | 0 → 1 → 6 | Saite derīga |
| 39062 Darkspear Jailor | 1:6453 | 24642, 24754, 24762, 24768, 24774, 24780, 24786, 26276, 31161 | 1 → 6 | Saite derīga |
| 39157 Lost Bloodtalon Hatchling | 1:6453 | 24623 | 0 → 1 | Saite derīga |

### Partija 10

| Tēls | Karte un zona | Tiešie kvestu mērķi | Notikumu ID | Rezultāts |
| --- | --- | --- | --- | --- |
| 39251 Northwatch Supply Crate | 1:14 | 25167 | 0 → 1 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 0 → 2 → 3 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 2 → 3 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 4 → 5 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 6 → 7 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 8 → 9 → 10 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 9 → 10 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 11 → 12 → 13 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 12 → 13 | Saite derīga |
| 39268 Northwatch Sharpshooter | 1:14 | 25173 | 14 → 15 | Saite derīga |
