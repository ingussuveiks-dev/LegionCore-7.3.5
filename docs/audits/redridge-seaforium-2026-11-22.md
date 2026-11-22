# To Win a War, You Gotta Become War (26651)

Kvests seko Prisoners of War `26646`. Keeshan `43458/343676` nodrošina Bravo Company Field Kit `60385`. Spēles burvestība `82587` ar efektu `1` ieslēdz darbību joslu `279`, kurā ir lādiņa uzstādīšanas burvestība `82585` un vēl trīs komplekta darbības. Lādiņam vajag spēles fokusu `1676`. Datubāzē tam atbilst divi novietoti objekti `301307/106469/106470` pie Munitions Hut `43589/39071` un Blackrock Tower `43590/39103`. Abu mērķu esošā SmartAI ieskaite notiek pēc īsta burvestības trāpījuma.

Sākotnējā servera pārbaudē spēlētājs bez kvesta varēja aktivizēt saglabātu komplektu. Spēlētājs ar kvestu varēja tieši izsaukt lādiņu pie fokusa un saņemt būves ieskaiti, pirms komplekts bija aktivizēts. Labojums piesaista komplekta auru nepabeigtam kvestam Redridge. Lādiņa izsaukums papildus pārbauda līdzi nesto komplektu un auras faktisko darbību joslas efektu `1`. Abi fokusi, mērķa atlase un ieskaites notikumi paliek sākotnējā servera ceļā.

## Servera pārbaude

- `Test-QuestObjectLootNative.ps1 -RedridgeSeaforiumBaseline` atkārtoja abus sākotnējos gadījumus ar esošajiem fokusiem un mērķu SmartAI. Pēc labojuma `-RedridgeSeaforium` pārbaudīja priekšnoteikumu, komplekta saņemšanu no Keeshan un īstās pagaidu lādiņa darbības ielādi.
- Pārbaude noraidīja neaktīva kvesta komplektu, tiešu lādiņu bez auras, lādiņu ar palikušu auru bez komplekta un lādiņu ārpus abu fokusu sasniedzamības. Vienas būves atkārtots trāpījums nevirzīja otras būves mērķi. Pirmajā mēģinājumā ieskaitīja Munitions Hut. Atkārtotajā mēģinājumā vispirms ieskaitīja Blackrock Tower un tad Munitions Hut.
- Kvesta atmešana izņēma komplektu, auru un pagaidu lādiņa burvestību. Pēc atkārtotas pieņemšanas otra būve pabeidza kvestu. Keeshan `43611/260620` pieņēma atlīdzības paketi, un no somas izņēma komplektu. Klienta darbību josla, lādiņa efekti un pilns Redridge maršruts vēl jāpārbauda.

## Izvietošana un diagnostika

Migrācija `2026_11_22_52_redridge_seaforium.sql` divreiz pārbaudīta izolētajā MariaDB. Darba datubāzē 15 salīdzinātās rindas sakrita ar pārbaudītajiem datiem. Updater ieraksts ir `RELEASED`, SHA-1 `D3E0366D87F0FAF46A730C35428B081D3A139D07`.

Kanoniskais Release ar abām šīs kārtas migrācijām sasniedza gatavību un korekti izslēdzās. Binārā faila SHA-256 ir `C2892B81E56DA543C03566B421AFF2237A51039162EA39C59C6EF0B42B0DE052`. Pēdējās pārbaudes neradīja avārijas, jaunus `DBErrors.log` ierakstus, brīdinājumus vai assertion kļūdas. Sākotnējās zondes aprīkojuma ielādes kļūdu novērsa, ielādējot īstās NPC aprīkojuma veidnes pirms aktieru novietojumiem. Ar pārbaudītām kontrolsummām arhivēts 171 fails mapē `build-extractors/bin/Release/fixed/redridge-seaforium-2026-11-22/`. Pagaidu testa failu tīrīšana atbrīvoja 749 396 053 baitus.
