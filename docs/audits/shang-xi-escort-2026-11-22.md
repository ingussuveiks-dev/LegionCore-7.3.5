# Worthy of Passing (29787)

Esošajai `105333` izsaukuma rindai 5829. apgabalā kvestam bija jābūt gan aktīvam, gan jau nodotam. Abi statusa filtri tagad atbilst pieņemtam kvestam. Saglabāta Legion burvestība, izsaukuma properties `3276`, Master Shang Xi `56159`, visi 12 sākotnējie maršruta punkti un saņēmējs `55672`.

Shang Xi un Guardian of the Elders `56274` pieder vienam pieņemtam kvesta mēģinājumam. Skripts atver esošos vārtus un turpina ceļu pēc īsta sarga nāves notikuma. Nogalināšanas kredīts nāk no pamata spēles ceļa. Ja dzīvs sargs pazūd, eskorts gaida nākamo mēģinājumu, nepārsniedzot trīs izsaukumus. Ja sargu uzvar pirms 11. punkta, vēlākā pauze vairs neaiztur skolotāju. Atmešana noņem attiecīgā spēlētāja abus aktierus.

`Test-QuestObjectLootNative.ps1 -ShangXiEscort` apstiprināja aktīvo zonas burvestību, divu spēlētāju privātos izsaukumus, dzīva sarga pazušanu ar atkārtotu izsaukšanu, īstu mājdzīvnieka nogalināšanu, vārtu stāvokli, agras uzvaras turpinājumu un atmešanu. Ceļa punktu notikumi tika vadīti izolētā servera pārbaudē ar sākotnējām koordinātēm. Pirmās palaišanas atklāja gan pretrunīgo datu filtru, gan testa dalībnieka trūkstošo zonas iestatījumu. Gala pārbaudē avārijas un jaunas neizskaidrotas žurnāla rindas netika atrastas. Klientā vēl jāpārbauda visa pastaiga, izeja pa kopīgajiem vārtiem un faktiskā nodošana.

Gala Release SHA-256 `2C9BDBD992364122E62065F686DA88B83CAEC134E1FD4C1EB9886E1DDD128448`. Saišu kartē pārbaudīti `Map.obj` un `zone_the_wandering_isle.obj`. Staged un kanoniskais serveris sasniedza `ready` izolētajā MariaDB un korekti izslēdzās. Produkcijas migrācijas galarezultāts un kontrolsumma salīdzināti ar pārbaudīto SQL rindu.

Diagnostikas kopijas ar pārbaudītām SHA-256 kontrolsummām ir `build-extractors/bin/Release/fixed/shang-xi-escort-2026-11-22/`. Noņemti 655 222 596 baiti pagaidu failu, saglabājot iepriekšējo Release atjaunošanai.
