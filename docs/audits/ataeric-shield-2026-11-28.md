# Transdimensional Warfare: Chapter III

Archmage Ataeric sākotnējā novietojuma `254632` SmartAI saite no vairoga veda uz patstāvīgu palīgu taimeri. Palīgu taimeris savukārt norādīja uz patstāvīgo 120 sekunžu pašnogalināšanās taimeri. SmartAI tās noraidīja kā nederīgas LINK rindas. Pats pēdējais taimeris varēja izpildīt `45769` ieskaiti, kamēr palīgu nāves nebija reģistrētas.

Projekta Legion 26972 dati dod `87251` Arcane Shield, `87253` īsta Arcane Fiend `45766` izsaukšanai un `87249` Arcane Expulsion. Pēdējās burvestības efekts 1 dod 5% enerģijas. `85376` dod `45769` ar standarta grupas notikuma kredītu, un kvests `27518` prasa vienu tādu ieskaiti. 2014. gada ieraksts pārskatīts trīs sekunžu kadros visā 3 minūšu un 44 sekunžu secībā. Tajā redzama kauja ar palīgiem, runu secība, vairoga sabrukums, pēckaujas pāreja un nodošana pie Sylvanas. Tekstu ID apstiprināti projekta 26972 hotfix datos. [Kvesta iziešanas ieraksts](https://www.youtube.com/watch?v=N6LQUACpVQc)

## Labojums

- Sākotnējais Ataeric uzceļ īsto vairogu. Cīņu sāk tuvumā esošs spēlētājs ar nepabeigtu `27518`. Pirmā izsaukuma aizkave un atkārtošanās saglabātas 3,5 un 7 sekundes. Līdz trim dzīviem fiend var būt vienlaicīgi, un dzīvā izsaukuma ilgumu joprojām nosaka pati `87253`.
- Enerģija palielinās tikai pēc īstas šīs cīņas fiend bojāejas tuvumā un redzamībā. Der īpašnieka vai viņa atbilstoša grupas biedra sitiens, arī caur petu vai sargu. Dabiska enerģijas atjaunošanās ir izslēgta, un spēlētāja nāvējošs trāpījums vairogotam burvim tiek apturēts.
- Katra derīgā nāve izpilda oriģinālo `87249`. Burvestības enerģijas efekts skar tikai palīga īsto izsaucēju šā izsaukuma laikā. Pats efekts dod 5%. No 20 līdz 80 procentiem runu vada 26972 teksti. Pie 100% noņem vairogu, izpilda standarta pašnogalināšanās burvestību `87255` un pēc tās īstās nāves vienreiz ieskaita grupai. Kvesta atmešana vai ilgstošs dalībnieka zudums atiestata vairogu un noņem cīņas palīgus.

## Pārbaude un izvietošana

- `Test-QuestObjectLootNative.ps1 -Ataeric` pārbaudīja īsto novietojumu, noraidījumu bez kvesta, 150 sekunžu gaidīšanu bez enerģijas, pārāk tālu kritušu fiend, katru 5% soli, spēlētāja, grupas biedra un peta sitienus, nesaistīta spēlētāja sitienu, 100% nāvi ar grupas ieskaiti un atmešanu ar jaunu mēģinājumu. Testā attāluma gadījumiem pārvietoti īstie izsauktie tēli. Spēles klients, pilna kustība līdz palīgiem, Daschla pēckaujas uzvedība, sākotnējā 85360/85361 fāžu pāreja un pilna nodošana pie Sylvanas paliek atsevišķas pārbaudes.
- Agrīnā testa palaišana avarēja neielādētu proc datu dēļ, un privātā MariaDB tajā pašā laikā redzēja pārtrūkušus testa savienojumus. Testa inicializācija papildināta ar attiecīgajām tabulām un peta frakciju. Gala pārbaude izpildīta ar kodu 0 un bez jauniem avāriju, DB kļūdu vai servera brīdinājumu ierakstiem.
- Kanoniskais serveris ielādēja migrāciju `2026_11_28_16_ataeric_shield.sql` ar statusu `RELEASED` un SHA-1 `551EAA5AA4A862FED70F24C1F752E2E04B353238`. Dzīvajos datos apstiprinātas abas C++ piesaistes, astoņi teksti un piecu veco SmartAI rindu noņemšana. Serveris sasniedza gatavību un izslēdzās ar kodu 0 bez jaunām kļūdām. Gala saistījuma kartē ir abas realizācijas. Kanoniskā `worldserver.exe` SHA-256 ir `0FA5E4B5AE89A1A70C1F22EA0FAD715FF073C29EFAC71BAC9FAB7B8A0566EE47`.
