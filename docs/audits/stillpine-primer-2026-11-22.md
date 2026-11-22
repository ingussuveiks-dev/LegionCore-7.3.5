# Learning the Language (9538)

Azuremyst Isle Aurren `17232` novietojumā `15825` dod vienu Stillpine Furbolg Language Primer `23818`. Tā `30419` burvestība lasa grāmatu pašam spēlētājam 12 sekundes. Totem of Coo `17360` novietojumā `16557` pieņem pabeigto `9538` un piedāvā `9539`.

Pirms labojuma varēja izlietot somā esošu grāmatu bez aktīva `9538`. Īstais lasīšanas efekts ar pieņemtu kvestu izsauca veco `AreaExploredOrEventHappens` ceļu un ierakstīja kļūdu servera žurnālā. Pievienotas prasības pēc aktīva kvesta un somā esošas grāmatas. Native efekts `16` tagad pabeidz tikai aktīvo `9538` kā sākotnējo bezskaitītāju notikumu ar `CompleteQuest`. Pārējiem efekta `16` lietojumiem paliek to līdzšinējā apstrāde.

`Test-QuestObjectLootNative.ps1 -StillpinePrimerBaseline` atkārtoja grāmatas izlietošanu bez kvesta. `-StillpinePrimer` ar labojumu apstiprināja atteikumu šajā gadījumā, grāmatas lasīšanu tikai pašam spēlētājam, citas personas kvesta un grāmatas saglabāšanu, vienas izlasītas grāmatas patērēšanu, nelasītas grāmatas izņemšanu pēc atmešanas un faktisko nodošanas paketi pie Totem of Coo. Nākamais kvesta piedāvājums `9539` apstiprināts serverī. Turpmāko totemu ceļš un klienta lasīšanas animācija jāpārbauda atsevišķi.

Pagaidu un kanoniskais Release SHA-256 `7A536D1EDCD34529439FEA1147C14088D7DE5D944DF6AD0190912E45E75C6FCC` sasniedza `ready` un korekti izslēdzās. Gala saitē izmantots `game:SpellEffects.obj`. Kompilators parādīja iepriekš esošu C5055 rindu `MoveDistract`, kas netika mainīta. Gala servera un native pārbaudes pabeigtas ar tīru diagnostiku. Produkcijas divas nosacījumu rindas un migrācijas kontrolsumma salīdzināta ar pārbaudīto kopu.

Ar pārbaudītām kontrolsummām saglabātas 116 diagnostikas datnes lokālajā `build-extractors/bin/Release/fixed/stillpine-primer-2026-11-22/`. Pēc pārbaudes izņemtas atjaunojamās pagaidu datnes, atbrīvojot 1 004 362 054 baitus. Iepriekšējais izpildfails saglabāts atjaunošanai.
