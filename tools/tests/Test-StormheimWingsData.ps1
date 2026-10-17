param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-wings"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_06_stormheim_wings_release.sql" -Raw
$sql=''
if(!$Live){foreach($table in @('gameobject_template','spell_script_names')){$filter=if($table -eq 'gameobject_template'){'entry IN(240650,244335,244336,244337)'}else{'spell_id=3365'};$sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE gameobject_template SET ScriptName=''; DELETE FROM spell_script_names WHERE ScriptName='spell_stormheim_wings_opening';`n$migration`n$migration`nUPDATE saved_gameobject_template SET ScriptName='go_stormheim_wings'; SELECT 'actual-template',t.* FROM gameobject_template t ORDER BY entry; SELECT 'expected-template',t.* FROM saved_gameobject_template t ORDER BY entry;"
}
$sql+=@'
SELECT 'go',entry,type,Data0,Data1,Data10,ScriptName FROM gameobject_template WHERE entry IN(240650,244335,244336,244337) ORDER BY entry;
SELECT 'opening',COUNT(*) FROM spell_script_names WHERE spell_id=3365 AND ScriptName='spell_stormheim_wings_opening';
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=38413 ORDER BY StorageIndex;
SELECT 'npc',c.guid,c.id,c.position_x,c.position_y,c.position_z,c.PhaseId,t.ScriptName,a.bytes1 FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_addon a ON a.entry=c.id WHERE c.id IN(97028,97031,97032,97033) ORDER BY c.id;
SELECT 'native-dummy-binding',COUNT(*) FROM spell_script_names WHERE spell_id=193637;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38413;
SELECT 'next',id,quest FROM creature_queststarter WHERE quest=39652;
'@
if(!$Live){$sql+="SELECT 'retained-script',s.* FROM spell_script_names s WHERE ScriptName<>'spell_stormheim_wings_opening'; SELECT 'expected-script',s.* FROM saved_spell_script_names s WHERE ScriptName<>'spell_stormheim_wings_opening'; UPDATE gameobject_template SET ScriptName=''; UPDATE gameobject_template SET ScriptName='custom' WHERE entry=240650; DELETE FROM spell_script_names WHERE ScriptName='spell_stormheim_wings_opening';`n$migration`nSELECT 'custom',COUNT(*) FROM gameobject_template WHERE ScriptName='custom'; SELECT 'custom-others',COUNT(*) FROM gameobject_template WHERE ScriptName=''; SELECT 'custom-binding',COUNT(*) FROM spell_script_names WHERE ScriptName='spell_stormheim_wings_opening';"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Wings release migration query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/release$(if($Live){'-live'}).tsv"
foreach($id in @(240650,244335,244336,244337)){if($rows -notcontains "go`t$id`t10`t1690`t0`t193637`tgo_stormheim_wings"){throw 'Wings circle contract changed'}}
foreach($row in @("opening`t1","native-dummy-binding`t0","ender`t96465`t38413","next`t96465`t39652")){if($rows -notcontains $row){throw "Wings release changed: $row"}}
if(@($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 4 -or @($rows | Where-Object {$_ -like "npc`t*"}).Count -ne 4){throw 'Wrong native site count'}
if(!$Live){foreach($pair in @(@('actual-template','expected-template'),@('retained-script','expected-script'))){$a=$pair[0]+"`t";$b=$pair[1]+"`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)});$expected=@($rows | Where-Object {$_.StartsWith($b)} | ForEach-Object {$_.Substring($b.Length)});if(Compare-Object $actual $expected){throw 'Wings unrelated fields/bindings changed'}}
 foreach($row in @("custom`t1","custom-others`t3","custom-binding`t0")){if($rows -notcontains $row){throw 'Custom script guard failed'}}}
$sql=@'
SELECT 'effect',COUNT(*) FROM spell_effect WHERE SpellID=193637; SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(193637,3365); SELECT 'lock',COUNT(*) FROM `lock` WHERE ID=1690; SELECT 'display',COUNT(*) FROM gameobject_display_info WHERE ID=16973; SELECT 'actor',COUNT(*) FROM creature_display_info WHERE ID IN(67198,68213); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID=8673;
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Wings Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('effect','misc','lock','display','actor','model')){if($rows -notcontains "$kind`t0"){throw "Unexpected Wings native $kind override"}}
if($Live){'PASS: live Wings four circles, Opening binding, native objectives/actors/ender and native overlay contract.'}
else{'PASS: native four sites/objectives, release binding twice, complete template/other Opening binding preservation and custom-script guard.'}
