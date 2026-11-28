# Axis of Awful Loch Modan

Bluegill Representative `44263` saņēma signālu no Mosshide Representative `44262` un uzsāka `4426300` uzbrukuma sarakstu. Tā vienlaikus mēģināja izpildīt neesošu `LINK 1`, lai gan 26972 datos jau ir attiecīgais Bluegill kliedziens `44507`. Esošais Mosshide saraksts ieskaitīja kvestu un beidza cīņu pēc aptuveni piecām sekundēm.

2014. gada spēles ieraksts pārskatīts trīs sekunžu kadros visā 3 minūšu un 41 sekundes garumā, bet pati tikšanās pārskatīta vienas sekundes kadros. Ap `1:48` pēc feromonu lietošanas redzams `Mosshide Tagged 1/1`. Smakas reakcijai seko murloku atbildes, vadītāja kliedziens ap `2:04` un uzbrukums ap `2:09`. Pēc tam spēlētājs atgriežas pie Cannary. [Kvesta iziešanas ieraksts](https://www.youtube.com/watch?v=BAFYVUX6TY0)

## Labojums

- Projekta datos Cannary's Cache `60681` satur Clever Plant Disguise Kit `60502` un Potent Murloc Pheromone `60503`. To lietošanas burvestības `82788` un `82799` pārbaudītas 26972 datos. Tagad `82799` pārbauda nepabeigtu `26868` un īsto maskēšanos pirms lietošanas. Esošais native mērķa filtrs saglabā Mosshide `44262`. Faktiskais spēlētāja trāpījums vienreiz ieskaita `44266` un sāk tikšanos.
- Atjaunots Bluegill vadītāja `LINK 1` ar oriģinālo kliedzienu. Vadītāju `96528` uzrunā pēc sākotnējām reakcijām, un tas saglabā esošo trīs sekunžu uzbrukuma aizkavi. Abus citus novietotos Bluegill uzrunā atsevišķi, bez vadītāja kliedziena atkārtošanas. Atjaunoti arī divi Bluegill dialogi, divas Mosshide reakcijas un Tunnel Rat izsauciens no esošajiem `44499–44507` tekstiem. Pirms uzbrukuma Mosshide vairs nav imūns pret NPC, un noslēgumā saglabāta viņa sākotnējā skriptētā bojāeja.

## Pārbaudes

- Privātajā MariaDB migrāciju atkārtoti izpildīja ar vienādu rezultātu. Servera `Test-QuestObjectLootNative.ps1 -AxisOfAwful` izmantoja piecus īstos novietojumus, abu reālo priekšmetu lietošanas paketes, standarta atdzišanas un izpildes laiku. Pārbaudīts noraidījums bez maskēšanās un bez aktīva kvesta, tūlītēja 1/1 ieskaite, dialogu secība, vadītāja kliedziens pirms uzbrukuma, abu pārējo Bluegill iesaistīšanās un Mosshide beigas bez atkārtotas ieskaites. Priekšmeti testa tēliem ievietoti somā no identificētajiem oriģinālajiem priekšmetu ID.
- Agrīnais tests avarēja bez izolētā pieslēguma realm datubāzei. Testa inicializācija papildināta ar šo pieslēgumu. Vēlāk testā bija jāgaida reālā atdzišana un burvestības izpilde pirms skaitītāja nolasīšanas. Gala pārbaude beidzās ar kodu 0 bez jauniem avāriju, `DBErrors` vai servera brīdinājumu ierakstiem.
- Spēles klientā vēl jāpārbauda animācijas, skaņas, priekšmetu iegūšana no lādes un kvesta nodošana. Ierakstā papildus redzama murloku iznākšana no ūdens un kobolda aizbēgšana. Labojuma cīņa izmanto trīs sākotnēji novietotos Bluegill, un rekonstrukcijā no kobolda secības izmantots tā īstais izsauciens.

## Izvietošana

- Kanoniskais serveris ielādēja `2026_11_28_17_axis_of_awful_dialogue.sql` ar statusu `RELEASED` un SHA-1 `CA49F4C2A1373972955E942588A7344335A9A0A4`. Dzīvie nosacījumi un SmartAI rindas precīzi sakrita ar divreiz izpildīto izolēto migrāciju. Galvenais serveris sasniedza gatavību un izslēdzās ar kodu 0. Jaunu avāriju, `DBErrors` vai servera brīdinājumu nebija.
- Ataeric un Axis testa diagnostikas oriģināli un SHA-256 manifests saglabāti `build-extractors/bin/Release/fixed/2026-11-28-early-smartai-last/`. Pārbaudītas agrīno testa avāriju laika atbilstības privātās MariaDB pārtrūkušajiem savienojumiem. Pēc labojumu pārbaudēm privātā MariaDB korekti izslēgta. Izdzēsti 34 atjaunojami pagaidu faili jeb 924 933 896 baiti. Kanoniskā avāriju mape ir tukša, un jauktais 1347 baitu `DBErrors.log` saglabāts.
