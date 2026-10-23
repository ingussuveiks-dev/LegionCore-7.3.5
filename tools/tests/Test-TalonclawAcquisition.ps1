$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_10_talonclaw_acquisition.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry IN(248834,248419);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
UPDATE gameobject_template SET Data1=0,ScriptName='' WHERE entry=248834;
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE source_type=1 AND entryorguid IN(248834,248419);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE pickup_gossip LIKE smart_scripts;
INSERT INTO pickup_gossip SELECT * FROM smart_scripts WHERE entryorguid=248834 AND id=0;
UPDATE pickup_gossip SET id=2,event_type=64,event_param1=3;
DELETE FROM smart_scripts WHERE entryorguid=248834 AND id=2;
INSERT INTO smart_scripts SELECT * FROM pickup_gossip;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=1 AND SourceEntry IN(248834,248419);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE SourceEntry=248834;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
VALUES (22,3,248834,1,0,2,0,128808,1,0,0,0,'','Smart Only If Player Has Artifact');
$migration
$migration
SELECT 'loot',g.entry,g.Data1,l.Item,l.Chance,l.MinCount,l.MaxCount,g.ScriptName
FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=248834;
SELECT 'settings',COUNT(*) FROM gameobject_template g JOIN saved_go s ON s.entry=g.entry
WHERE g.entry=248834 AND g.flags=s.flags AND g.Data0=s.Data0 AND g.Data3=s.Data3 AND g.Data8=s.Data8 AND g.Data30=s.Data30;
SELECT 'root',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND source_type=1 AND id=0 AND event_type=70 AND event_param1=3 AND action_type=85 AND action_param1=205589 AND link=1;
SELECT 'links',COUNT(*) FROM smart_scripts s JOIN saved_sai original ON original.entryorguid=s.entryorguid AND original.source_type=s.source_type AND original.id=s.id AND original.link=s.link AND original.event_type=s.event_type AND original.action_type=s.action_type AND original.action_param1=s.action_param1 WHERE s.entryorguid=248834 AND s.id=1;
SELECT 'early',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND source_type=1 AND id=2;
SELECT 'orphan',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=248834 AND SourceId=1 AND SourceGroup=3;
SELECT 'other-go',COUNT(*) FROM gameobject_template g JOIN saved_go s ON s.entry=g.entry WHERE g.entry=248419 AND g.Data1=s.Data1 AND g.ScriptName=s.ScriptName;
SELECT 'other-sai',COUNT(*) FROM smart_scripts WHERE entryorguid=248419 AND source_type=1 AND id=2 AND action_param1=200279;
UPDATE gameobject_template SET Data1=123456,ScriptName='custom_pickup' WHERE entry=248834;
INSERT INTO smart_scripts SELECT * FROM pickup_gossip;
$migration
SELECT 'custom',COUNT(*) FROM gameobject_template WHERE entry=248834 AND Data1=123456 AND ScriptName='custom_pickup';
SELECT 'custom-gossip',COUNT(*) FROM smart_scripts WHERE entryorguid=248834 AND id=2 AND action_param1=205589;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Talonclaw temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("loot`t248834`t248834`t128808`t100`t1`t1`tgo_artifact_acquisition","settings`t1","root`t1","links`t1","early`t0","orphan`t0","other-go`t1","other-sai`t1","custom`t1","custom-gossip`t1")){
 if($rows -notcontains $expected){throw "Talonclaw fixture mismatch: $expected"}
}
'PASS: Talonclaw real loot linkage, repeat migration, native scene/completed-loot links, removed duplicate owned-copy path, preserved admission flags and other Hunter chest'
