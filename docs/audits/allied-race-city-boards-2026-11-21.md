# Stranger in a Strange Land Legion 7.3.5

Lokālie `quest_template_locale` un `quest_objectives_locale` ieraksti no būvējuma 26124 prasa atrast frakcijas ziņojumu dēli. Vecākais pieejamais TrinityCore vēstures ieraksts `2018_05_26_10_world.sql` saglabā dēļa mērķa tekstu, un 20. līmeņa maršruts ir redzams arī laikmeta datos. Mūsdienu Chromie uzdevums ir vēlākas versijas turpinājums. [Legion kvesta dati](https://beta.wowdb.com/quests/50313-stranger-in-a-strange-land) · [Legion maršrutu pieraksti](https://www.wow-pro.com/legion-allied-races-guides/)

| Kvests | Priekšnosacījums | Dēlis | Saņēmējs |
| --- | --- | --- | --- |
| `50303` Nightborne | `49933` | Orgrimmar `206109` / spawns `107487` | Melitier Vahlouran `131328`, fāze `4001` |
| `50305` Void Elf | `49788` | Stormwind `206294` / spawns `108162` | Keira Onyxraven `131347`, fāze `4000` |
| `50313` Lightforged Draenei | `49772` | Stormwind `206294` / spawns `108162` | Vindicator Minkey `131334`, fāze `4001` |
| `50319` Highmountain Tauren | `49773` | Orgrimmar `206109` / spawns `107487` | Halian Shlavahawk `131201`, fāze `4000` |

Visiem četriem mērķiem bija `Amount=0`. Parastā pieņemšana tādēļ uzreiz atzīmēja kvestu kā izpildītu un ieslēdza attiecīgā vēstnieka fāzi. Tagad standarta objekta mērķis prasa vienu darbību pie īstā dēļa. Abi dēļi saglabā esošo izvēlni un citu kvestu piedāvājumus. Esošā fāzes definīcija parāda atbilstošo vēstnieku pēc dēļa kredīta un noņem fāzi pēc atmešanas vai atlīdzības saņemšanas.

Ielādējot tēlu, nepabeigtiem nodošanas ierakstiem ar veco statusu `COMPLETE` un nulles kredītu atjaunojas aktīvais mērķis. Native atlīdzības pārbaude prasa iegūto kredītu arī vecam ierakstam pirms atjaunošanas. Šajos četros kvestos nav kvesta priekšmetu vai priekšmetu atlīdzības. Astoņiem kvesta un priekšnosacījuma ierakstiem angļu valodas pamatteksts atjaunots no esošajām 26124 lokalizācijas rindām. Dēļu kartes punkti atbilst faktiskajiem spawniem. Diviem variantiem trūkstošie piedāvātāja un saņēmēja punkti atjaunoti no viņu izvietojuma un atzīmēti kā lokāli atvasināti.

## Pārbaudes un izvietošana

- `Test-QuestObjectLootNative.ps1 -AlliedCityBaseline` atkārtoja priekšlaicīgu pabeigšanu un vēstnieka redzamību ar visiem četriem sākotnējiem mērķiem. Pirmajā mēģinājumā testa privātās piekļuves funkcija neielinkojās. Nākamajā mēģinājumā fiksēta gaidīta neatbilstība, kad jaunais skripts reģistrēts ar sākotnējo datubāzi. Pēc testa uzstādījuma labošanas atšķirība reproducēta ar tīru diagnostiku.
- `-AlliedCity` izpildīja faktisko pieņemšanas, objekta kredīta, fāžu un atlīdzības kodu visām četrām rasēm. Pārbaudīti minimālais līmenis un priekšnosacījums, nepareiza rase, dēļa kopija un darbība no attāluma, abi dēļa mijiedarbības ceļi, vecs nulles kredīta ieraksts, atkārtots klikšķis, atmešana un atkārtota pieņemšana. Pēc kredīta pareizais NPC bija sasniedzams un atlīdzības izvēles pārbaude izturēta, kamēr otram spēlētājam mijiedarbība bija nepieejama. Pārbaudīts arī fāzes stāvoklis pēc saglabātas atlīdzības. Saglabātā `-Anowyn` pārbaude izturēta.
- Būvējums izmantoja faktiskās `game.lib` un `scripts.lib` ievades, ar pārbaudītiem `game:Player.obj` un `scripts:go_scripts.obj` simboliem. Kanoniskais `worldserver.exe` ar SHA-256 `CFEB85E7BA0BC2EECCE124E4A69CF023E7477E99E735E63175E3A66C2B664408` startēja ar migrēto izolēto pasauli un korekti apstājās. Migrācija `2026_11_21_40_allied_race_city_boards.sql` ar SHA-1 `EACAE8654501FD3ECEB10CFF6D6939BE62D86EBA` piemērota `legion_world` un reģistrēta kā `RELEASED`. Pilns ražošanas nolasījums sakrita ar pārbaudīto datu kopiju.
- Sekmīgajās pārbaudēs un importā nebija jaunu avāriju ziņojumu, `DBErrors.log` ierakstu vai neizskaidrotu servera brīdinājumu. `build-extractors/bin/Release/fixed/allied-race-city-boards-2026-11-21/` saglabāti 69 diagnostikas faili ar pārbaudītām SHA-256 kontrolsummām. Izņemti 39 pagaidu faili jeb 1 720 799 057 baiti. Saglabāta atjaunošanas kopija un atkārtoti lietojamā audita datubāzes sagatave. Uzdevuma procesi ir apstājušies.

7.3.5 klientā vēl jāiziet iepriekšējā kvesta ceļš uz galvaspilsētu, dēļa un kartes attēlojums, vēstnieka parādīšanās un pati atlīdzības saņemšana. Native pārbaudē izmantota faktiskā atlīdzības izvēles atbilstības funkcija un sasniedzamais saņēmējs.
