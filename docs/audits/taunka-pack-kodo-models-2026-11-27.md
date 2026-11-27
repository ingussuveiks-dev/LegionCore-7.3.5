# Taunka Pack Kodo modeļi Northrend

Ceļā uz Aluneth kvestu `DBErrors.log` reģistrēja piecas neveiksmīgas `26160` ielādes Borean Tundra. Pieciem `26160` un četriem `26488` izvietojumiem bija norādīts `23646`, kas nav vietējā 26972 `CreatureDisplayInfo.db2` un darba `creature_model_info`. `ObjectMgr::ChooseDisplayId` dod priekšroku izvietojuma modelim. Abu NPC darba veidnēs ir `30562`, kurš atrodas abos avotos.

Migrācija noņem deviņas konkrētās `23646` norādes. Pasaules izvietojumi tagad izmanto esošo `30562` NPC veidni. MariaDB pagaidu kopijā pirmajā reizē mainījās deviņas rindas, bet otrajā reizē neviena. Darba datubāzē ir visas deviņas veidnes noklusējuma vērtības un atjauninājuma SHA-1 `397AFDD250B0AFF5CA95EAD69026303A3114C635`. Darba serveris palaidās un korekti apstājās bez jaunām avārijām vai kļūdu ierakstiem. Abu NPC grupu parādīšanās klientā vēl jāpārbauda.

Iepriekšējie pieci ieraksti paliek aktīvajā jauktajā `DBErrors.log` kopā ar pārējo diagnostiku. Pagaidu tabulas un servera palaišanas izvade saglabāta ignorētajā `.codex/taunka-pack-kodo-models-20261127` mapē.
