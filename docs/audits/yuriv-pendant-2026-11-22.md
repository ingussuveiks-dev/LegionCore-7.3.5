# Until Death Do Us Part (264)

Kvesta devēja Clarice Foster `5543` ir Thunder Bluff novietojumā `94437`. Viņa dod vienu Clarice's Pendant `6145`. Nodošanas objekts ir Yuriv kapakmens `24776`, Silverpine novietojumā `6121`. Tā slēdzene `179` prasa `LOCKTYPE_OPEN_KNEELING`, bet kulona `8001` burvestība iepriekš prasīja slēdzenes tipu 0.

`8001` tagad izmanto atbilstošo slēdzenes tipu. Burvestībai nepieciešams somā esošs kulons, pieņemts `264` un faktiskā nodošanas objekta mērķis. Pieļauti gan nepabeigti kvesti, gan kvesti, kam jau ieskaitīts somā esošais kulons. Kopīgais kapakmens paliek pasaules objekts un pieņemtā kvesta pārbaude attiecas uz kulona lietošanu.

`Test-QuestObjectLootNative.ps1 -YurivPendant` apstiprināja īsto pasaules slēdzeni, kvesta priekšmeta saņemšanu, kļūdaina mērķa atteikumu, atteikumu bez pieņemta kvesta, nodošanas izvēlni un kulona izņemšanu no somas gan pēc nodošanas, gan pēc atmešanas. Tests izmantoja esošo blakus novietoto plāksni `24777` kā nepareizo mērķi. Sākotnējās testa pārbaudēs precizēta devējas karte un slēdzenes prasības pozīcija DB2 masīvā. Klientā vēl jāpārbauda attēlotais lietošanas un nodošanas dialogs.

Gala Release SHA-256 `ECE34751E01A1080A9FC6B6AB6E9826D313CD52F10871F612072EA49B142D063`. Gala saite izmantoja `game:SpellMgr.obj`. Staged un kanoniskais serveris sasniedza `ready` izolētajā MariaDB un korekti izslēdzās bez jaunām avārijām vai neizskaidrotām kļūdām. Produkcijas sešu nosacījumu rindas un migrācijas kontrolsumma salīdzinātas ar pārbaudītajiem datiem.

Diagnostikas kopijas un pārbaudītās kontrolsummas glabājas `build-extractors/bin/Release/fixed/yuriv-pendant-2026-11-22/`. Noņemti 640 650 054 baiti pagaidu failu un saglabāta iepriekšējā Release atjaunošanas kopija.
