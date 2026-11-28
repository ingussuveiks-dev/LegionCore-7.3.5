# An Ocean Not So Deep Darkshore

Abām Greymist SmartAI rindām (`33262` un `33277`) bija atsauce uz neesošu `LINK 1`. Tās mēģināja ieskaitīt `62683` pēc jebkura `62682` trāpījuma. Kredīta burvestība izmanto vadītāju kā mērķi, bet robota tiešā vadība detonācijas beigās var jau būt pārtraukta.

Sākotnējā konsole `195006`, novietojums `37921`, jau izsauca `64924 → 64923`. Legion 26972 burvestība `64923` izmanto datubāzes galamērķi, transportlīdzekli `34207` un izsaukuma īpašības `1201`. Galamērķa rindas trūkuma dēļ serveris to novietoja pie ūdens virsmas. Sākotnējie dati to atkārtoja ar īsto konsoles klikšķi.

Pilnais 2016. gada janvāra ieraksts pārskatīts četru sekunžu kadros, bet konsoles un detonācijas posmi pārskatīti vienas līdz divu sekunžu kadros. Ierakstā redzama konsoles lietošana ap `1:20`, zemūdens vadība, murloku pulcēšanās, bumbas eksplozija un skaitītājs ap `1:54`, atgriešanās pie konsoles tūlīt pēc eksplozijas un kvesta nodošana ap `3:37`. Ieraksts ir no agrākas klienta versijas. Burvestību un transportlīdzekļa identitātes pārbaudītas projekta Legion 26972 datos. [Kvesta iziešanas ieraksts](https://www.youtube.com/watch?v=wfTohl5ZIKk)

## Labojums

- Trūkstošais `64923` punkts rekonstruēts blakus oriģinālajam Greymist Tidehunter novietojumam `121889`. Servera pārbaude apstiprina gan ūdeni, gan jūras gultni zem robota. Izsaukšana pieprasa dzīvu spēlētāju ar aktīvu `13560` pie īstās konsoles, saista robotu ar šo kvesta mēģinājumu pirms publicēšanas un saglabā standarta iekāpšanu.
- Iedarbināšanai jāsēž savā robotā. Atkārtota burvestība nevar pārstartēt ieslēgto laika atskaiti. Datu saite detonē pēc astoņu sekunžu auras dabiskām beigām. Saglabātas oriģinālās pulsa, bojājuma, izsēdināšanas saites un SmartAI paziņojumu un pazušanas secība.
- Jaunā `62682` apstrāde piešķir kredītu `32852` tikai par dzīva Tidehunter vai Warrior bojāeju šā trāpījuma rezultātā. Tā izmanto `62683` standarta grupas kredīta ceļu ar spēlētāju kā avotu. Robots pēc detonācijas atjauno vadību un nosūta faktisko vadītāju pie konsoles. Atgriešanās punkts rekonstruēts pie Gary novietojuma `122212`.

## Pārbaudes

- `Test-QuestObjectLootNative.ps1 -Greymist` ar sākotnējiem datiem atkārtoja robota parādīšanos pie virsmas. Privātajā MariaDB migrācija izpildīta divreiz. Labojuma pārbaudē izmantots sākotnējais konsoles novietojums un lietošanas pakete, standarta iekāpšana, kvestu skaitītāji un atgriešanās teleportācijas apstiprinājums. Pārbaudītas astoņu sekunžu atskaites beigas, atkārtota iedarbināšana, Tidehunter bojāeja, Warrior izdzīvošana, grupas biedrs un nesaistīts spēlētājs, atkārtota konsoles lietošana, atmešana, pieņemšana no jauna un Warrior dotā pēdējā 50. ieskaite.
- `-DarkshoreWildlifeFlight` atkārtoti pārbaudīja `13885` lidojumu, abas sēdvietas, nolaišanos, nodošanu un atkārtotu mēģinājumu.
- Divas agrīnas testa palaišanas avarēja testa vides inicializācijas dēļ. MariaDB tajos pašos laikos reģistrēja atvienotus testa klientus. Pārbaudes vide papildināta ar nepieciešamajiem modeļa, preču, lappušu, burvestību bojājuma un draudu datiem. Darbīgās pārbaudes beidzās ar kodu 0 un bez jauniem MariaDB brīdinājumiem, avārijām, `DBErrors` vai servera assert ierakstiem. Privātā MariaDB korekti izslēgta.
- Spēles klientā jāpārbauda kamera, vadības josla, Gary dialogs, taunta un skaņu attēlojums, dabiskā murloku pulcēšanās un pilns 50/50 iziešanas un nodošanas ceļš. Vēl jāizmēģina manuāla izkāpšana, pārsavienošanās un robota bojāeja kaujā.

## Izvietošana

- Gala saistītājs izmantoja atjaunināto `game.lib` ar SHA-256 `100E25C5635345DAAFBF73EADC8E22603F62157C47E34DF52713E2532102CFFD` un `scripts.lib` ar SHA-256 `E68BE503529CBDF79099C0062087D4D67EDA64C7B237831B0776323BDEF8110C`. Abas burvestību realizācijas ir atrodamas gala saistījuma kartē. Pārbaudītā un izvietotā `worldserver.exe` SHA-256 ir `3332DF0F2029F29890EC2EF745C6E443152CA790C0AEE013498AAF0FF3DDFB5B`.
- Kanoniskais `worldserver` sasniedza gatavību un korekti izslēdzās ar kodu 0. Dzīvajā datubāzē migrācijai ir statuss `RELEASED` un SHA-1 `8A45EDAA60A3747925BE26A24CE7F8B99151AAE4`. Pārbaudīts zemūdens galamērķis, abas C++ piesaistes, abas burvestību saites, 4 sākotnējās robota SmartAI rindas un 2 veco Greymist rindu noņemšana. Jaunu avāriju, DB kļūdu, brīdinājumu vai assert ierakstu nebija.
- Diagnostikas oriģināli, SHA-256 manifests un iepriekšējās versijas atgriešanas kopija saglabāti `build-extractors/bin/Release/fixed/2026-11-28-greymist-decoy/`. Dzēsts 41 atjaunojams pagaidu fails, kopā 1 298 605 998 baiti. Esošais jauktais `DBErrors.log` saglabāts 1347 baitu apjomā, un aktīvā avāriju mape ir tukša.
