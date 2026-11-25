# Novecojušo labojumu tīrīšana

No repozitorija izņemts atsauktais StoreUI instalētājs. Tas jau bez nosacījumiem beidza darbību pirms failu izmaiņām. Featured kartēm serveris izmanto parasto režģi, kas pārbaudīts ar iepriekšējo labojumu `2026_09_23_314_use_standard_featured_store_cards.sql`. Novecojušās instalēšanas komandas izņemtas no dokumentācijas. Vecais instalētājs un atslēgšanas iemesls paliek Git vēsturē pie `68e46fd0f`. [Store labojuma dokumentācija](../battlepay-store-icons.md)

No lokālās izstrādes mapes izņemti divi aizstāti 21. novembra `worldserver.exe` un MAP pāri un divi pagaidu difi. Difu izmaiņas sakrīt ar Faronaar `57c1df233` un Adventure Guide `5ee6831fd` commitā iekļautajām izmaiņām. Kopā izņemti 6 faili, 329 803 555 baiti. Lokālā dzēšanas saraksta SHA-256 ir `147E5C7554FFB7A1C4BEC5879497BB100C0AE8C21A54C06A30D0B88366232889`. [Dzēšanas saraksts](../../build-extractors/bin/Release/fixed/2026-11-25-obsolete-level-fixes/cleanup.csv)

Pārbaudītas kanoniskā servera EXE, MAP un konfigurāciju kontrolsummas un jaunākā atkopšanas pāra identitāte. Aktīvo avāriju mape bija tukša, un tīrīšanas laikā servera žurnāliem netika pievienoti ieraksti. Lietotāja esošā klienta konfigurācijas rezerves kopija, regresiju avoti un SQL migrāciju vēsture ir pieejami.
