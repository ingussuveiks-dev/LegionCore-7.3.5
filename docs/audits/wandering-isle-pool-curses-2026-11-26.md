# Singing Pools lāsti un līdzsvara stabi

Vecais vienreizējais ieejas taimeris palaida garām kritienu tajā pašā baseina trigerī, ja spēlētājs ieejas brīdī atradās virs ūdens. Nokāpjot no staba, skripts vēlreiz izsauca pēdējo atcerēto apgabala trigeri. Lāstu tagad nosaka pašreizējās 7.3.5 trigera robežas un ūdens sadure. Lāsts pazūd, izkāpjot sausumā, nomainot karti vai izrakstoties. Līdzsvara stabs saglabā ierobežojumu vardes formai, bet nokāpšana vairs neizsauc agrāku trigeri.

`Test-QuestObjectLootNative.ps1 -SingingPoolsWaterBaseline` ar oriģinālās kartes ūdeni atkārtoja kritienu tajā pašā `6986` trigerī bez pārvērtības. `-SingingPools` pārbaudīja šķidruma un sausuma pārejas visos sešos trigeros `6986/6987/6988/6990/6991/7012` un visas piecas formas. Pārbaudīts arī novietotais stabs `54993/244137`, īstā mijiedarbība `46598`, vardes formas atteikums, uzkāpšana sausam tēlam, nokāpšana un atkārtots kritiens. Izgāja arī iepriekšējā zvana un nodošanas pārbaude. Pilns maršruts, attēlojums un kustība klientā paliek atvērti.

Sākotnējās zondes meklēja nepareizu ūdens plaknes paraugu. Divi staba mēģinājumi avarēja zondē, kurā trūka standarta burvestību proc datu ielādes. Pēc tās pievienošanas īstais stabs un visi baseini izgāja ar `exit 0`. Saglabātas arī agrāko mēģinājumu diagnostikas.

Pārbaudes un darba serveris sasniedza gatavību un korekti apstājās. Gala zondē un abās palaišanās nebija jaunu avāriju, servera brīdinājumu vai DBErrors ierakstu. Gala saitē izmantotas paredzētās `scripts.lib` un `game.lib`. Saistītajā PE pārbaudīts izsaukums uz trigera robežām, šķidruma datiem un auras noņemšanu. Kanoniskā EXE SHA-256 ir `8EE0129AEFAFED1463C7632633BA556195B6E78787260C0A3A7030230CD142EB`.

Saglabāti 177 diagnostikas faili ar kontrolsummām un iepriekšējā EXE un MAP atjaunošanas kopija. Izņemti 112 pagaidu faili, kopā 4 263 147 310 baiti. [Diagnostika](../../build-extractors/bin/Release/fixed/2026-11-26-wandering-isle-pool-curses/)
