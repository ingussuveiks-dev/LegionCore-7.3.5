# Only the Worthy Shall Pass (29421)

Master Li Fei `54135` atrodas Cavern of the Burning Mind novietojumā `244088`. Pēc `29664` jau esošais pieņemšanas ceļš izsauc dueļa pretinieku `54734`. Izsaukums tagad pieder konkrētajam pieņemtajam kvesta mēģinājumam. Divi spēlētāji var sākt atsevišķus dueļus pie viena skolotāja, un atkārtots pieņemšanas izsaukums neveido otru tā paša spēlētāja pretinieku.

Padošanos pārbauda pēc bojājuma mērogošanas, arī vienam sitienam, kas pārvar 10 procentu dzīvības slieksni. Uzvaru var izraisīt spēlētājs vai viņa mājdzīvnieks. Padošanās, sveiciens un kredīts notiek vienreiz, pēc tam pretinieks tiek noņemts. Ja spēlētājs zaudē, neveiksme tiek atzīmēta tikai viņa duelim. Atmešana noņem savu pretinieku un ļauj sākt jaunu mēģinājumu. Kvestam nav priekšmeta, ko izņemt no somas.

Native `-LiFeiDuel` pārbaude ielasīja sākuma NPC, kavernas novietojumu, dialogu, abus pasaules piedāvājuma galus un īsto mērķi. Tā apstiprināja divus personīgus dueļus, atkārtotu pieņemšanas izsaukumu, cita spēlētāja sitiena noraidīšanu, atmešanu un atkārtotu pieņemšanu, mājdzīvnieka vienu uzvarošu sitienu, atļautu nodošanu un spēlētāja faktisku zaudējumu. Izgāja arī saglabātā `-MinerTroubles` pārbaude, kurā tiek noņemti personīgi eskorta ratiņi. Klientā vēl jāpārbauda kavernas ieeja, padošanās attēlojums, faktiskā nodošana un pāreja uz `29422`.

Gala Release SHA-256 `3188C77043CB43DEEF0A1EAF1234EC8C174DDCBF5078882834547941B2D27BE6`. Saišu kartē apstiprināts `scripts:zone_the_wandering_isle.obj` dueļa bojājuma apstrādātājs. Staged un kanoniskais serveris sasniedza `ready` izolētajā MariaDB bez jaunām avārijām vai neizskaidrotiem žurnāla ierakstiem. Labojums izmanto esošās SQL sasaistes.

Diagnostikas kopijas, kontrolsummas un pagaidu failu dzēšanas saraksts glabājas `build-extractors/bin/Release/fixed/li-fei-duel-2026-11-22/`. Atbrīvoti 1 001 418 600 baiti un saglabāta iepriekšējā Release atjaunošanas kopija.
