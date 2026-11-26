# Reģionālās petu apmācības ķēdes

Vecajā laidienā Murog `47764` pieņēma dialoga izvēli `14991/1`, izdziedināja petu un pabeidza `31589` spēlētājam ar 999 vara monētām. SmartAI izsauca dziedināšanu un ieskaiti pirms dialoga apmaksas pārbaudes. Reģionālajos dialogos tā pati secība bija piesaistīta staļļmeistariem un petu instruktoriem. [Murog un Varzok ķēde](https://pd.zroot.it/quests/31589)

24 dialogu izvēles visās 12 `On The Mend` ķēdēs izmanto `133994` pēc maksas un dialoga pārbaudes. Par sekmīgu pakalpojumu iekasē 1000 vara monētu. Kvesta ieskaiti saņem spēlētājs ar aktīvu kvestu pēc faktiskās burvestības iedarbības pie kvestā nosauktā staļļmeistara. Shelby un Seriadne izmanto esošo staļļmeistaru izvēlni `9821`. Bezzil ārstēšanas izvēle pievienota viņa preču izvēlnei `12106`. Agrāk pieņemtais `31554` var saņemt Seriadne ieskaiti; parasta piedāvājuma pierādījuma šai versijai joprojām trūkst. [Dziedināšanas migrācija](../../sql/updates/world/2026_11_26_40_regional_pet_tutorial.sql)

| Instruktors | Atjaunotie sākumi |
| --- | --- |
| Grady Bannson `63075` | `31548`, `31549`, `31551`. [Learning the Ropes](https://pd.zroot.it/quests/31548) · [On The Mend](https://pd.zroot.it/quests/31549) · [Got one!](https://pd.zroot.it/quests/31551) |
| Valeena `63070` | `31552`, `31553`, `31826`. [Learning the Ropes](https://pd.zroot.it/quests/31552) · [On The Mend](https://pd.zroot.it/quests/31553) · [Level Up!](https://pd.zroot.it/quests/31826) |
| Will Larsons `63083` | `31582`, `31583`. [Learning the Ropes](https://pd.zroot.it/quests/31582) · [On The Mend](https://pd.zroot.it/quests/31583) |
| Matty `63086` | `31586`, `31587`. [On The Mend](https://pd.zroot.it/quests/31586) · [Got one!](https://pd.zroot.it/quests/31587) |

Piedāvājumi atjaunoti pie tiem pašiem instruktoriem, kuri 7.3.5 datos pieņem nodošanu. MoP Classic kataloga saites palīdz apstiprināt sākuma aktierus. Kvesta priekšnoteikumus, mērķus, atlīdzības un novietojumus ņēma no Legion datubāzes. [Pēdējo divu piedāvājumu migrācija](../../sql/updates/world/2026_11_26_41_regional_pet_capture_offers.sql)

Ar 1. līmeņa tēliem pārbaudītas 24 dialogu izvēles un visas 12 pārejas no `Learning the Ropes` atlīdzības līdz `On The Mend` nodošanai un `Level Up!` pieņemšanai. Servera zonde pārbaudīja nepietiekamu atlikumu, nederīgu un atkārtotu dialoga paketi, bloķētu burvestību, sekmīgu dziedināšanu, maksu un ieskaiti. Pirms otrās migrācijas Grady un Matty pēc `Level Up!` balvas noraidīja `Got one!` pieņemšanu. Pēc migrācijas abu kvestu parastā pieņemšana un vienreizējā atlīdzība strādāja. Pēdējā posma zonde izmantoja progresa ieskaites `65876` un `65356` kā ievadi. Petu kaujas, tveršanas animācija un spēlētāja žurnāls WoW klientā vēl jāpārbauda. Atkārtoti izgāja arī Bert un abu frakciju jubilejas pārbaudes. [Servera zonde](../../tools/tests/OnTheMendNative.cpp)

Abas migrācijas divkāršā izpildē deva vienādus rezultātus. Laidiena EXE SHA-256 ir `2F328CC19ABEA8534EBB1F22BFCEDC6FAB3F0D4E7831DB7019818234741F2783`. Gala saite izmantoja `game.lib` un `scripts.lib` no to Release mērķiem. Darba updater migrāciju SHA-1 ir `82D72DE089CFD642C7211400C98B762A9F1D6A59` un `E39696D803DE576A56B583F91EB29778FD8370C3`. [Pārbaudes arhīvs](../../build-extractors/bin/Release/fixed/2026-11-26-regional-pet-tutorial/manifest.json)

Abās gala palaišanās serveris sasniedza gatavību un korekti apstājās bez jaunām avārijām, DBErrors vai servera brīdinājumiem. Izolētā MariaDB korekti apstājās. Agrīnie testa mēģinājumi atklāja atšķirību starp izolētās un darba datubāzes `rank` kolonnu, kā arī nepilnīgi ielādētu zondes priekšnoteikumu. Gala migrācija izmanto abām shēmām kopīgās kolonnas, un zonde ielādē saglabāto priekšnoteikumu.

Ar kontrolsummām arhivēti 276 pārbaudes faili. Noņemti 203 atjaunojami testa un aizstātā atkopšanas laidiena faili ar kopējo izmēru 3 485 610 507 baiti. Brīvā vieta uz C: pieauga par 3 424 788 480 baitiem. Pēc tīrīšanas pārbaudīti abi darba izpildfaili, konfigurācijas, aktīvie žurnāli un atstātais atkopšanas EXE/MAP pāris. [Tīrīšanas rezultāts](../../build-extractors/bin/Release/fixed/2026-11-26-regional-pet-tutorial/cleanup-result.json)
