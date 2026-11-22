# Down with Crushcog! (26364)

Jarvi Shadowstep `42353/84470` piedāvā kvestu pēc Paint it Black `26342` un iedod atkārtoti lietojamo Orbital Targeting Device `58253`. Mekkatorque `42849/84169` sarunas izvēlne `11662/0` sāk uzbrukumu kopā ar Stonegrind `42852/84288`. Kvestā atsevišķi jāieskaita saruna ar Mekkatorque un īstā Razlo `42839/84395` sakāve.

Sākotnējos datos izvēlne piešķīra sarunas atzīmi, bet tai nebija kaujas turpinājuma. Razlo trūka transportlīdzekļa saites uz `925`, kam atbilst pilota sēdeklis `8136` un esošais pilots `42494`. Atsevišķā servera pārbaudē ar iepriekšējo bibliotēku Mekkatorque sarunas skaitītājs sasniedza `1`, kauja nesākās, un Razlo faktiskā nāve neatjaunināja uzvaras skaitītāju.

Labojums izmanto esošos pasaules aktierus, četrus sargus, pilota iekāpšanu un spēles uzvaras burvestību `79931`. Pirms kaujas abi sabiedrotie pārvietojas uz cīņas vietu, kur serveris turpmāk mēra viņu kaujas attālumu. Pēc pabeigšanas vai atcelšanas atjaunojas abu sākuma vietas. Ierīces burvestība `80101` izmanto īsto līdzi nesto priekšmetu un izveido privātu mērķa marķieri `42929` ar esošajiem izsaukuma parametriem `1362`. Mekkatorque ieroči `80098/80148/80110` ņem laukuma centru no konkrētā marķiera. Ierīce darbojas pievienotam spēlētājam ar aktīvu kvestu un tikai kaujas laikā. Uzvaras atzīmi saņem tiesīgie dalībnieki pēc faktiskās Razlo nāves.

Dialoga secība, pārvietošanās punkti un kaujas intervāli rekonstruēti no 7.3.5 datiem un divu vēsturisku ierakstu kadriem ar sešu sekunžu soli. Otrajā ierakstā redzama pieņemšana, Mekkatorque saruna, tuvošanās, mērķa atzīmēšana, sakāve un nodošana. Dialoga precīza laika sakritība, skaņa, kaujas vizuālie efekti un izspēle aktuālajā klientā vēl jāpārbauda. [Uzbrukuma ieraksts](https://www.youtube.com/watch?v=EXxbvLe5uRw) · [Pilna kvesta ieraksts](https://www.youtube.com/watch?v=hi8KPJa3tAg)

## Servera pārbaudes

- `Test-QuestObjectLootNative.ps1 -DownWithCrushcog` pārbaudīja Jarvi pieņemšanu, sarunas izvēli, sākotnējā mēģinājuma atmešanu, atkārtotu pieņemšanu un īstu pienākšanu kaujas vietā. Razlo pilots iekāpa savā sēdeklī, un abi pretinieki palika kaujā.
- Pārbaude noraidīja ierīces lietošanu pirms kaujas, sveša dalībnieka metienu, mērķi ārpus kaujas vietas un izsaukumu bez ierīces. Derīgs metiens publicēja privātu marķieri un atņēma Razlo veselību. Vienu pēdējo mājdzīvnieka sitienu ieskaitīja īpašniekam un derīgam grupas biedram. Tuvumā esošais nepievienotais spēlētājs ar aktīvu kvestu kredītu nesaņēma. Jarvi pieņēma atlīdzības komplekta `599` izvēli `59050` un izņēma ierīci. Pēc uzvaras pārbaudīta sabiedroto kaujas apturēšana un abu sākuma pozīciju atjaunošana.
- Saglabātās `PaintItBlack`, `NoTanks` un 92 kvesta laupījuma pārbaudes izpildījās sekmīgi. Vietējam spēles klientam šajā pārbaudē nevarēja iziet maršrutu.

## Izvietošana un diagnostika

- Gala sasaistes kartē pārbaudītas `Map::SummonCreature` un Dun Morogh skriptu reģistrācijas. `game.lib` SHA-256 ir `774353011E682F3383D635449F447736B256C2FAD450F126731F202E32CDF91D`, un `scripts.lib` SHA-256 ir `A1A6D4CA3A19ACE611DCBB86471DDAAC9E508B5D1ED8FBB5147C3E2D338168F3`. Kanoniskā `worldserver.exe` SHA-256 ir `C2892B81E56DA543C03566B421AFF2237A51039162EA39C59C6EF0B42B0DE052`.
- Izolētajā MariaDB migrācija izpildīta divreiz. Darba datubāzē visas 25 pārbaudītās rindas sakrita ar izolētā testa rezultātu. Migrācija `2026_11_22_50_crushcog_assault.sql` reģistrēta ar statusu `RELEASED` un SHA-1 `3E4C7A10784E51D8AF0D280796F9A3D091310D3A`. Abi uzdevuma MariaDB procesi korekti izslēdzās.
- Pārbaudītais kandidāts un kanoniskais Release sasniedza gatavību ar izolētiem datiem un korekti izslēdzās. Gala pārbaudes neradīja avāriju failus, jaunus `DBErrors.log` ierakstus, brīdinājumus vai assertion kļūdas. Sākotnējā pretpiemēra zondes nepilnīgajai izvēlņu datu ielādei trūka POI tabulas pierādījumu. Pārbaudi atkārtoja ar kvestam paredzēto izvēlni, un kļūdas cēlonis reproducējās bez šiem zondes ziņojumiem.
- Sākotnējie un gala žurnāli ar pārbaudītām kontrolsummām glabājas `build-extractors/bin/Release/fixed/down-with-crushcog-2026-11-22/`. Arhivēti 350 faili, noņemti 3 624 689 466 baiti pagaidu būvējumu un testa failu, saglabāts iepriekšējais kanoniskais binārais fails un linker karte atkopšanai. Koplietotā izolētā datubāzes kopija paliek turpmākajām audita pārbaudēm.
