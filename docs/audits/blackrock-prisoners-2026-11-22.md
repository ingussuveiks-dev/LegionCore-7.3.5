# Prisoners of War (26646)

Redridge Holding Pen `204441/1379` lieto spēles slēdzeni `1930` ar Blackrock Holding Pen Key `59261` un burvestību `81515`. Atslēga ir attiecīgā laupījuma maisiņā `204389/105634`. Būrī ir septiņi īstie gūstekņi `43572` ar novietojumiem `39156–39162`. Esošais SmartAI atbrīvo gūstekņus un dod `43574` ieskaiti pēc būra atvēršanas.

Sākotnējā servera zonde ar tukšu būri saņēma ieskaiti no parasta klikšķa bez atslēgas. Tagad poga atveras pēc atslēgas burvestības, ja spēlētājam aktīvs kvests, atslēga ir somā un būrī vēl ir dzīvs, atbrīvojams gūsteknis. Saglabāta oriģinālā atvēršanas, gūstekņu un ieskaites secība. Bravo Company Field Kit `60384` lieto spēles auru `82580` un darbību joslu `278`. Aura tagad prasa nepabeigtu kvestu, pareizo apgabalu un līdzi nestu komplektu.

`Test-QuestObjectLootNative.ps1 -BlackrockPrisoners` pārbaudīja Keeshan pieņemšanu pēc `26640`, komplekta darbību, atteikumu bez atslēgas un ar svešu vai tukšu būri, faktiskās atslēgas ceļu, vienreizēju ieskaiti un nodošanu. Atkārtoti pārbaudīti arī `RavagerCage`, `AkidaCages` un `RedridgeSeaforium`. Klientā vēl jāpārbauda maisiņa izlaupīšana, gūstekņu kustība, komplekta josla un pilna Redridge secība.

Migrācija `2026_11_22_53_blackrock_holding_pen.sql` divreiz tika sekmīgi piemērota izolētajā MariaDB. Darba MariaDB astoņas pārbaudītās rindas un `RELEASED` kontrolsumma sakrita. Kanoniskais `worldserver.exe` sasniedza gatavību izolētajā datu kopijā un korekti izslēdzās. Izvietotā faila SHA-256 bija `5B828A12E53D663D387134E192617BDDC2236CD4FC30AF9A0BAC64E34D67E483`.

Pārbaudēs nebija jaunu avāriju, `DBErrors.log` ierakstu, brīdinājumu vai assertion kļūdu. Ar pārbaudītām kontrolsummām saglabāti 138 diagnostikas faili mapē `build-extractors/bin/Release/fixed/blackrock-prisoners-2026-11-22/`. Izņemti 2 178 727 378 baiti pagaidu failu. Iepriekšējais kanoniskais binārais fails paliek lokālajā atkāpšanās kopijā.
