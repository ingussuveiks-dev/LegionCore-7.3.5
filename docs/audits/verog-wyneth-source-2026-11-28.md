# Verog the Dervish Northern Barrens

Kolkar Marauder `3275` nāves rindas `LINK 2` norādīja uz kaujas taimeri, kas izpilda `8014` Tetanus. Lokālajā TrinityCore 3.3.5 vēsturē `2012_06_04_00_world_sai.sql` tajā pašā vietā reiz bija Verog izsaukums pēc nejaušas parasta Kolkar nāves. Legion datos no šā paša vecā ceļa bija palikuši arī divi patstāvīgi izsaukumi uz `3274` un `3397`.

Legion 26972 kvesta `851` apraksts norāda uz Wyneth. Sākotnējais `34846` novietojums `260742` jau izsauc Verog `3395` pēc Wyneth nāves. Verog sāk uzbrukumu pēc izsaukuma un dod priekšmetu `5023` kā kvesta laupījumu. 2016. gada 81 sekundes ieraksts pārskatīts trīs sekunžu kadros un kauja vienas sekundes kadros. Wyneth iet bojā ap `0:38`, Verog tuvojas teltij ap `0:43`, un spēlētājs paņem priekšmetu ap `0:51`. Redzama arī atgriešanās pie kvesta devēja. [Kvesta iziešanas ieraksts](https://www.youtube.com/watch?v=0l4zy0pga2M)

Labojums izņem trīs precīzi identificētās parasto Kolkar nāves rindas un divas to vairs nesasniedzamās runas rindas. Dzīvajās kaujās paliek Rushing Charge, Strike, Tetanus, Battle Shout, Corruption un Bloodlust. Wyneth izsaukums, Verog uzbrukums un vietējā kvesta laupījuma iespēja paliek sākotnējos datos.

Privātajā MariaDB divkārša migrācijas izpilde atstāja 13 paredzētās rindas no 18. Salīdzinājums ietvēra abas Wyneth kaujas rindas, Wyneth nāves darbību un Verog agresiju.

`Test-QuestObjectLootNative.ps1 -Verog` izmantoja parasto Kolkar un Wyneth sākotnējos novietojumus. Pārbaudīja, ka parastās nāves neizsauc Verog, savukārt Wyneth nāve izsauc īsto `3395` paredzētajā vietā. Gan spēlētāja, gan peta nāvējošais sitiens izpildīja reālo Verog bojāeju. Spēlētājs saņēma `5023` no kopīgā līķa kvestu laupījuma, un bija iespējams pabeigto kvestu nodot. Faktiska nodošana klientā, grupas sadale un dialogu attēlojums paliek atsevišķas pārbaudes.

Agrīnā testa palaišana mēģināja atjaunot to pašu stekā glabāto Wyneth objektu un avarēja testa kartes iztīrīšanā. Izolētā MariaDB tajā brīdī reģistrēja atvienotus testa klientus. Katrs atkārtotais mēģinājums tagad izmanto savu karti, un kvesta priekšmets tiek pārbaudīts īstajā līķa laupījumā. Gala pārbaude beidzās ar kodu 0 bez jauniem avāriju, `DBErrors` vai servera brīdinājumu ierakstiem. Privātā MariaDB korekti izslēgta.

Kanoniskais serveris sasniedza gatavību un korekti izslēdzās ar kodu 0. Jaunu avāriju failu vai pievienotu kļūdu un brīdinājumu nebija. Datu migrācijai `2026_11_28_15_verog_wyneth_source.sql` ir statuss `RELEASED` un SHA-1 `BF482F1FA64BCD438639190AAEC95F6EA9D65EB1`. Dzīvajās rindās apstiprināti visi 13 pārbaudītie notikumi un sākotnējā `5023` rinda. Izvietotā `worldserver.exe` SHA-256 ir `3332DF0F2029F29890EC2EF745C6E443152CA790C0AEE013498AAF0FF3DDFB5B`.

Diagnostikas oriģināli un SHA-256 manifests glabājas `build-extractors/bin/Release/fixed/2026-11-28-verog-wyneth/`. Izdzēsti 16 atjaunojami testa faili jeb 377 699 275 baiti. Aktīvā avāriju mape ir tukša, un iepriekšējais jauktais `DBErrors.log` saglabāts 1347 baitu apjomā.
