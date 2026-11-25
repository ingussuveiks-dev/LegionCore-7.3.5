-- Restore Legion artifact acquisition map guidance for the audited 36 routes.
-- Coordinate sources are the local 26972 QuestPOI DB2 data, matching TDB 735.00
-- rows where needed, and locally verified positions for quests without a native blob.
-- Idx1 is the server's coordinate key, independent of the displayed BlobIndex.
START TRANSACTION;

CREATE TEMPORARY TABLE artifact_poi_plan (
    QuestID INT NOT NULL, BlobIndex INT NOT NULL, Idx1 INT NOT NULL,
    ObjectiveIndex INT NOT NULL, QuestObjectiveID INT NOT NULL, QuestObjectID INT NOT NULL,
    MapID INT NOT NULL, WorldMapAreaId INT NOT NULL, Floor INT NOT NULL,
    Priority INT NOT NULL, Flags INT NOT NULL, WorldEffectID INT NOT NULL,
    PlayerConditionID INT NOT NULL, WoDUnk1 INT NOT NULL, AlwaysAllowMergingBlobs INT NOT NULL,
    TargetIdx1 INT NOT NULL, ChangeKind TINYINT NOT NULL,
    SourceCount INT NOT NULL, SourceHash CHAR(64) NULL,
    TargetCount INT NOT NULL, TargetHash CHAR(64) NOT NULL, VerifiedBuild INT NOT NULL,
    PRIMARY KEY (QuestID, BlobIndex, Idx1),
    UNIQUE KEY (QuestID, TargetIdx1)
) ENGINE=InnoDB;

INSERT INTO artifact_poi_plan VALUES
    (39247, 3, 6, 32, 0, 0, 1494, 1054, 1, 0, 0, 0, 0, 1045927, 0, 10, 1, 1, 'f803e823d65a02b75d42721fd2c86ac043ed591b43796736d8848762e593470c', 1, '99e1231763bc10a4610a97c0a9cfb74c91eecdc26c6de664f8b79bee70d3e69d', 26972),
    (39247, 3, 7, 32, 0, 0, 1494, 1054, 1, 0, 0, 0, 0, 1045927, 0, 11, 1, 1, '3ae8fb997a42f910bd1760bef5514e83ae59c48066de4b78700f3947f42db412', 1, '99e1231763bc10a4610a97c0a9cfb74c91eecdc26c6de664f8b79bee70d3e69d', 26972),
    (40385, 0, 7, 2, 282143, 103798, 1220, 1014, 10, 0, 2, 0, 0, 0, 0, 7, 0, 0, NULL, 12, '0bc1e381f5585d5e40b21be0f2fb18bb2fe1299c79ec16b680634683927373ce', 26972),
    (40385, 0, 8, 32, 0, 0, 1220, 1014, 10, 0, 0, 0, 42192, 1215578, 0, 8, 0, 0, NULL, 1, '12abd8a1f6cc35c16fd81fb1d942c9ad323614813945fa9c833b0fd447b09e5c', 26972),
    (40385, 1, 9, 32, 0, 0, 1220, 1017, 0, 0, 0, 0, 42191, 1215545, 0, 9, 0, 0, NULL, 1, '740029bf3f45ec8c1a64ed93222042307ce55449ae1350f547b695bcea678abd', 26972),
    (40385, 3, 3, -1, 0, 0, 1220, 1072, 0, 0, 2, 0, 48911, 0, 0, 10, 1, 1, '58d857c8ea741bf17069b0b23dc161f87c93327083b06d8dfd7f9ca23dbde28b', 1, '3d6b9a40f3b3b9ffd2479d5ce45a8159f8845f22c21f7f304e179e4ec8b64ddf', 26972),
    (40385, 4, 4, -1, 0, 0, 1220, 1072, 0, 0, 2, 0, 48912, 0, 0, 11, 1, 12, 'eada9eab2132192a275fc120e7ad79cad80e92c6b9630659197d140b8b858632', 1, '9a189902fea97351c99126da50b41faedbd31b9e407f62d48cd34d090878cf10', 26972),
    (40604, 0, 2, 32, 0, 0, 0, 32, 0, 0, 0, 0, 0, 1102602, 0, 2, 0, 0, NULL, 1, '90b89660bfb47b9323068422fee3fa95ef30f64660cc026100bdfd2ef5583a5a', 26972),
    (40710, 0, 1, 0, 281106, 101365, 0, 20, 0, 0, 2, 0, 0, 0, 0, 8, 1, 1, '3c8fcd2a14a0046c322bf39d301d923505b869fcc527696e70975607da0992b8', 12, 'c42401981ceb795d23eebb9c338daee2fdd3c81c8a290542141f0175cad1f8b3', 26972),
    (40783, 0, 2, 1, 281198, 101701, 0, 34, 0, 0, 0, 0, 0, 1119349, 0, 5, 1, 1, '953bd691407af31fe8f09480727ce98f8154c08bd8612c36e5dbb5fdc0afd152', 1, '11f95d9e04d32ba39ea8c0beb5aa7d2682d2c016fcef3b674245271bd0216b57', 26972),
    (40784, 0, 0, 1, 281200, 101702, 0, 34, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, NULL, 1, '134a6d9b5cdd66c05e4dbfd8d2b190ff922f2dec2f87ac7b2749f63840dfbaa3', 0),
    (40784, 0, 1, 32, 0, 0, 0, 34, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, NULL, 1, '66bdd6e9b305fbd2e9057653e9b81fe39daba9580f351db84536ce97cf1b9ffe', 0),
    (40793, 0, 2, 32, 0, 0, 1514, 1044, 0, 0, 0, 0, 0, 1098209, 0, 2, 0, 0, NULL, 1, '7a8252c92d681925a8a7ca162b76ad488dbefa48ab03e441a6a53890118529be', 26972),
    (40793, 1, 1, -1, 0, 0, 1514, 1044, 0, 0, 0, 0, 0, 1129231, 0, 3, 1, 1, 'f60b2e047930e144f1290f8349c57f0eda8aa1e92a019108cf6c1bdf26ffd411', 1, 'f7c2ca5ec730745fa5f15626ba4711db42d355a2b82031f424d171149a36f0e5', 26972),
    (40821, 0, 2, 32, 0, 0, 1107, 1050, 0, 0, 0, 0, 0, 1116642, 0, 2, 0, 0, NULL, 1, '0401b11cf9a33c280bc839437093a00c5014c3c1b61bd88a465abae14dc796f2', 26972),
    (40835, 0, 2, 32, 0, 0, 0, 32, 0, 0, 0, 0, 0, 1102602, 0, 2, 0, 0, NULL, 1, '90b89660bfb47b9323068422fee3fa95ef30f64660cc026100bdfd2ef5583a5a', 26972),
    (40847, 0, 4, 3, 281425, 102461, 0, 673, 0, 0, 2, 0, 0, 0, 0, 9, 1, 1, 'bc0b49d677d410236911ff4fc75949392e599e0886b77f845899f5baca437305', 1, 'b80d0aa8587c808abcb47fb74497b2e1350b64eac63b2b3737083781b350591e', 26972),
    (40849, 0, 1, 0, 281315, 102120, 0, 673, 0, 0, 2, 0, 0, 0, 0, 8, 1, 1, '18d4ec2d246acc522fa41132b9737408e94c51e6ceea51ad7b93f73abf2203df', 1, 'c4268cff36057a9ef4273c1ef21746ee5eb3e13cae7fb28b6adc614a981f5556', 26972),
    (40930, 0, 2, 1, 281402, 100821, 0, 34, 0, 0, 0, 0, 0, 1105746, 0, 5, 1, 1, 'c116f8a51a2e61166212004b5bd5869b1af6196c01c4b6cf47d1860296f3c5fa', 1, '564e189e04cf356d19aeb272f3eda583d09fc46b05d8c52002f0cf81ac190173', 26972),
    (40932, 0, 2, 32, 0, 0, 0, 32, 0, 0, 0, 0, 0, 1102602, 0, 2, 0, 0, NULL, 1, '90b89660bfb47b9323068422fee3fa95ef30f64660cc026100bdfd2ef5583a5a', 26972),
    (40933, 0, 3, 32, 0, 0, 0, 32, 0, 0, 0, 0, 0, 1102602, 0, 3, 0, 0, NULL, 1, '90b89660bfb47b9323068422fee3fa95ef30f64660cc026100bdfd2ef5583a5a', 26972),
    (41015, 0, 0, 32, 0, 0, 1512, 1040, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, NULL, 1, '7954632bc4dd64817e0384510b6fc65a3e08fcbd435d66bb3e65c6dcaf41b164', 0),
    (41017, 0, 0, 0, 281671, 46798, 1512, 1040, 1, 0, 2, 0, 0, 0, 0, 0, 2, 0, NULL, 1, '2917fc3ea3023a8740756ebfee68c8a2eb948dfbea62189a7f9d6a794ee97eed', 0),
    (41017, 0, 1, 32, 0, 0, 1512, 1040, 1, 0, 0, 0, 0, 0, 0, 1, 2, 0, NULL, 1, '7954632bc4dd64817e0384510b6fc65a3e08fcbd435d66bb3e65c6dcaf41b164', 0),
    (41105, 0, 6, 32, 0, 0, 1479, 1035, 1, 0, 0, 0, 0, 1047209, 0, 6, 0, 0, NULL, 1, '375f2ec73c6b63ee3453cc26e2159cfb292c268f24621072fd15a1e2f33926d1', 26972),
    (41105, 0, 2, 32, 0, 0, 1479, 1035, 1, 0, 0, 0, 0, 1047209, 0, 7, 1, 1, '1302d1c2517f6da845643261884ea5b9bb08fa118096c87f623607c1d6bb6dc4', 1, '375f2ec73c6b63ee3453cc26e2159cfb292c268f24621072fd15a1e2f33926d1', 26972),
    (41113, 0, 0, 32, 0, 0, 1513, 1068, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, NULL, 1, '94ab8c3cb6dcff92a280ce10c97f19c59942f0526c8fa18165786b0d8d05be41', 0),
    (41119, 2, 5, 32, 0, 0, 1494, 1054, 1, 0, 0, 0, 0, 1126593, 0, 9, 1, 1, 'f803e823d65a02b75d42721fd2c86ac043ed591b43796736d8848762e593470c', 1, '99e1231763bc10a4610a97c0a9cfb74c91eecdc26c6de664f8b79bee70d3e69d', 26972),
    (41119, 2, 6, 32, 0, 0, 1494, 1054, 1, 0, 0, 0, 0, 1126593, 0, 10, 1, 1, '3ae8fb997a42f910bd1760bef5514e83ae59c48066de4b78700f3947f42db412', 1, '99e1231763bc10a4610a97c0a9cfb74c91eecdc26c6de664f8b79bee70d3e69d', 26972),
    (41436, 1, 5, 32, 0, 0, 1220, 1018, 0, 0, 0, 0, 0, 1134796, 0, 5, 0, 0, NULL, 1, 'aef286b880ad030615a5ca9087650db7cd6c64dae607cec0ea297c387461ee79', 0),
    (41436, 0, 1, 0, 282419, 104352, 1220, 1018, 0, 0, 2, 0, 0, 0, 0, 6, 1, 1, '61ec1cd688289ac217bc97739af1eee021329964e105e885a9176b57dc460d96', 1, '7d7a2ab4a9a42895bc48accc62e15284c5a072773c1a9954e5e3742786214168', 26972),
    (41449, 0, 2, 32, 0, 0, 1220, 1018, 0, 0, 0, 0, 0, 1134508, 0, 2, 0, 0, NULL, 1, '5c5956276bdaa1ad5414c487b5e3d91b8226c087bbcccd978dc2b78f77de79cc', 0),
    (41626, 0, 2, 1, 282954, 105092, 571, 488, 0, 0, 0, 0, 0, 1140706, 0, 6, 1, 1, '26bf74fbdc6e5cee9f9dc4486817f992ba2542b2a809eb321ab7819abd4edb04', 1, 'aa658138fecb1759e9d0b6c06db461434d89e3fea3deba1df3be3c3eb6f529b3', 26972),
    (42001, 0, 2, 32, 0, 0, 1513, 1068, 2, 0, 2, 0, 0, 0, 0, 2, 0, 0, NULL, 1, 'a456c71c2499896abd3d978d1049330b912cc6d9bdb21ddc3fa357caf98cce08', 26972),
    (42002, 0, 2, 1, 283323, 105726, 571, 491, 0, 0, 2, 0, 0, 0, 0, 5, 1, 1, 'bf3becd8cb56858ff1e2d2224d45c1f95c1acdc31abb594face57c0d90724469', 12, '7904ad90e2d20b1c7e6b320542af7db7113adaf09bf28afbee177c7df963cb27', 26972),
    (42005, 0, 5, 4, 283335, 105788, 571, 491, 0, 0, 2, 0, 0, 0, 0, 5, 0, 0, NULL, 12, '5bec7faa728da8cb8dc45dc2c89d6585707257c0e747d843e221e81b7f2dae9e', 26972),
    (42005, 0, 6, 32, 0, 0, 571, 491, 0, 0, 0, 0, 0, 1145889, 0, 6, 0, 0, NULL, 1, 'd656ddfdff64456d7f1ba43acdbaf1d55c317e82339f44c273deb19be2b8ba29', 26972),
    (42006, 0, 2, 1, 283339, 105092, 571, 488, 0, 0, 0, 0, 0, 1140706, 0, 6, 1, 1, '02c1a29ff510d41d98eedea29fe7a2fd6fef9e6bdd68c8bd4b800388e4e12668', 1, 'aa658138fecb1759e9d0b6c06db461434d89e3fea3deba1df3be3c3eb6f529b3', 26972),
    (42074, 1, 2, 32, 0, 0, 1220, 1021, 0, 0, 0, 0, 0, 1149135, 0, 5, 1, 1, '476015b3acfde8fbd93c6948c08c61dff8c061333d075ec01e26ecf0196edf00', 1, 'd827f4cd7d5171eede096494fa3b54074b786426fed84380909dd8f4e8ca8300', 24330),
    (42074, 1, 3, 32, 0, 0, 1220, 1021, 0, 0, 0, 0, 0, 1149135, 0, 6, 1, 1, 'ca0dd23096fe3cf1b56d341788bc7d23c671bae2d5947c4961fa70d174ae7b4d', 1, 'd827f4cd7d5171eede096494fa3b54074b786426fed84380909dd8f4e8ca8300', 24330),
    (42120, 2, 2, -1, 0, 0, 1220, 1014, 10, 0, 0, 0, 44622, 1194329, 0, 10, 1, 1, '595ca68cbe14d2f689ecf0a531554d2cd3025e4effea755513a50bae0cc3a901', 1, '6c31b09fa2a910cf1114ebf282996529fff7b1e543505a45f8506d5180611a05', 26972),
    (42377, 0, 2, 1, 284134, 107294, 571, 488, 0, 0, 0, 0, 0, 1165985, 0, 13, 1, 1, '8b497f00ce281ce99f4ed5210600447ab4d2beda835436daac06f7f4f7d1ce39', 1, 'b7cb7a1981e92bca3abf1d03cff839b25663f0ae5e9f126df67725a31268fe4c', 24330),
    (42377, 1, 3, 1, 284134, 107294, 571, 488, 0, 0, 2, 0, 0, 0, 0, 14, 1, 1, '56c07bf2273b61433a6b783316837dab2e6aa38f5672d63dc98b53155900bf70', 1, 'bdbc35e60e1eb48c85e075d5b64c424115b32dfea6480f9b955f1d59d967c641', 24330),
    (42377, 2, 4, 1, 284134, 107294, 571, 488, 0, 0, 0, 0, 0, 1166467, 0, 15, 1, 1, 'ad0bf13c66e0f46eb6f44d2f8bc74620a9280c93d16dcd3570265bf57a76fe29', 1, 'f0fe25cc6339890bffd56e29acdd34ae47ca7206c8d7aff9e4b01d22775a224e', 24330),
    (42377, 0, 5, 5, 284135, 107295, 571, 488, 0, 0, 2, 0, 0, 0, 0, 16, 1, 1, '75087845b17f10fb9cdc560ec291e5a3214f72a95c4a0918bfdd36362405207b', 12, '11df3cd90a76eac67326083c078de0b6166a86d36a0c21c9a62d0d57a92ea1fa', 26972),
    (42440, 0, 3, 32, 0, 0, 1220, 1018, 0, 0, 0, 0, 0, 1167428, 0, 3, 0, 0, NULL, 1, 'caa36f1d1e7b3367027e154b429608f27495da3688308de1230cddcaa7c07a82', 26972),
    (42477, 0, 3, 2, 284373, 107711, 0, 19, 0, 0, 0, 0, 0, 1170459, 0, 11, 1, 1, '54140c130450cea81d639fe2002073d726789978f3f60c7c3d9d955aeeaeb099', 1, '6a9112e8296dec007d58472f1d696da750a75d75ec33049f5b408dc42e5b1ffb', 26972),
    (42477, 1, 4, 2, 284373, 107711, 0, 992, 0, 0, 0, 0, 0, 1170459, 0, 12, 1, 12, 'e33c2047f5611987d9aae3f2689ef6ed8707168fbc37ec8885adb6739014be31', 1, '6a9112e8296dec007d58472f1d696da750a75d75ec33049f5b408dc42e5b1ffb', 26972),
    (42477, 0, 6, 32, 0, 0, 1513, 1068, 2, 0, 2, 0, 0, 0, 0, 13, 1, 1, '842310d32272df1caebbe53db50a9a0039c352865d79d30ca43aff06bfa8a2bd', 1, 'a456c71c2499896abd3d978d1049330b912cc6d9bdb21ddc3fa357caf98cce08', 26972),
    (42627, 1, 3, 1, 284637, 48824, 1220, 1014, 4, 0, 2, 0, 44077, 0, 0, 6, 1, 1, 'f8b61a69091c5e87eb33ef7d7755abf0f4378bda22f1cca085520c044f64ad66', 1, 'a7f58aea8b63ce7943e83286392e79e7e95f1e67056ad22fef05a2855ecb08f8', 26972),
    (42627, 0, 4, 32, 0, 0, 0, 30, 0, 0, 2, 0, 0, 0, 0, 7, 1, 1, '989264ded2595c8b152cb0ea6b74587f508313afbaec434b6405e18404dc69cb', 1, '9bb3d8709ca5ac54e4aaa91621fcd18df50328a1df512f6880c2d623e67919ed', 26972),
    (42765, 0, 3, 32, 0, 0, 870, 806, 0, 0, 0, 0, 0, 1199472, 0, 3, 0, 0, NULL, 1, '3961ab5112c989ec78d64a00cbd65e6a787b4f2f46dc3b8103ae2f280b1fdcaf', 26972),
    (42772, 0, 0, 0, 284747, 108814, 0, 22, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, NULL, 1, 'd92a4ce2ed5546156c363c0373ba69a6c420841925d56ae7a1428efb1319da85', 0),
    (42772, 1, 1, 0, 284747, 108814, 0, 22, 0, 0, 2, 0, 0, 0, 0, 1, 2, 0, NULL, 1, '96a2f424d5cd8707ab14145b02173627f2597b1fdebab7a4f086453cf9938941', 0),
    (42772, 2, 2, 0, 284747, 108814, 0, 22, 0, 0, 2, 0, 0, 0, 0, 2, 2, 0, NULL, 1, 'bd91bbacab3ca9c27cc15c4e176274bec7ca85806804b0f69e65ea7e7c1e6269', 0),
    (42772, 0, 3, 32, 0, 0, 0, 22, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, NULL, 1, 'dd85580ccd537345a4cd463b58936c98d93da4f1527d59437fdbe70e4764e458', 0),
    (43100, 0, 2, 5, 285059, 109693, 0, 22, 0, 0, 2, 0, 0, 0, 0, 9, 1, 1, 'b0568f4dfb5ea376573c652e71a541ad4c759aa85bfe6b400469698a2307da27', 12, '21c2630bc5d6e7829c5fbf98c327dd0c5e4bf28dc68461d686e4365c93fbf448', 26972),
    (43254, 1, 10, 32, 0, 0, 1220, 1014, 10, 0, 0, 0, 0, 1206181, 0, 10, 0, 0, NULL, 1, '7b637a703b4cb83bac298793e342f23efcbde6e6f22e1c1248fcc2d855d8af11', 24330),
    (43254, 0, 1, 0, 285114, 110523, 1220, 1014, 10, 0, 2, 0, 0, 0, 0, 11, 1, 1, 'd870b928502d204bac7707e0dcc366096a5cb7d57fb66678d42d73b51f02e5cf', 1, '2a148aeedebc87a15b712bfecbcfbef6da59dc5562ff1d86885af5dab46d1624', 26972),
    (43254, 0, 7, 32, 0, 0, 1220, 1014, 10, 0, 2, 0, 0, 0, 0, 12, 1, 1, '213659561936a40c90c33fd5e26b38a81b0408b63538032bc554864fff9583b8', 1, 'a0a7ffdfb2a9b65d9d783b91f834c0bfa1efe72173a683567b148b237356622c', 24330),
    (43254, 1, 8, 5, 285270, 110607, 1220, 1014, 10, 0, 2, 0, 0, 0, 0, 13, 1, 1, 'b8e0aa7a3f3865da89a17960a20f28b9720671307686f5180b94b886969dd1be', 1, '55f763a286470d586e455ecfc9b0836a2ccce3488d9e151205e063041d0f5d0b', 26972),
    (43254, 0, 9, 32, 0, 0, 1220, 1014, 10, 0, 2, 0, 0, 0, 0, 14, 1, 1, 'cfd0138d59afe3a285f5aa7e2209f2725d651a6424acf7ed43e6d77b2058c9a8', 1, 'a0a7ffdfb2a9b65d9d783b91f834c0bfa1efe72173a683567b148b237356622c', 24330);

CREATE TEMPORARY TABLE artifact_poi_coordinates (
    QuestID INT NOT NULL, Idx1 INT NOT NULL, Idx2 INT NOT NULL,
    X INT NOT NULL, Y INT NOT NULL, VerifiedBuild INT NOT NULL,
    PRIMARY KEY (QuestID, Idx1, Idx2)
) ENGINE=InnoDB;

INSERT INTO artifact_poi_coordinates VALUES
    (39247, 10, 0, 1288, -261, 26972),
    (39247, 11, 0, 1288, -261, 26972),
    (40385, 7, 0, -826, 4226, 26972),
    (40385, 7, 1, -811, 4235, 26972),
    (40385, 7, 2, -806, 4265, 26972),
    (40385, 7, 3, -806, 4288, 26972),
    (40385, 7, 4, -815, 4308, 26972),
    (40385, 7, 5, -833, 4316, 26972),
    (40385, 7, 6, -849, 4317, 26972),
    (40385, 7, 7, -871, 4309, 26972),
    (40385, 7, 8, -873, 4289, 26972),
    (40385, 7, 9, -871, 4273, 26972),
    (40385, 7, 10, -866, 4250, 26972),
    (40385, 7, 11, -852, 4224, 26972),
    (40385, 8, 0, -860, 4272, 26972),
    (40385, 9, 0, 3148, 3391, 26972),
    (40385, 10, 0, 4388, 5295, 26972),
    (40385, 11, 0, 4388, 5294, 26972),
    (40604, 2, 0, -10440, -2144, 26972),
    (40710, 8, 0, 2254, 2383, 26972),
    (40710, 8, 1, 2284, 2400, 26972),
    (40710, 8, 2, 2294, 2430, 26972),
    (40710, 8, 3, 2294, 2471, 26972),
    (40710, 8, 4, 2294, 2494, 26972),
    (40710, 8, 5, 2254, 2521, 26972),
    (40710, 8, 6, 2224, 2531, 26972),
    (40710, 8, 7, 2173, 2517, 26972),
    (40710, 8, 8, 2157, 2481, 26972),
    (40710, 8, 9, 2157, 2447, 26972),
    (40710, 8, 10, 2170, 2400, 26972),
    (40710, 8, 11, 2197, 2383, 26972),
    (40783, 5, 0, -10335, -487, 26972),
    (40784, 0, 0, -10338, -484, 0),
    (40784, 1, 0, -10333, -485, 0),
    (40793, 2, 0, 924, 3606, 26972),
    (40793, 3, 0, 722, 3570, 26972),
    (40821, 2, 0, 3124, 1105, 26972),
    (40835, 2, 0, -10440, -2144, 26972),
    (40847, 9, 0, -14340, 503, 26972),
    (40849, 8, 0, -14340, 500, 26972),
    (40930, 5, 0, -10367, -1257, 26972),
    (40932, 2, 0, -10440, -2144, 26972),
    (40933, 3, 0, -10440, -2144, 26972),
    (41015, 0, 0, 1411, 1349, 0),
    (41017, 0, 0, 1412, 1344, 0),
    (41017, 1, 0, 1411, 1349, 0),
    (41105, 6, 0, 796, 7229, 26972),
    (41105, 7, 0, 796, 7229, 26972),
    (41113, 0, 0, -813, 4698, 0),
    (41119, 9, 0, 1288, -261, 26972),
    (41119, 10, 0, 1288, -261, 26972),
    (41436, 5, 0, 4295, 7423, 0),
    (41436, 6, 0, 4300, 7413, 26972),
    (41449, 2, 0, 4289, 7426, 0),
    (41626, 6, 0, 3097, 516, 26972),
    (42001, 2, 0, -817, 4696, 26972),
    (42002, 5, 0, -72, -4967, 26972),
    (42002, 5, 1, 0, -4953, 26972),
    (42002, 5, 2, 61, -4917, 26972),
    (42002, 5, 3, 84, -4864, 26972),
    (42002, 5, 4, 79, -4779, 26972),
    (42002, 5, 5, 35, -4711, 26972),
    (42002, 5, 6, -27, -4702, 26972),
    (42002, 5, 7, -103, -4702, 26972),
    (42002, 5, 8, -157, -4702, 26972),
    (42002, 5, 9, -224, -4761, 26972),
    (42002, 5, 10, -197, -4877, 26972),
    (42002, 5, 11, -126, -4944, 26972),
    (42005, 5, 0, -171, -5276, 26972),
    (42005, 5, 1, -85, -5222, 26972),
    (42005, 5, 2, -36, -5092, 26972),
    (42005, 5, 3, -27, -4976, 26972),
    (42005, 5, 4, -27, -4913, 26972),
    (42005, 5, 5, -81, -4868, 26972),
    (42005, 5, 6, -153, -4913, 26972),
    (42005, 5, 7, -197, -4967, 26972),
    (42005, 5, 8, -242, -5038, 26972),
    (42005, 5, 9, -269, -5106, 26972),
    (42005, 5, 10, -274, -5177, 26972),
    (42005, 5, 11, -229, -5258, 26972),
    (42005, 6, 0, -55, -4837, 26972),
    (42006, 6, 0, 3097, 516, 26972),
    (42074, 5, 0, -854, 3624, 24330),
    (42074, 6, 0, -854, 3624, 24330),
    (42120, 10, 0, -658, 4364, 26972),
    (42377, 13, 0, 3590, 265, 24330),
    (42377, 14, 0, 4146, 204, 24330),
    (42377, 15, 0, 4089, 233, 24330),
    (42377, 16, 0, 4241, 370, 26972),
    (42377, 16, 1, 4361, 365, 26972),
    (42377, 16, 2, 4461, 382, 26972),
    (42377, 16, 3, 4547, 402, 26972),
    (42377, 16, 4, 4619, 469, 26972),
    (42377, 16, 5, 4553, 524, 26972),
    (42377, 16, 6, 4428, 512, 26972),
    (42377, 16, 7, 4295, 523, 26972),
    (42377, 16, 8, 4195, 511, 26972),
    (42377, 16, 9, 4091, 473, 26972),
    (42377, 16, 10, 4054, 394, 26972),
    (42377, 16, 11, 4137, 369, 26972),
    (42440, 3, 0, 3398, 5571, 26972),
    (42477, 11, 0, -11685, -2384, 26972),
    (42477, 12, 0, -11685, -2384, 26972),
    (42477, 13, 0, -817, 4696, 26972),
    (42627, 6, 0, -968, 4436, 26972),
    (42627, 7, 0, -9153, 264, 26972),
    (42765, 3, 0, 966, -2395, 26972),
    (42772, 0, 0, 1183, -1778, 0),
    (42772, 1, 0, 1143, -1725, 0),
    (42772, 2, 0, 1065, -1748, 0),
    (42772, 3, 0, 1102, -1795, 0),
    (43100, 9, 0, 1207, -2650, 26972),
    (43100, 9, 1, 1264, -2650, 26972),
    (43100, 9, 2, 1305, -2637, 26972),
    (43100, 9, 3, 1334, -2561, 26972),
    (43100, 9, 4, 1296, -2507, 26972),
    (43100, 9, 5, 1226, -2475, 26972),
    (43100, 9, 6, 1168, -2459, 26972),
    (43100, 9, 7, 1140, -2456, 26972),
    (43100, 9, 8, 1073, -2478, 26972),
    (43100, 9, 9, 1031, -2551, 26972),
    (43100, 9, 10, 1076, -2618, 26972),
    (43100, 9, 11, 1165, -2644, 26972),
    (43254, 10, 0, -816, 4263, 24330),
    (43254, 11, 0, -819, 4256, 26972),
    (43254, 12, 0, -817, 4259, 24330),
    (43254, 13, 0, -829, 4374, 26972),
    (43254, 14, 0, -817, 4259, 24330);

CREATE TEMPORARY TABLE artifact_poi_apply LIKE artifact_poi_plan;

-- Match the complete existing blob and prevent duplicate or custom target sets.
-- An identical target set can be reused after an interrupted application.
INSERT INTO artifact_poi_apply
SELECT a.*
FROM artifact_poi_plan a
WHERE EXISTS (SELECT 1 FROM quest_template q WHERE q.ID = a.QuestID)
    AND (a.QuestObjectiveID = 0 OR EXISTS (
        SELECT 1 FROM quest_objectives o WHERE o.QuestID = a.QuestID
            AND o.ID = a.QuestObjectiveID AND o.ObjectID = a.QuestObjectID))
    AND (a.ChangeKind = 2 OR EXISTS (
        SELECT 1 FROM quest_poi b WHERE b.QuestID = a.QuestID
              AND b.BlobIndex = a.BlobIndex
              AND b.Idx1 = a.Idx1
              AND b.ObjectiveIndex = a.ObjectiveIndex
              AND b.QuestObjectiveID = a.QuestObjectiveID
              AND b.QuestObjectID = a.QuestObjectID
              AND b.MapID = a.MapID
              AND b.WorldMapAreaId = a.WorldMapAreaId
              AND b.Floor = a.Floor
              AND b.Priority = a.Priority
              AND b.Flags = a.Flags
              AND b.WorldEffectID = a.WorldEffectID
              AND b.PlayerConditionID = a.PlayerConditionID
              AND b.WoDUnk1 = a.WoDUnk1
              AND b.AlwaysAllowMergingBlobs = a.AlwaysAllowMergingBlobs))
    AND (
        (a.ChangeKind = 0 AND (
            SELECT COUNT(*) FROM quest_poi b WHERE b.QuestID = a.QuestID AND b.Idx1 = a.Idx1) = 1)
        OR (a.ChangeKind IN (1, 2) AND NOT EXISTS (
            SELECT 1 FROM quest_poi b WHERE b.QuestID = a.QuestID AND b.Idx1 = a.TargetIdx1))
    )
    AND (a.ChangeKind <> 1 OR (
        EXISTS (SELECT 1 FROM quest_poi b WHERE b.QuestID = a.QuestID
            AND b.Idx1 = a.Idx1 AND b.BlobIndex <> a.BlobIndex)
        AND (
            SELECT COUNT(*) = a.SourceCount AND
                SHA2(GROUP_CONCAT(CONCAT_WS(',', p.Idx2, p.X, p.Y, p.VerifiedBuild)
                    ORDER BY p.Idx2 SEPARATOR ';'), 256) = a.SourceHash
            FROM quest_poi_points p WHERE p.QuestID = a.QuestID AND p.Idx1 = a.Idx1
        )
    ))
    AND (
        SELECT COUNT(*) = 0 OR (
            COUNT(*) = a.TargetCount AND
            SHA2(GROUP_CONCAT(CONCAT_WS(',', p.Idx2, p.X, p.Y, p.VerifiedBuild)
                ORDER BY p.Idx2 SEPARATOR ';'), 256) = a.TargetHash)
        FROM quest_poi_points p WHERE p.QuestID = a.QuestID AND p.Idx1 = a.TargetIdx1
    );

INSERT INTO quest_poi_points (QuestID, Idx1, Idx2, X, Y, VerifiedBuild)
SELECT c.QuestID, c.Idx1, c.Idx2, c.X, c.Y, c.VerifiedBuild
FROM artifact_poi_coordinates c
JOIN artifact_poi_apply a ON a.QuestID = c.QuestID AND a.TargetIdx1 = c.Idx1
LEFT JOIN quest_poi_points existing ON existing.QuestID = c.QuestID
    AND existing.Idx1 = c.Idx1 AND existing.Idx2 = c.Idx2
WHERE existing.QuestID IS NULL;

-- The original coordinates remain available to the sibling blob.
UPDATE quest_poi b
JOIN artifact_poi_apply a ON a.QuestID = b.QuestID AND a.BlobIndex = b.BlobIndex AND a.Idx1 = b.Idx1
SET b.Idx1 = a.TargetIdx1
WHERE a.ChangeKind = 1;

INSERT INTO quest_poi (QuestID, BlobIndex, Idx1, ObjectiveIndex, QuestObjectiveID, QuestObjectID, MapID, WorldMapAreaId, Floor, Priority, Flags, WorldEffectID, PlayerConditionID, WoDUnk1, AlwaysAllowMergingBlobs, VerifiedBuild)
SELECT a.QuestID, a.BlobIndex, a.TargetIdx1, a.ObjectiveIndex, a.QuestObjectiveID, a.QuestObjectID, a.MapID, a.WorldMapAreaId, a.Floor, a.Priority, a.Flags, a.WorldEffectID, a.PlayerConditionID, a.WoDUnk1, a.AlwaysAllowMergingBlobs, a.VerifiedBuild
FROM artifact_poi_apply a
WHERE a.ChangeKind = 2;

-- The existing point is the native Tirisfal marker. Restore its map area.
UPDATE quest_poi b
JOIN quest_poi_points p ON p.QuestID = b.QuestID AND p.Idx1 = b.Idx1 AND p.Idx2 = 0
SET b.WorldMapAreaId = 20
WHERE b.QuestID = 42120 AND b.BlobIndex = 0 AND b.Idx1 = 5 AND b.ObjectiveIndex = 1
    AND b.QuestObjectiveID = 283715 AND b.QuestObjectID = 106416 AND b.MapID = 0
    AND b.WorldMapAreaId = 0 AND b.Floor = 0 AND b.Priority = 0 AND b.Flags = 2
    AND b.WorldEffectID = 0 AND b.PlayerConditionID = 0 AND b.WoDUnk1 = 0
    AND b.AlwaysAllowMergingBlobs = 0 AND p.X = 2195 AND p.Y = 2409 AND p.VerifiedBuild = 22908;

DROP TEMPORARY TABLE artifact_poi_apply;
DROP TEMPORARY TABLE artifact_poi_coordinates;
DROP TEMPORARY TABLE artifact_poi_plan;
COMMIT;
