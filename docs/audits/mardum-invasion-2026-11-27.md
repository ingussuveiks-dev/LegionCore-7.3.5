# Mardum ievads un The Invasion Begins karogs

## Illidan dialogs

Mardum kvestā `40077` atkārtojās abas Illidan ievada rindas. Tās sakrīt ar Legion `26972` `ConversationLine` ierakstiem `1683` un `1684`, `BroadcastText` `100005` un `100571`, darba sarunu `705` un ainu `1106` (pakotne `1487`). `sceneTrigger_dh_init` līdz šim izveidoja sarunu pēc katra `CUEILLIDANTH`, arī pēc ievada uzskaites kvesta `40076` saņemšanas. Pieteiktajam tēlam `40076` jau bija apbalvots. Tagad Mardum Demon Hunter var saņemt ievada sarunu, kamēr šim uzskaites kvestam ir `NONE` vai `INCOMPLETE`. Uzskaites kvests tiek pabeigts pēc sekmīgas sarunas izveides. Sākuma aina un tās pārējie notikumi paliek aktīvi.

## Banner Illidari

Kad `The Invasion Begins` `40077` dēmonu skaits bija sasniedzis 15, sākotnējais karogs rādīja `0/1 Change the Legion banner`. Darba objekts `244898`/`131210` ir Mardum tipa `10` goober ar native Lock `93` (Open Tinkering), `PERSONAL_USE` un ieslēgtu `IN_USE` jau pēc ielādes. `IN_USE` bloķē parasto `GameObject::Use`. Iepriekšējais skripta `GossipHello(true)` netika sasniegts no goober izmantošanas vai atslēgas burvestības. Klienta reporta pakete izsauc `GossipHello(false)`. `Spell::SendLoot` pēc sekmīgas slēdzenes atvēršanas izsauc `GameObject::Use`.

Migrācija `06` noņem `IN_USE` un saglabā `PERSONAL_USE`, Lock `93` un pārējos veidnes laukus. Objekta AI `GossipUse` tagad apstrādā faktisko lietošanu. Tam vajadzīgs dzīvs spēlētājs pie redzama objekta pareizajā fāzē ar aktīvu kvestu, pabeigtu `280276` un vēl neieskaitītu `280292`. Ieskaiti dod parastais `KillCreditGO`. Pēc ieskaites sākas esošā personīgā aina `191677`/`1116`/`1493`, un kopīgajam objektam neiedarbina lietošanas aizkavi, kas bloķētu nākamo spēlētāju. Nošķirta mijiedarbības atskaite iespēju ieskaitīt nedod.

## Izvietošana

Release `scripts.lib` un `game.lib` izmantoti no faktiskajiem CMake saites ceļiem. Gala saites kartē abi Mardum apstrādātāji ir no `scripts:mardum.obj`. Kanoniskajā `build-extractors/bin/Release` pēc lietotāja servera apstāšanās ievietots `worldserver.exe` ar SHA-256 `3CD5DD977FBBC442F242F5B72F031874C9EA5F4BA17B83DD9008E73AF17814D5`. Atjauninātājam nodota migrācija `2026_11_27_06_mardum_invasion_banner.sql`. Pirms lietotāja pārbaudes saglabāti žurnālu izmēri un avāriju saraksts. Lietotājs lūdza spēles pārbaudi veikt pašam. Tāpēc veikta kompilācija un saites pārbaude, bet atsevišķas zondes un aģenta servera starts nav veikti. Klientā vēl jāapstiprina karoga ieskaite un aina, ievada dialogs pirmoreiz un tā neatkārtošanās.

Lietotāja startā migrācija piemērota 12:33:20 ar atjauninātāja SHA-1 `D6C3A7A113D3734B8C187E88DDEC3BA9AB244B77`. Darba tabula apstiprina `flags=262144`, Lock `93` un `go_q40077`. Serveris bija gatavs 12:33:35 bez jaunas avārijas, brīdinājuma, kļūdas vai `DBErrors.log` papildinājuma. 12:34:30 serveris reģistrēja pieteikšanos ar Demon Hunter.

Lietotāja retestā karogs palika `0/1`. Saglabātie `40077` skaitītāji 15 un 0 apstiprināja, ka dēmonu prasība bija izpildīta. Iepriekšējais skripts `GetQuestObjectiveData` ķēdei nodeva objektīvu ierakstu ID `280276` un `280292`. Faktiskā pārslodze meklē `ObjectID`. Tāpēc dēmonu pārbaude atgrieza nulli. Labojumā izmantoti `98486` un karoga īstais objekta ID `244898`. Mijiedarbības atskaites ceļš var sākt `6477` Opening ar native `LockType` 12, ko apstiprina 26972 `SpellEffect` `2266` (efekts `33`, mērķis `23`) un `Lock` `93`. Ieskaiti turpina dot faktiskais lietošanas ceļš pēc atvēršanas. Atkārtota atskaite neaizstāj jau notiekošo `6477` burvestību.

Labotais Release `worldserver.exe` SHA-256 `E9C5E90A6053002F08B9BF668A7FD5C15E45FB31D33A19BB02A06EC04F45AB39` un tam atbilstošā saites karte uzlikti 12:42:38 pie apturēta lietotāja servera. Abas mijiedarbības metodes ir gala saites kartē. Fiksēts jauns diagnostikas sākuma stāvoklis. Klienta pārbaude pēc otrā būvējuma vēl nav apstiprināta.

Lietotāja 12:52 spēles mēģinājumā karogs ieskaitījās un varēja nodot `40077`, saņemt apbalvojumu un pieņemt `Enter the Illidari: Ashtongue` (`40378`). Datubāzē pārbaudīta apbalvotā un aktīvā kvesta rinda. Klientā pēc ieskaites karogs joprojām bija klikšķināms. Sekojošais labojums uzreiz nosūta atjaunotos karoga laukus konkrētajam spēlētājam. Serveris klientam noņem mijiedarbības un mirdzuma karodziņus un nosūta `NOT_SELECTABLE`, ja attiecīgajam spēlētājam ir karoga ieskaite vai apbalvots `40077`. Stāvokli nosaka no saglabātā kvesta pēc atkārtotas ieiešanas. Karogs paliek redzams, un cita spēlētāja lietošanu šī izmaiņa neietekmē. Klientā jāapstiprina jaunais stāvoklis pēc ieskaites un atkārtotas ieiešanas.

Release būvējums (worldserver SHA-256 A8422CA35FCC6AD12C54B1B6C5393ACB4C9CD10F345CC5C3236290B27A3547F9) un abi karoga koda ceļi pārbaudīti gala saites kartē. Būvējums sagatavots izvietošanai pēc lietotāja servera apturēšanas. Klienta pārbaude pēc izvietošanas vēl vajadzīga.

## Ashtongue vārtu aktivators

Belleria saņēma Sevis ziņu par uzlādētajiem vārtiem, savukārt kvests `40378` palika pie 0/1. Darba objekts `241751` (spawn `131213`, Mardum) ir goober ar Lock `93`, personīgu lietošanu un pastāvīgu `IN_USE` bitu. Vecais skripts gaidīja `GossipHello(true)`, ko parastās goober lietošanas ceļš nesasniedza. Mijiedarbības atskaite vien atgriezās uzreiz. Migrācija `20` noņem aizņemto stāvokli konkrētajai veidnei. Atskaite sāk parasto Opening burvestību `6477`. Faktiskajā lietošanā derīgam spēlētājam ieskaita `88872`, pēc apstiprinātas ieskaites palaiž ainu `189261` un saglabā esošos personīgos izsaukumus. Atkārtota lietošana pārbauda saglabāto objektīvu. Pārējo vārtu skripti šajā reizē netika mainīti.

Release saites kartē `GossipUse` ir no pārbūvētā `scripts:mardum.obj`. Kanoniskā izpildfaila SHA-256 ir `13B30D0084E0EC26A98A478ECA988CB0824D91B9677CB20A335B0A0C4863B61F`. Atbilstošā migrācija ievietota atjauninātājā pēc servera apturēšanas. Pirms klienta retesta saglabāti žurnālu izmēri un avāriju saraksts. Lietotāja vadītā pārbaude vēl jāveic.
