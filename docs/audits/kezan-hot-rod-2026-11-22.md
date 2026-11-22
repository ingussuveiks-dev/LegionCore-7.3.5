# Rolling with my Homies `14071`

Megs Dreadshredder `34874/189554` dod atslēgu `46856` pēc Megs in Marketing `28349`. Hot Rod `34840` izmanto transportlīdzekļa ierakstu `448`. Atslēgas burvestība `66393` izsauc auto un izmanto `66392`, lai iesēdinātu īpašnieku. Iepriekš atslēgas ieskaite radās pašā burvestības izpildē, un pasažiera skripts deva ieskaiti vēl pirms iekāpšanas. Pārbaudē ar iepriekšējo kodu `9bd5ecdce` Gobber ieskaiti varēja saņemt, atrodoties citā tā paša modeļa transportlīdzeklī.

| Aktieris un novietojums | Izsaukums | Pasažieris | Sēdeklis |
| --- | --- | --- | --- |
| Ace `34892/240082` | `66597` | `34957` | 1 |
| Gobber `34954/240081` | `66599` | `34958` | 2 |
| Izzy `34890/240080` | `66600` | `34959` | 3 |

Vadītāja ieskaite `48323` seko īstajai iekāpšanai sēdeklī `0` spēlētāja paša Hot Rod. Uzņemšana sākas pie attiecīgā pasaulē novietotā aktiera, piesaista jaunā pasažiera pilno GUID aktīvajam mēģinājumam un ieskaita tikai viņa faktisko iekāpšanu paredzētajā sēdeklī. Izsauktais mazais pavadonis pēc izveides izpilda sākotnējo `EnterEvadeMode`, kas notīra rindā gaidošo iekāpšanu. Tāpēc iekāpšana sākas nākamajā aktiera atjauninājumā un vēlreiz pārbauda vadītāju, kvesta mēģinājumu un brīvu sēdekli. Neizdevies mēģinājums ļauj atkārtoti piebraukt pie īstā aktiera.

Migrācija `2026_11_22_57_hot_rod_boarding.sql` saglabā visas deviņas esošās Hot Rod SmartAI rindas. `quest_template_addon` atslēgas saglabāšanas karogs attiecas tikai uz `14071` un tā esošo sākuma priekšmetu `46856`. Atslēgu pēc atlīdzības joprojām var izmantot Kezanā. Datu izmaiņa pārbaudīta divos secīgos izolētās MariaDB importos. [Atslēgas izmantošana Kezanā](https://warcraft.wiki.gg/wiki/Keys_to_the_Hot_Rod)

`Test-QuestObjectLootNative.ps1 -KezanHotRod` pārbaudīja pieņemšanu pie novietotās Megs ar priekšnosacījumu, parastu atslēgas burvestību, neatbilstošu transportlīdzekli, neatļautu pasažiera burvestību īstajā auto, visus trīs pasaulē novietotos aktierus, reālos sēdekļus, spēles atlīdzības paketi `571` un atkārtotu braucienu ar saglabāto atslēgu pēc nodošanas. Zonde pārvietoja auto starp esošajiem aktieriem un apstrādāja spēles servera laika notikumus, tādēļ klienta vadība, radio, animācijas un pilnā braukšanas secība paliek pārbaudāmas klientā. 107 sekunžu 2015. gada spēles ieraksts pārskatīts ar vizuāliem kadriem ik pēc astoņām sekundēm no brauciena sākuma līdz nodošanai. [2015. gada gājiens](https://www.youtube.com/watch?v=QyALeDsnkvc)

Agrīnajā zondē trūka īsto burvestību aktivizācijas tabulu. Tās ielādēja un atkārtoja zondi. Izolētās MariaDB agrīnie atvienošanās brīdinājumi sakrita ar šīs zondes avāriju. Gala zonde izmantoja tās pašas `game.lib` un `scripts.lib`, kuras izmantoja Release gala sasaiste. [Pamata 1.–20. līmeņa audits](level-1-20-coverage-followup-2026-11-21.md)

Pārbaudes `ForestEttin` un `MonkeyBusiness` atkārtoti izpildījās sekmīgi. Pagaidu un kanoniskais `worldserver.exe` ar SHA-256 `945D60A6F1A4F4CADC25C40D3E0C5811A972D19FC54FE5DC907378ACD3210D04` izolētajā datu kopijā sasniedza gatavību un korekti izslēdzās. Darba MariaDB visas 11 nolasītās rindas sakrita ar pārbaudītajiem datiem un migrācija ir reģistrēta kā `RELEASED` ar SHA-1 `E2F66D2BB26A6431C71900987E1F7544819674DD`. Gala pārbaudēs nebija jaunu avāriju, `DBErrors.log` ierakstu, servera brīdinājumu vai assertion kļūdu.

Ar pārbaudītām kontrolsummām arhivēti 334 diagnostikas faili mapē `build-extractors/bin/Release/fixed/kezan-hot-rod-2026-11-22/` un izņemti 6 118 170 284 baiti pagaidu failu. Paturēta iepriekšējā kanoniskā Forest Ettin izpildfaila un sasaistes kartes kopija atjaunošanai līdz klienta pārbaudei.
