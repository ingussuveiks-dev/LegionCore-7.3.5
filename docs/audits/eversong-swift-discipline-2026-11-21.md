# Swift Discipline Eversong Woods

Kvestu `9066` pieņem un nodod pie Instructor Antheol `15970`. Pieņemšana dod vienu *Disciplinary Rod* `22473`, kas lieto burvestību `27907`. Divi esošie burvestības nosacījumu zari atlasa Apprentice Ralen `15941` un Apprentice Meledor `15945`. Katram ir sava SmartAI burvestības trāpījuma rinda, savs kvesta mērķis un dzīvs spawns kartē `530`.

Ar sākotnējiem nosacījumiem somā palicis zizlis darbojās uz mācekli bez aktīva kvesta un atkārtoti uz mācekli, par kuru jau bija saņemts kredīts. Migrācija katram esošajam mērķa zaram pievieno prasību pēc nepabeigta kvesta un attiecīgā trūkstošā kredīta. Abu mācekļu SmartAI un standarta kredīta apstrāde saglabāti. Pēc abu kredītu saņemšanas zizlis jānodod pie Antheol.

## Pārbaudes un izvietošana

- `Test-QuestObjectLootNative.ps1 -SwiftDisciplineBaseline` ar vecajiem nosacījumiem atkārtoja abus nederīgos metienus. `-SwiftDiscipline` ielādēja faktiskās Source 17 rindas un abu mācekļu SmartAI notikumus. Pārbaudīts metiens uz instruktoru, metieni pirms pieņemšanas un pēc pabeigšanas, atteikta atkārtota darbība uz to pašu mācekli, abi atsevišķie kredīti, otrs spēlētājs, standarta nodošana ar zižļa izņemšanu, atmešana un atkārtota pieņemšana. Native sagatavē abi mācekļi ir vienuviet, lai pārbaudītu burvestības mērķa atlasi. Ražošanas nolasījumā abi sākotnējie spawni palika savās vietās.
- Izmaiņa ir datubāzē. Kanoniskais `worldserver.exe` ar SHA-256 `45A0CC91424AF9A73B3AFEA7B6EDAA2AE9E20D5F3E203B3A20D844454DE76A16` ar migrēto izolēto pasauli startēja un korekti apstājās. Migrācija `2026_11_21_43_eversong_swift_discipline.sql` ar SHA-1 `87D544A4CBB8EA53CAED6A08821ADAD4C768B31E` piemērota `legion_world` un reģistrēta kā `RELEASED`. Ražošanas nolasījumā mainījās tikai četras jaunās nosacījumu rindas.
- Pārbaudēs, palaišanā un importā nebija jaunu avāriju, `DBErrors.log` ierakstu vai neizskaidrotu brīdinājumu. Saglabāts 31 diagnostikas fails ar pārbaudītām SHA-256 kontrolsummām zem `build-extractors/bin/Release/fixed/eversong-swift-discipline-2026-11-21/`. Izņemti 18 pagaidu faili jeb 714 298 974 baiti. Kopējā audita datubāzes sagatave saglabāta. Uzdevuma procesi korekti apstājās.

Klientā paliek ceļš pie abiem mācekļiem, burvestības attēlojums un atlīdzības logs pie Antheol. Grupas kredīta uzvedība saglabā esošo SmartAI ceļu un šajā pārbaudē nav izspēlēta.
