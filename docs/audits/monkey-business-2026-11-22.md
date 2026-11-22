# Monkey Business (14019)

Bamm Megabomb `35758` novietojumā `189988` Lost Isles kartē `648` piedāvā un pieņem kvestu. No viņa saņem vienu banānu ķekaru `49028`, ko lieto desmit reizes ar burvestību `67917`. Teritorijā `4721` atrodas 16 Bomb-Throwing Monkey `34699` novietojumi fāzes maskā `14`. Mērķis prasa desmit barošanas ieskaites `35760`.

Pirms labojuma somā palikušus banānus varēja izlietot uz pērtiķa arī bez kvesta, izraisot tā skriptēto nāvi. SQL migrācija `2026_11_22_14_kezan_monkey_business.sql` prasa aktīvu `14019`, somā esošus banānus un dzīvu `34699` kā mērķi. Esošais SmartAI barošanas notikums izpildās vienreiz uz vienu parādīšanos. Saglabāta skaņa, `67919` aktivizācija, skriptētā nāve un sākotnējais burvestības atteikums par cita spēlētāja kaujā piesaistītu mērķi. Vienu saņemto banānu ķekaru var lietot vairākas reizes.

`Test-QuestObjectLootNative.ps1 -MonkeyBusinessBaseline` atkārtoja barošanu bez aktīva kvesta. `-MonkeyBusiness` izmantoja desmit faktiskos pērtiķu novietojumus un pārbaudīja ieskaiti, atteikumu citam un mirušam mērķim, atteikumu bez priekšmeta vai pēc pabeigšanas, iznākumu parastā kaujā un banānu izņemšanu pēc atmešanas. Sākotnējā burvestība saglabāja tiesības izmantot paša piesaistītu mērķi un atteica sveša spēlētāja mērķi. Faktiskā atlīdzības pakete pie Bamm izsniedza izvēlēto atlīdzību un izņēma ķekaru no somas.

Migrācijas atkārtota izpilde saglabāja identiskus datus. Kanoniskais Release `7A536D1EDCD34529439FEA1147C14088D7DE5D944DF6AD0190912E45E75C6FCC` ar pārbaudīto datubāzi startējās un korekti izslēdzās bez jaunas kļūdu diagnostikas. Produkcijā salīdzinātas četras nosacījumu rindas, visas deviņas SmartAI rindas un migrācijas kontrolsumma. Lokālajā `build-extractors/bin/Release/fixed/kezan-monkey-business-2026-11-22/` saglabātas 116 diagnostikas datnes ar kontrolsummām. Pagaidu izpildfailu un būves datņu izņemšana atbrīvoja 1 950 873 498 baitus.

Klientā atsevišķi pārbaudāma redzamība gaitā no iepriekšējās Goblinu sākuma salas fāzes un banānu izmešanas vizuālais efekts.
