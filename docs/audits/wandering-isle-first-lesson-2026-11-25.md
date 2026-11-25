# Wandering Isle pirmā mācība

Lokālajā Legion 26972 `QuestLineXQuest` ķēdē 573 pirmie septiņi ieraksti ir `30027/30033/30034/30035/30036/30037/30038`, un astotais ir `29406`. Shang Xi `53566` jau piedāvāja visas septiņas klases mācības. Servera zonas 5834 dati automātiski pievienoja vecos `30039–30045` variantus, un katrai ieroča mācībai pieņemšanu aizturēja divi addon priekšnoteikumu lauki. Vecā ievada izņemšana sakrīt ar 7.0.3 vēsturi. [Much to Learn vēsture](https://warcraft.wiki.gg/wiki/Much_to_Learn)

Migrācija noņem tikai zonas 5834 vecos piedāvājumus un zināmās savstarpējās prasības. Sākotnējās klases un rases prasības un pāreja uz `29406` darbojas ar septiņām pārbaudītām datu rindām. Saglabāti veco kvestu saņēmēji un nākamā kvesta ID, tādēļ esošam tēlam ar pieņemtu ievadu joprojām ir nodošanas ceļš. Burvestības `56267` atbilstošās zonas rindas ir pieejamas ar izvēlētās mācības statusu un esošo `29409` prasību. Vecā kvesta atbilstošā aura saglabāta.

MariaDB pagaidu tabulās pārbaudītas visas septiņas pārejas, divkārša migrācija, blakus zona un pielāgots priekšnoteikums un auras ieraksts. Darba datubāzes nolasījums sakrita ar pārbaudītajiem datiem. Updater reģistrēja SQL kā `RELEASED` ar SHA-1 `4137E906B120C22BEB290EEFE5A4E46A59ABB84E`. Kanoniskais worldserver sasniedza gatavību un korekti izslēdzās bez jaunām avārijām, servera brīdinājumiem, kļūdām vai DBErrors ierakstiem.

Spēles klientā vēl jāpārbauda jaunā tēla ierašanās, Shang Xi piedāvājums, ieroča aprīkošanas attēlojums un pāreja uz `29406`, kā arī iepriekš uzsākta ievada nodošana. Pierādījumi ir zem `build-extractors/bin/Release/fixed/2026-11-25-level-quest-coverage/`.
