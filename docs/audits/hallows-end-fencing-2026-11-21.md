# Fencing the Goods zirnekļu taka abām frakcijām

Alianses `29398` un Ordas `29416` maršrutos izvietoti 32 sezonāli zirnekļi no kurjeru atrašanās vietām līdz izsoļu namiem. Takas marķieru novietojums rekonstruēts pēc Legion mērķu reģioniem un esošo tā paša maršruta pilsētu NPC pozīcijām. Katrs zirneklis nedaudz pārvietojas ap savu marķieri.

- `53965` lieto native neredzamību `100512`, un `54148` lieto `100617`. Atbilstošā noteikšanas aura `100618` vai `100619` autocast tabulā ir piesaistīta pasākumam, attiecīgajam nepabeigtajam kvestam un zonas robežām. Ordas takas zonas ir Durotar un Orgrimmar. Alianses takas zona ir Stormwind. Abu efektu stiprumi 100 un 101 un tipi 14 un 15 pārbaudīti Legion DB2.
- Zirnekļi kalpo par norādēm. Kredīts joprojām rodas no īstā izsoļu rīkotāja sarunas `100529`, saglabājot jau atjaunoto grupas kredīta ceļu un parasto izsoles izvēli.
- `Test-HallowsEndFencingData.ps1` pārbaudīja tukšo sākuma stāvokli, visas 32 pasaules un pasākuma saites, abas neredzamības auras un trīs zonas piesaistes. Kanoniskais Release ar abām migrācijām ieslēdza un izslēdza pasākumu, korekti beidza darbu un neatstāja jaunas servera diagnostikas. Migrācija 33 ir piemērota `legion_world`, updater SHA-1 `2b710c433d9f4a866aea3135dcd656d896f6d8ff`.

7.3.5 klientā vēl jāpārbauda marķieru redzamība abos maršrutos, jo īpaši kuģī, kāpnēs un ieejās. Atjaunotā taka izmanto dažādus vietējos orientierus, un precīzais sākotnējais zirnekļu izvietojums pagaidām nav apstiprināts. [Fencing the Goods](https://warcraft.wiki.gg/wiki/Fencing_the_Goods)
