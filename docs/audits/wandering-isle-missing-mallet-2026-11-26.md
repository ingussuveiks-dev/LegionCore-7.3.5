# Missing Mallet (29768)

Vienīgo novietoto Dai-Lo Recess Mallet `214406/102549` varēja iztukšot apmeklētājs bez kvesta. Atmetot kvestu, savāktais `74298` palika somā. Migrācija piesaista objektam esošo viena kvesta krājumu pārbaudi un iekļauj āmuru standarta atmešanas tīrīšanā. Ji nodošanai joprojām vajag no objekta saņemto āmuru.

`Test-QuestObjectLootNative.ps1 -SingingPoolsMalletBaseline` atkārtoja tukšo koplietoto laupījumu un palikušo āmuru. `-SingingPools` pārbaudīja `Rascals` nodošanu, `Missing Mallet` pieņemšanu pie Ji, atteikumu bez kvesta, īstā āmura paņemšanu Ki-Han Brewery, vecā atlikuma izņemšanu pēc atmešanas, atkārtotu savākšanu un `59` paketes atlīdzību. Fiziskā ieeja alus darītavā un laupījuma attēlojums paliek pārbaudāmi klientā.

Migrācija `2026_11_26_08_wandering_isle_missing_mallet.sql` divreiz piemērota testa datubāzē un darba serverī reģistrēta kā `RELEASED` ar SHA-1 `A7562BD3586E714E3266442C01592154BDFDC7B7`. Izolētais un darba serveris sasniedza gatavību un korekti apstājās. Pārbaudēs nebija jaunu avāriju, servera brīdinājumu, kļūdu vai DBErrors ierakstu. Saglabāti 60 diagnostikas faili ar kontrolsummām. Izņemti 20 pagaidu faili. [Diagnostika](../../build-extractors/bin/Release/fixed/2026-11-26-wandering-isle-missing-mallet/)
