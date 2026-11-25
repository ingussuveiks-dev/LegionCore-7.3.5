# Automātiskā kvesta pieņemšana zonā

`Player::PrepareAreaQuest` pēc atļaujas pārbaudes izsauca `AddQuest` un turpināja uz pabeigšanas un kvesta loga darbībām. Ar pilnu žurnālu `AddQuest` atgriezās pirms ieraksta izveides. Kvesta logu tik un tā nosūtīja. Ceļš arī izlaida `CanAddQuest` pārbaudi par vietu obligātajam sākuma priekšmetam.

Pirms pievienošanas tiek izmantota parastā žurnāla un sākuma priekšmeta pārbaude. Pabeigšanu un attiecīgo kvesta logu apstrādā tikai pēc aktīva ieraksta izveides. Spēlētājs var atkārtoti ierosināt pieņemšanu, atgriežoties zonā pēc vietas atbrīvošanas.

`Test-AreaQuestAdmission.ps1` izpildīja divas no produkcijas avota izvilktas metodes kontrolētā C++ testā. Sākotnējais kods reproducēja piedāvājumu ar pilnu žurnālu. Labotā versija izturēja pilnu žurnālu, inventāra trūkumu, jau sasniegtu konkrētā priekšmeta vienību limitu, atkārtotu ierašanos, nederīgu spēlētāju, pazudušu kvesta veidni un neveiksmīgu pievienošanu. Pārbaudīts arī nākamais tās pašas zonas kvests un atkārtojama kvesta atlīdzības logs.

Gala saistē izmantoti CMake Release `game.lib` un `scripts.lib`. Iegūtajā un uzstādītajā PE pārbaudīti `CanTakeQuest`, `CanAddQuest`, `AddQuest` un pabeigšanas un logu izsaukumi. SHA-256 `28373E1747787868C620F860B312954B70C5D0CA4A4BA693CE43AD6EF61EFC88`. Kanoniskais serveris sasniedza gatavību un beidzās ar kodu 0. Jaunu avāriju, servera diagnostikas vai DBErrors ierakstu nebija. Kvesta loga uzvedība 7.3.5 klientā vēl jāpārbauda. Pierādījumi ir zem `build-extractors/bin/Release/fixed/2026-11-25-level-quest-coverage/`.
