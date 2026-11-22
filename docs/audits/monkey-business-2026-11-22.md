# Monkey Business (14019)

Bamm Megabomb `35758` novietojumā `189988` Lost Isles kartē `648` piedāvā un pieņem kvestu. No viņa saņem vienu banānu ķekaru `49028`, ko lieto desmit reizes ar burvestību `67917`. Teritorijā `4721` atrodas 16 Bomb-Throwing Monkey `34699` novietojumi fāzes maskā `14`. Mērķis prasa desmit barošanas ieskaites `35760`.

Pirms labojuma somā palikušus banānus varēja izlietot uz pērtiķa arī bez kvesta, izraisot tā skriptēto nāvi. SQL migrācija `2026_11_22_14_kezan_monkey_business.sql` prasa aktīvu `14019`, somā esošus banānus un dzīvu `34699` kā mērķi. Esošais SmartAI barošanas notikums izpildās vienreiz uz vienu parādīšanos. Saglabāta skaņa, `67919` aktivizācija, skriptētā nāve un sākotnējais burvestības atteikums par cita spēlētāja kaujā piesaistītu mērķi. Vienu saņemto banānu ķekaru var lietot vairākas reizes.

`Test-QuestObjectLootNative.ps1 -MonkeyBusinessBaseline` atkārtoja barošanu bez aktīva kvesta. `-MonkeyBusiness` izmantoja desmit faktiskos pērtiķu novietojumus un pārbaudīja ieskaiti, atteikumu citam un mirušam mērķim, atteikumu bez priekšmeta vai pēc pabeigšanas, iznākumu parastā kaujā un banānu izņemšanu pēc atmešanas. Sākotnējā burvestība saglabāja tiesības izmantot paša piesaistītu mērķi un atteica sveša spēlētāja mērķi. Faktiskā atlīdzības pakete pie Bamm izsniedza izvēlēto atlīdzību un izņēma ķekaru no somas.

Migrācijas atkārtota izpilde saglabāja identiskus datus. Kanoniskais Release `7A536D1EDCD34529439FEA1147C14088D7DE5D944DF6AD0190912E45E75C6FCC` ar pārbaudīto datubāzi startējās un korekti izslēdzās bez jaunas kļūdu diagnostikas. Produkcijā salīdzinātas četras nosacījumu rindas, visas deviņas SmartAI rindas un migrācijas kontrolsumma. Lokālajā `build-extractors/bin/Release/fixed/kezan-monkey-business-2026-11-22/` saglabātas 116 diagnostikas datnes ar kontrolsummām. Pagaidu izpildfailu un būves datņu izņemšana atbrīvoja 1 950 873 498 baitus.

Klientā atsevišķi pārbaudāma redzamība gaitā no iepriekšējās Goblinu sākuma salas fāzes un banānu izmešanas vizuālais efekts.

## Spēlētāji maina kaujas piesaisti metiena laikā

`-MonkeyBusinessFlightBaseline` ar bibliotēku no `2dc07336c` atkārtoja ieskaiti nepareizajam spēlētājam. Kvestu pieņēmušais metējs trāpīja parastajam mērķim, bet pa to laiku cits spēlētājs bija to piesaistījis kaujā. Iepriekšējais trāpījums izmantoja pēdējo kaujas piesaisti un palaida pērtiķa nāves notikumu.

`67917` tagad glabā metiena kvesta pieņemšanas reizi un trāpījuma brīdī vēlreiz pārbauda aktīvo kvestu, banānus somā, dzīvu mērķi un sākotnējo aizliegumu izmantot sveša spēlētāja piesaistītu mērķi. Atteikts trāpījums neizpilda SmartAI notikumu un neizlieto pašu mērķi. Veiksmīga trāpījuma ieskaiti sāk faktiskais barotājs, un native grupas nosacījumi joprojām nodrošina ieskaiti tiesīgiem biedriem.

Paplašinātā `-MonkeyBusiness` pārbaude beidzās ar desmit sekmīgiem barošanas gadījumiem, dzīvu mērķi pēc svešas piesaistes, trāpījuma atteikumu pēc atmešanas un atkārtotas pieņemšanas, kā arī derīgu grupas ieskaiti. Atkārtotās `-HogtiedRazormane` un `-TongaGuide` pārbaudes pabeigtas bez jaunām kļūdām. Pagaidu un kanoniskais Release `743352C5779843C4CA71EEF1E21B22CFDC68F24150F5F9C0BEE9C61288CCF6CA` sasniedza `ready` un korekti izslēdzās. Gala saitē apstiprināti `SmartScript.obj` un `Spell.obj`. Šim labojumam pietika ar C++ izmaiņām. Saglabātas 102 diagnostikas datnes ar pārbaudītām kontrolsummām `build-extractors/bin/Release/fixed/kezan-monkey-flight-2026-11-22/`. Pagaidu datņu izņemšana atbrīvoja 2 281 492 743 baitus.
