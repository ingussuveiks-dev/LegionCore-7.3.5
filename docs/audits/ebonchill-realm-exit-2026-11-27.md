# Ebonchill izeja no Balaadur valstības

Spēlētājs saņēma Ebonchill, un uzdevums `42479` prasīja atgriezties pie Meryl. Spēlē trūka izejas. Abi saglabātie uzdevuma skaitītāji bija sasniegti. Tēlam ir apgūta arī parastā `Teleport: Hall of the Guardian` burvestība `193759`. Uzdevuma aprakstā pēc Ebonchill paņemšanas ir paredzēta atgriešanās caur altāri. [The Mage Hunter](https://warcraft.wiki.gg/wiki/The_Mage_Hunter)

Esošais portāla objekts `251051` izmanto Legion 7.3.5 burvestību `214778`. Datubāzē portālam nebija parādīšanās avota vai burvestības galamērķa. Instance pēc pabeigtas ieroča paņemšanas publicē personīgu portālu tikai burvim, kuram ir abi uzdevuma skaitītāji un Ebonchill ekipējumā vai līdzi somās. Klikšķis ar oriģinālo burvestību ved uz Meryl datubāzes sākumpunktu pie altāra. Precīzo vēsturisko portāla novietojumu nevarēja noteikt, tādēļ izmantota pārbaudāma platformas pozīcija blakus ieroča paņemšanas vietai. Ja pabeigtā instance vairs nepieņem tēlu pēc ielogošanās, tie paši saglabātie skaitītāji un reāli līdzi esošs ierocis ļauj izmantot esošo `214991` galamērķi Hall of the Guardian. Pabeigšanas prasības un ieroča saņemšana paliek dzimtajā uzdevuma plūsmā.

Spēlētājs apstiprināja ieroča saņemšanu sākotnējā sesijā. Portāla izskats, klikšķis, ierašanās pie altāra un pabeigtas instances ielogošanās vēl jāpārbauda klientā. Aktīvās spēles darbplūsmā papildu automatizētu vai atsevišķa servera testu neveicām.

Dzēsti 10 lieki testa būvējumu izpildfaili un saišu kartes, kopā 832 484 333 baiti. Katram failam saglabāta identiska kopija ar pārbaudītu SHA-256. Darba servera failu un konfigurācijas kontrolsummas pēc tīrīšanas sakrita. Dzēsto ceļu un kontrolsummu saraksts ir lokālā `.codex/ebonchill-realm-exit-20261127/duplicate-build-cleanup-result.json` failā.

Release kompilācija un gala saistīšana pabeigta ar aktuālajiem `game.lib` un `scripts.lib`. Saišu kartē pārbaudīti portāla publicēšanas un pabeigtas instances ielogošanās ceļi. Sagatavotā izpildfaila SHA-256 ir `672A1CF179931A6D653A0F5D47833E5FFFDD9E90A718CC44FA63CEEE5AE70D9A`. Lietotāja sākotnējās spēles sesijas žurnālā pirms izvietošanas nebija jaunu SQL kļūdu vai avāriju. Iepriekšējais jauktais kļūdu žurnāls saglabāts.

Apvienotais būvējums izvietots kanoniskajā Release mapē. Datubāzes migrāciju pārbaude ir piesaistīta lietotāja nākamajai servera palaišanai.
