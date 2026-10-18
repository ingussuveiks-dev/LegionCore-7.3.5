param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt scene SQL check failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_14_vrekt_drake_scene.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE scene_original_ct LIKE creature_template;
INSERT INTO scene_original_ct SELECT * FROM creature_template WHERE entry IN(107830,115846,115885,116601);
UPDATE scene_original_ct SET ScriptName=IF(entry=115846,'npc_vrekt','') WHERE entry IN(107830,115846,115885);
CREATE TEMPORARY TABLE scene_original_sai LIKE smart_scripts;
INSERT INTO scene_original_sai SELECT * FROM smart_scripts WHERE entryorguid IN(107830,116601);
DELETE FROM scene_original_sai WHERE entryorguid=107830 AND source_type=0 AND id IN(0,1);
INSERT INTO scene_original_sai(entryorguid,source_type,id,Difficulties,event_type,event_chance,event_flags,action_type,action_param1,target_type,target_param1,comment)
VALUES(107830,0,0,'',1,100,1,11,211409,19,115846,'OOC - CS'),(107830,0,1,'',1,100,1,11,211409,19,115846,'external sentinel');
CREATE TEMPORARY TABLE creature_template LIKE scene_original_ct;
CREATE TEMPORARY TABLE smart_scripts LIKE scene_original_sai;
INSERT INTO creature_template SELECT * FROM scene_original_ct;
INSERT INTO smart_scripts SELECT * FROM scene_original_sai;
$migration
SELECT 'first-ct',t.* FROM creature_template t ORDER BY entry;
SELECT 'first-sai',t.* FROM smart_scripts t ORDER BY entryorguid,source_type,id;
$migration
SELECT 'repeat-ct',t.* FROM creature_template t ORDER BY entry;
SELECT 'repeat-sai',t.* FROM smart_scripts t ORDER BY entryorguid,source_type,id;
UPDATE scene_original_ct SET ScriptName='npc_vrekt_scene' WHERE entry IN(107830,115846,115885);
DELETE FROM scene_original_sai WHERE entryorguid=107830 AND source_type=0 AND id=0;
SELECT 'expected-ct',t.* FROM scene_original_ct t ORDER BY entry;
SELECT 'expected-sai',t.* FROM scene_original_sai t ORDER BY entryorguid,source_type,id;
"@
foreach($entry in @(107830,115846,115885)){
 foreach($field in @('ScriptName','AIName')){
  $tag="foreign-$entry-$field"
  $sql+=@"

DELETE FROM creature_template;
INSERT INTO creature_template SELECT * FROM scene_original_ct;
UPDATE creature_template SET $field='external_sentinel' WHERE entry=$entry;
DELETE FROM smart_scripts WHERE entryorguid=107830 AND source_type=0 AND id=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,Difficulties,event_type,event_chance,event_flags,action_type,action_param1,target_type,target_param1,comment)
VALUES(107830,0,0,'',1,100,1,11,211409,19,115846,'OOC - CS');
SELECT '$tag-before',t.* FROM creature_template t ORDER BY entry;
SELECT '$tag-sai-before',t.* FROM smart_scripts t ORDER BY entryorguid,source_type,id;
$migration
SELECT '$tag-after',t.* FROM creature_template t ORDER BY entry;
SELECT '$tag-sai-after',t.* FROM smart_scripts t ORDER BY entryorguid,source_type,id;
"@
 }
}
$rows=@(Query $sql)
function Tagged($tag){(@($rows | Where-Object {$_ -like "$tag`t*"} | ForEach-Object {($_ -split "`t",2)[1]}) -join '|')}
foreach($table in @('ct','sai')){if((Tagged "first-$table") -ne (Tagged "repeat-$table") -or (Tagged "repeat-$table") -ne (Tagged "expected-$table")){throw "Scene migration changed unrelated $table data or was not repeatable"}}
foreach($entry in @(107830,115846,115885)){foreach($field in @('ScriptName','AIName')){$tag="foreign-$entry-$field";if((Tagged "$tag-before") -ne (Tagged "$tag-after") -or (Tagged "$tag-sai-before") -ne (Tagged "$tag-sai-after")){throw "Partial scene overwrote $tag"}}}
$anchor=@(Query 'SELECT guid,id,map,zoneId,areaId,position_x,position_y,position_z,orientation FROM creature WHERE guid IN(293175,146706022) ORDER BY guid;')
if(($anchor -join '|') -ne "293175`t107830`t1220`t7541`t8362`t1816.98`t734.469`t85.5647`t1.18135|146706022`t115846`t1220`t7541`t8362`t1823.38`t754.189`t81.9073`t3.99319"){throw 'Scene anchors changed; re-audit the reconstruction'}
if($Live){
 $bindings=@(Query "SELECT entry,IF(AIName='','-',AIName),ScriptName FROM creature_template WHERE entry IN(107830,115846,115885) ORDER BY entry;")
 if(($bindings -join '|') -ne "107830`tSmartAI`tnpc_vrekt_scene|115846`t-`tnpc_vrekt_scene|115885`t-`tnpc_vrekt_scene"){throw 'Deployed scene binding mismatch'}
 if((Query 'SELECT COUNT(*) FROM creature WHERE id=115885;') -ne '0'){throw 'Unexpected permanent captured drake spawn'}
 if((Query "SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=107830 AND source_type=0 AND id=0 AND action_type=11 AND action_param1=211409 AND target_type=19 AND target_param1=115846 AND comment='OOC - CS';") -ne '0'){throw 'Old scene row remains deployed'}
}
'PASS: repeat scene migration changes only three script bindings and the exact old chain row; all other fields/rows, neighbour and six foreign binding cases preserved. Live DB anchors unchanged; no permanent captured spawn.'
