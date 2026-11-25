# The Disciple’s Challenge (29409)

Sākotnējā servera pārbaudē Jaomin Ro `54611/243731` ieskaitīja uzvaru arī tuvumā stāvošam spēlētājam ārpus grupas. Pēc cīņas pārtraukšanas deviņu sekunžu atjaunošanās gaidīja aktīvu upuri un pretinieks palika nepieejams. Jaunais skripts saista uzvaru ar pēdējo spēlētāja, mājdzīvnieka vai aizbildņa sitienu. Ieskaiti saņem tikai spēlētājs un tiesīgie grupas biedri Jaomin tuvumā un tajā pašā fāzē. Saglabāta piecu procentu padošanās robeža, pieklājības žests, reibonis un atsevišķā piekūna fāze. Atjaunošanās skaita laiku arī pēc cīņas beigām un izmanto esošo atgriešanās un reset ceļu.

`Test-QuestObjectLootNative.ps1 -WanderingFirstLesson` ar novietoto Jaomin pārbaudīja tēlu bez kvesta, piekūna fāzi pirms padošanās, mājdzīvnieka beigu sitienu, tuva un tāla grupas biedra un ārpusgrupas tēla ieskaiti, atkārtotu sitienu padošanās laikā un divus jaunus izaicinātājus pēc atgriešanās. Nākamās uzvaras sasniedza spēlētājs ar vienu sitienu un cita spēlētāja aizbildnis. Grupas ieskaitei izmantota esošā kvesta progresa metode ar attālumu no Jaomin. Vispārīgais notikuma izsaukums ar pašu NPC kā avotu šai objektīva konfigurācijai izmantoja sarunas ceļu un ieskaitīja tikai izsaucējam.

Pārbaudīta īstā `70` atlīdzības izvēle un pāreja uz `29410`. Aysa of the Tushui pabeigts pie novietotā Merchant Lorvo `54943/243772`. Pēc nodošanas viņš piedāvāja The Missing Driver `29419` un Items of Utmost Importance `29424`, kurus varēja pieņemt kopā. Datubāzes kvestu saites saglabātas.

Kanoniskā Release izpildfaila SHA-256 ir `64216EE659AE2315D2ED97F3E41D90C3FE992AC82F9106C27A71A1D139885A48`. Gala saišu kartē un izpildfailā pārbaudīts bojājuma apstrādātājs ar īsto kvesta ieskaiti un atgriešanās izsaukums. Pārbaudes un darba serveris sasniedza gatavību un korekti apstājās bez jaunām avārijām, servera brīdinājumiem vai DBErrors ierakstiem. [Diagnostika](../../build-extractors/bin/Release/fixed/2026-11-25-wandering-isle-disciple-challenge/)

Ar kontrolsummām saglabāti 89 diagnostikas faili. Izņemti 42 pagaidu faili, kopā 1 726 469 981 baits. Saglabāta iepriekšējā Release EXE un MAP atjaunošanas kopija.

Spēles klientā vēl jāapstiprina pārvērtības attēlojums, padošanās animācija, kā arī pāreja līdz Lorvo.
