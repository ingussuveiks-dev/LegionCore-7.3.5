# Blood Elf Demon Hunter sākuma josla

Jaunam 98. līmeņa Blood Elf Demon Hunter kvestu sākuma zonā Mardum darbību josla klientā bija tukša. Darba tabulā `playercreateinfo_action` nebija nevienas rindas rases `10` un klases `12` pārim. Night Elf Demon Hunter veidnē bija piecas rindas. `Player::Create` kopē konkrētās rases un klases veidni, savukārt klienta pieteikšanās ielādē aktīvās specializācijas `character_action` rindas. Esošā tēla joslā tādu nebija.

Legion 7.3.5 `SkillLineAbility` apstiprina Fel Rush `195072`, Chaos Strike `162794` un Spectral Sight `188501` pie Demon Hunter prasmes `1848` un Arcane Torrent `202719` pie Blood Elf prasmes `756` ar rases masku `512` un klases masku `2048`. Visām četrām darbībām mācīšanās veids ir `2`. Havoc specializācijas `577` ieraksts `5633` māca Demon's Bite `162243`, kas bija arī jau saglabāts tēla burvestībās. Tēlam bija saglabātas abas attiecīgās prasmes. `Player::_LoadSkills` atjauno prasmju burvestības pirms `_LoadActions`, kas pārbauda katru joslas burvestību.

Migrācija `2026_11_27_05_blood_elf_demon_hunter_actions.sql` ieliek piecas sākuma darbības pogās `0`, `1`, `2`, `9` un `10`, saglabājot jau konfigurētas pogas. Esošā pieteiktā tēla tukšajai Havoc joslai tās pašas piecas darbības pievienotas vienā transakcijā, kamēr `worldserver` bija izslēgts. Rakstīšanas nosacījumi pārbaudīja konkrēto tēlu, izrakstīto stāvokli, sākuma līmeni un karti, specializāciju un pilnīgi tukšu aktīvo joslu. Datubāzes pārlasīšanā bija visas piecas paredzētās rindas.

Atsevišķs aģenta servera starts un automatizēti testi izlaisti aktīvās spēles pārbaudes laikā. Pirms nākamā lietotāja starta saglabāts žurnālu garumu un avāriju mapes sākuma stāvoklis. Klienta attēlojums esošajam tēlam un jauna tēla sākuma joslas ielāde vēl nav apstiprināta.

Lietotāja servera startā migrācija piemērota 12:06:22 ar SHA-1 `06BA613725432C19DEFEA2CAFCD525B567554EA9` un `RELEASED` stāvokli. Darba veidnē pārbaudītas visas piecas rindas. Serveris gatavs 12:06:35, un esošā tēla ieiešana redzama 12:07:18. Pievienotajos žurnālu datos nav jaunu kļūdu, brīdinājumu, `DBErrors.log` papildinājumu vai avāriju. Žurnāls apliecina pieslēgšanos, klienta pogu attēlojums vēl jāapstiprina.

Vēlākās spējas paliek ievada kvestu apbalvojumos. Darba datu `Eye On the Prize` (`39049`) apbalvojumā ir Eye Beam `198013`, bet `Fel Secrets` (`40051`) apbalvojumā ir Metamorphosis `191427`. Blizzard apraksta Mardum kā pakāpenisku spēju apguvi, sākot ar 98. līmeni. [Blizzard klases ievads](https://worldofwarcraft.blizzard.com/en-us/news/20231211/becoming-illidari-a-look-at-the-demon-hunter)

Vēlākā lietotāja Mardum ekrānattēlā esošajam tēlam darbību josla jau ir aizpildīta. Jauna tēla izveidošana pēc migrācijas atsevišķi nav pārbaudīta klientā.
