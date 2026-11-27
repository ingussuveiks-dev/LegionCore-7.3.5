# Rapier of the Gilnean Patriots

Lorna Crowley `37783` pēc `Push Them Out` dod `24904` un oriģinālo rapieri `50334`. 26972 priekšmets izmanto `71388` ar diviem `TARGET_UNIT_DEST_AREA_ENTRY` efektiem. Vecā Livery Outpost rinda vienlaikus prasīja atlīdzināta kvesta statusu `64` un statusu kopu `11`. Vecā servera zonde nolasīja šo rindu un ar faktisko Lorna izsniegto priekšmetu atkārtoja atteikumu aktīva kvesta laikā.

World37 atļauj lietošanu aktīva, nepabeigta `24904` laikā Livery Outpost `4842` un Gilneas City `4755`. Abu efektu mērķu tabulā ir kaujas dalībnieku ieraksti no vietējā DestinyCore vēsturiskās cīņas. Mērķim jābūt dzīvam un ar neitrālu vai labvēlīgu attieksmi. Pirms skripta piesaistes zonde konstatēja, ka SourceType 13 mērķa atlase var izvēlēties arī cita fāzes stāvokļa aktieri ar pareizo ierakstu. Abi efekti tagad papildus izslēdz aktierus ārpus burvestības lietotāja fāzes.

Labotā zonde atkārtoja parasto kvesta pieņemšanas paketi un priekšmeta lietošanu abās kaujas vietās. Abus efektus saņēma izvēlētais kaujas sabiedrotais. Pretinieks, blakus esošais nesaistītais NPC, pats spēlētājs un citas fāzes sabiedrotais palika neietekmēti. Pārbaudīts aizliegums ārpus kaujas vietām un pēc pabeigšanas, kā arī priekšmeta noņemšana pēc atmešanas un atkārtota saņemšana. Rapieris nepiešķīra uzvaras ieskaiti.

Gala saite izmantoja atjaunoto `scripts.lib` un esošo `game.lib`. Izvietotajam `worldserver.exe` SHA-256 ir `4F7DF518D017F926D64CAE354C5E5378D46B7B5DB69C906FA2B2F357EB9E54B9`. World37 tika pārbaudīta divkāršā piemērošanā izolētajā datubāzē. Darba worldserver sasniedza gatavību un korekti apstājās. Gala pārbaudēs nav jaunu avāriju, servera brīdinājumu, kļūdu vai papildinājumu `DBErrors.log`.

Darba atjauninātājā World37 ir `RELEASED` ar SHA-1 `2BB08E28251D11618B2C9F38BC9C01AEC64A40E0`. Svaigā 1.–20. līmeņa atlasē ir 4 865 ieslēgti kvesti, 178 sasaistīti skripti Release failā un nulle konstatētu savstarpēji izslēdzošu tā paša kvesta statusa nosacījumu. Ar kontrolsummām arhivēti 132 diagnostikas un pārbaudes faili. Noņemti 145 atjaunojami pagaidu faili (2 048 634 028 baiti), saglabājot iepriekšējā laidiena atkopšanas kopiju un esošo jaukto kļūdu žurnālu. [Pārbaudes un diagnostika](../../build-extractors/bin/Release/fixed/2026-11-27-gilneas-rapier/)

Krennan kaujas sākums, cīņas viļņi, uzvaras ieskaite un WoW klienta attēlojums vēl jāapstiprina. [Kaujas avotu audits](gilneas-battle-sources-2026-11-27.md)
