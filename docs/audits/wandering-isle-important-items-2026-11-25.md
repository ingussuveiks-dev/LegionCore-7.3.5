# Items of Utmost Importance (29424)

No Merchant Lorvo pieņemtais kvests izmanto Amberleaf Scamp `54130` parasto līķu laupījumu. Ar sešiem oriģinālajiem izvietojumiem pārbaudīts, ka nāve pati nepiešķir `72071`, katrs līķis piedāvā vienu krājumu un viens un tas pats `CMSG_LOOT_ITEM` nevar to izsniegt divreiz. Atlīdzība ir pieejama ar visiem sešiem krājumiem, un Lorvo tos patērē nodošanā. Paralēlais The Missing Driver kvests palika nepabeigts. Tika izmantoti esošie servera un datubāzes dati.

Pirmajos divos pārbaudes mēģinājumos tika nolasīts personīgais laupījuma konteiners. Šis parastais līķis kvesta laupījumu glabā koplietotajā konteinerā ar piekļuves sarakstu katram spēlētājam. Labotā pārbaude izmantoja to pašu konteinera GUID un vienuma numuru, ko izmanto servera izsūtītā koplietotā laupījuma atbilde. Iepriekšējie un atkārtotie rezultāti neatstāja jaunas avārijas, servera brīdinājumus vai DBErrors ierakstus.

`Test-QuestObjectLootNative.ps1 -WanderingFirstLesson` pabeidza arī līdzšinējās iesācēju apmācības, tempļa un Jaomin pārbaudes. Ar kontrolsummām saglabāti 27 diagnostikas faili. Izņemti 27 pagaidu būves faili, kopā 1 092 299 988 baiti. [Diagnostika](../../build-extractors/bin/Release/fixed/2026-11-25-wandering-isle-important-items/)

Laupījuma loga un nodošanas attēlojums spēles klientā vēl jāapstiprina.
