# Cluster Cluck (24671)

Lost Isles kartē ir 95 Wild Clucker (`38111`) novietojumi. Īstais Remote Control Fireworks (`52712`) izmanto `71170`, un kvests iedod vienu priekšmetu. Priekšmeta burvestībai trūka savas mērķa pārbaudes. Esošais nosacījums attiecās uz NPC klikšķi. Tagad burvestībai vajadzīgs aktīvs kvests, priekšmets somā un dzīva Wild Clucker.

`npc_wild_clucker` izmanto esošos 12 ceļa punktus. Tas dod kredītu `38117` tikai pēc veiksmīgas skrējiena uzsākšanas. Kamēr vista skrien, tā nedod atkārtotu kredītu tam pašam spēlētājam vai citam kvestu pildošam spēlētājam. Saglabāti oriģinālie skaņas, salūta un pazušanas izsaukumi.

`Test-QuestObjectLootNative.ps1 -ClusterCluck` izmantoja faktiskās Release bibliotēkas un izolēto MariaDB. Pārbaudīta kvesta priekšmeta piešķiršana, noraidījums bez kvesta un uz cita mērķa, divu spēlētāju izmēģinājums ar vienu vistu, atmešana un atkārtota pieņemšana, desmitā ieskaitīšana un kvesta priekšmeta noņemšana nodošanas brīdī. Datu migrācija divreiz izpildīta bez dublikātiem.

Release uzstādīts ar SHA-256 `4753A197FA452A7817B3C07125BEEC1A2E2BDBC2F5D44569200F4FEF7B8A28D9`. Gala saites karte apstiprināja `npc_wild_cluckerAI::SpellHit` no `scripts:lost_isles.obj`. Staged un kanoniskais Release sasniedza `ready` ar izolēto datubāzi. Darba MariaDB apstiprinātas septiņas datu rindas un `RELEASED` ieraksts. Pārbaudēs nebija jaunu avāriju, DBErrors vai neizskaidrotu kļūdu. Faktiskā animācija, skaņa, skriešanas ceļš un nodošana pie Hobart vēl jāpārbauda klientā.
