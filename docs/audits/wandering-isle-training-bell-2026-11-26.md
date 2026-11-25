# The Singing Pools un The Lesson of Dry Fur

Pārbaudīta The Singing Pools `29521` nodošana pie Aysa `54975/244270` un The Lesson of Dry Fur `29661` pieņemšana. Kvests izmanto Training Bell `209608/102318` kartē `860`. No šī objekta sākas 7.3.5 burvestība `102843`, kas ieskaita mērķi `55077`.

Sākotnējā pārbaudē `CMSG_GAME_OBJ_USE` iedarbināja zvanu, bet `CMSG_GAME_OBJ_REPORT_USE` apstājās objekta dialoga ceļā. Zvana AI tagad abas mijiedarbības novada caur objekta standarta lietošanu. Ieejas pārbaudes prasa dzīvu spēlētāju ar nepabeigtu kvestu, īsto zvanu, atbilstošu fāzi, attālumu, redzamību un atrašanos ārpus ūdens. Saglabāts zvana sākotnējais tips un burvestība.

`Test-QuestObjectLootNative.ps1 -SingingPoolsBaseline` atkārtoja atšķirību starp abām paketēm. `-SingingPools` pārbaudīja atteikumu, spiežot no Aysa atrašanās vietas, parasto klikšķi, atmešanu un atkārtotu pieņemšanu, otru klikšķa ceļu, vienu ieskaiti, standarta `54` paketes jostu un Finding an Old Friend `29676` pieņemšanu. Pārbaudē spēlētājs pie zvana tika pārvietots kontrolēti. Kustība pāri stabiem, zvana skaņa un attēlojums spēles klientā paliek atvērti.

Pārbaudes un darba serveris sasniedza gatavību un apstājās ar `exit 0`. Gala pārbaudēs nebija jaunu avāriju, brīdinājumu vai DBErrors ierakstu. Gala saitē izmantotas Release `scripts.lib` un `game.lib`. Saistītajā PE pārbaudīts zvana izsaukums uz standarta lietošanas ceļu un kvesta un attāluma pārbaudes. Kanoniskā EXE SHA-256 ir `8BC783FDB76854A4E5F4C392DC34B089A2C314E971600F3D59E5354E3222C1AC`. Migrācija `2026_11_26_01_wandering_isle_training_bell.sql` reģistrēta kā `RELEASED` ar SHA-1 `BA5E75EB6A0222CF0F8DC65B062521EC3615A7B7`.

Saglabāti 59 diagnostikas faili ar kontrolsummām un iepriekšējā EXE un MAP atjaunošanas kopija. Izņemti 22 pagaidu faili, kopā 993 031 245 baiti. [Diagnostika](../../build-extractors/bin/Release/fixed/2026-11-26-wandering-isle-training-bell/)
