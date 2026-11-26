# Dienvidu meža krājumi un Pei-Wu tīģeri

`Stocking Stalks` sākotnējā pārbaudē `211394` bambusa avotu varēja atvērt spēlētājs bez kvesta un patērēt tukšo laupījumu. Šajos 14 novietojumos ir tikai `80806`. Tie tagad izmanto `29795` un esošo `go_single_quest_supply_chest` pārbaudi. Pārējiem bambusa veidiem un sprāgstvielu saiņiem ir papildu laupījuma rindas. Visas 24 sākotnējās laupījuma rindas saglabātas.

Sākotnējā zondē pēc atmešanas somā palika gan `80806`, gan `74955`. Tie piesaistīti attiecīgi `29795` un `30590` parastajai atmešanas tīrīšanai. Abi priekšmeti ir katrs sava konkrētā kvesta mērķis.

Nativā zonde izgāja Korga piedāvājumu, neatļauta tukšā bambusa avota atvēršanas atteikumu, atmešanu, atkārtotu pieņemšanu un desmit iegūtu bambusu nodošanu. Pārbaudīts `Wrecking the Wreck` piedāvājums un nodošana pie Makael, sešu sprāgstvielu ieguve, izņemšana pēc atmešanas un pēc nodošanas. Salas datos ielādētas 84 bambusa un 35 sprāgstvielu vietas. `Preying on the Predators` pārbaudīti deviņi konkrēti Pei-Wu tīģeri no 98 oriģinālajiem novietojumiem, ar spēlētāja un peta pēdējiem sitieniem, kā arī nodošana pie Wei. Pirmajā tīģeru zondē testa petam trūka īpašnieka kontroles un fāzes. Pēc sagataves labojuma parastā peta ieskaite darbojās bez ražošanas C++ maiņas.

Mērķa rindu un atrasto avotu labojums automātiski ielādēts kanoniskajā Release `79E2CB0AEFD692C88C9C2F548B8FEE2E8AECDF05EF79AD82462D4B67605A2C8A`. Pārbaudītas faktiskās datu rindas un visu 24 laupījuma rindu skaits. Darba serveris sasniedza `ready` un korekti izslēdzās ar statusu 0. Gala zondē un palaišanā nav jaunu servera kļūdu, DBErrors vai avāriju.

Ar kontrolsummām arhivēti 86 faili un izņemti 50 pagaidu faili, 1 801 287 031 baits. Klientā vēl jāpārbauda marķieri, pārvietošanās un izlases papildu laupījums. [Testu pierādījumi](../../build-extractors/bin/Release/fixed/2026-11-26-wandering-isle-south-supplies/)
