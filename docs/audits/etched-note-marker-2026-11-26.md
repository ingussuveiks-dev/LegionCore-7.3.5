# Etched Note 3092

Pārbaudē Tauren Hunter pēc `Our Tribe, Imprisoned` saņēma vienu īsto Etched Note no Rohaku `44927/169370`. Atmešana noņēma priekšmetu, jauna pieņemšana to piešķīra vēlreiz, un Lanka Farshot `3061/169364` pieņēma kvestu un noņēma priekšmetu atlīdzības brīdī. Sākotnējā pārbaudē pēc sekmīgas nodošanas kartes ielādētājam trūka atzīmes.

Pašreizējais Lanka novietojums ir Kalimdor `1`, Camp Narache `6452/221`. Klienta 26972 kartes lapa `890` aptver šo Camp Narache zonu. Sākotnējā atzīmes rinda norādīja uz karti `0` un tai trūka koordinātu. Migrācija sasaista jau esošo nodošanas rindu ar pārbaudīto karti un paņem noapaļotās pašreizējā NPC koordinātas `(-2905, -263)`. Tās iegūtas no darba izvietojuma. [Migrācija](../../sql/updates/world/2026_11_26_28_etched_note_camp_narache_marker.sql)

`-EtchedNote` pārbaudīja īsto pieņemšanu, atkārtošanu, nodošanu un atzīmes ielādi. Divkārša migrācijas izpilde un darba datubāze deva vienu un to pašu punktu. Darba serveris startēja un korekti apstājās bez jaunām avārijām, DBErrors vai servera brīdinājumiem. Ar kontrolsummām saglabāti 40 faili un izņemts 21 pagaidu fails, kopā 719 832 550 baiti. Atzīmes attēlojums WoW klientā paliek jāpārbauda. [Servera zonde](../../tools/tests/EtchedNoteNative.cpp) · [Diagnostikas arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-etched-note-marker/manifest.json)
