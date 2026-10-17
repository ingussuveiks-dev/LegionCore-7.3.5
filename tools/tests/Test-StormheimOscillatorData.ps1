param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-oscillator"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_01_stormheim_crystal_oscillator.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{gameobject_template='entry IN(240286,240586)';gameobject='id=240286';gameobject_loot_template='Entry IN(240286,60251)';quest_objectives='QuestID=38558';quest_template_addon='ID IN(38558,38058)'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE gameobject_template SET Data1=0 WHERE entry=240286;`n$migration`n$migration`nUPDATE saved_gameobject_template SET Data1=240286 WHERE entry=240286;`n"
 foreach($table in $tables.Keys){$sql+="SELECT 'actual-$table',t.* FROM $table t; SELECT 'expected-$table',t.* FROM saved_$table t;`n"}
}
$sql+=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38558,38058) ORDER BY q.ID;
SELECT 'objective',ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=38558;
SELECT 'loot',g.entry,g.Data1,l.Item,l.Chance,l.QuestRequired,l.LootMode,l.GroupId,l.MinCount,l.MaxCount FROM gameobject_template g JOIN gameobject_loot_template l ON l.Entry=g.Data1 WHERE g.entry=240286;
SELECT 'chest',entry,type,displayId,flags,size,Data0,Data8,Data15,Data30,Data31,AIName,ScriptName FROM gameobject_template WHERE entry=240286;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,PhaseId FROM gameobject WHERE id=240286;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(38558,38058) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=38558;
SELECT 'giver',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.PhaseId,t.npcflag,t.faction,t.ScriptName,w.Displayid1 FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.id=90866 ORDER BY c.guid;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup IN(240286,60251)) OR (SourceTypeOrReferenceId=30 AND SourceEntry=123887);
SELECT 'personal-metadata',COUNT(*) FROM personal_loot_template WHERE entry IN(240286,60251) OR goEntry=240286;
"@
if(!$Live){$sql+="UPDATE gameobject_template SET Data1=999,Data30=444,flags=123 WHERE entry=240286;`n$migration`nSELECT 'custom',Data1,Data30,flags FROM gameobject_template WHERE entry=240286; UPDATE gameobject_template SET Data1=0 WHERE entry=240286; DELETE FROM gameobject_loot_template WHERE Entry=240286;`n$migration`nSELECT 'missing',Data1 FROM gameobject_template WHERE entry=240286; INSERT INTO gameobject_loot_template SELECT * FROM saved_gameobject_loot_template; UPDATE gameobject_loot_template SET Item=999999 WHERE Entry=240286;`n$migration`nSELECT 'wrong-item',Data1 FROM gameobject_template WHERE entry=240286;"}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Oscillator World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($line in @("objective`t278005`t1`t123887`t1`t0`t0","loot`t240286`t240286`t123887`t100`t1`t1`t0`t1`t1","chest`t240286`t3`t8644`t2113540`t0.05`t1690`t0`t0`t60251`t0`t`t","spawn`t114753`t240286`t1220`t2962.24`t3081.73`t420.672`t","starter`t90866`t38558","starter`t90866`t38058","ender`t90866`t38558","conditions`t0","personal-metadata`t0","quest`t38558`t33554440`t65536`t18446744073709551615`t0`t38058`t39800`t38058`t0")){if($rows -notcontains $line){throw "Missing Oscillator contract: $line"}}
if(!$Live){foreach($table in $tables.Keys){$a="actual-$table`t";$e="expected-$table`t";$actual=@($rows | Where-Object {$_.StartsWith($a)} | ForEach-Object {$_.Substring($a.Length)} | Sort-Object);$expected=@($rows | Where-Object {$_.StartsWith($e)} | ForEach-Object {$_.Substring($e.Length)} | Sort-Object);if($actual.Count -ne $expected.Count -or ($actual.Count -and (Compare-Object $actual $expected))){throw "Full-row preservation failed$table"}};foreach($line in @("custom`t999`t444`t123","missing`t0","wrong-item`t0")){if($rows -notcontains $line){throw 'Oscillator migration guard failed'}}}
$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
$sql='SELECT ''sparse'',COUNT(*) FROM item_sparse WHERE ID=123887; SELECT ''item'',COUNT(*) FROM item WHERE ID=123887; SELECT ''lock'',COUNT(*) FROM `lock` WHERE ID=1690; SELECT ''effects'',COUNT(*) FROM spell_effect WHERE SpellID=3365; SELECT ''object'',COUNT(*) FROM gameobject_display_info WHERE ID=8644; SELECT ''display'',COUNT(*) FROM creature_display_info WHERE ID=7028; SELECT ''model'',COUNT(*) FROM creature_model_data WHERE ID=182; SELECT ''item-effects'',COUNT(*) FROM item_effect WHERE ItemID=123887;'
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Oscillator native-reference query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('sparse','item','lock','effects','object','display','model','item-effects')){if($rows -notcontains "$name`t0"){throw "Unexpected Oscillator native-reference overlay: $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: real Oscillator migration repeated, full protected rows/personal settings preserved, custom/missing/wrong loot refused; live quest/item/chest/next relations checked.'
