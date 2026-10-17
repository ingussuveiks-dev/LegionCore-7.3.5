param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-gift"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_13_stormheim_gift.sql" -Raw
$sql=''
if(!$Live){$sql=@'
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(243454,243455,243456);
UPDATE saved_gameobject_template SET ScriptName='' WHERE entry=243455;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_gameobject_template;
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
CREATE TEMPORARY TABLE saved_spell_script_names LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_spell_script_names;
'@;$sql+=$migration+"`n"+$migration+"`n"}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38872,38873,39153,39385,39154,38878) ORDER BY q.ID;
SELECT 'incoming',ID FROM quest_template_addon WHERE NextQuestID=39154 ORDER BY ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(39385,39154) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39385,39154,38878);
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39385,39154,38878);
SELECT 'go-starter',id,quest FROM gameobject_queststarter WHERE quest=39385;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,Data3,Data10,Data14,ScriptName,SpellStateVisualID FROM gameobject_template WHERE entry IN(243454,243455,243456) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z FROM gameobject WHERE id IN(243454,243455,243456) ORDER BY guid;
SELECT 'giver',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(93603,94227,95611) ORDER BY guid;
SELECT 'spellarea',spell,area,quest_start,autocast,quest_start_status FROM spell_area WHERE quest_start=39385;
SELECT 'condition',ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=189152;
SELECT 'condition-full',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=189152;
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=189134 AND ScriptName='spell_stormheim_gift_detonator';
SELECT 'summon-credit',event_type,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=95409 AND source_type=0;
SELECT 'controller',entry,AIName,ScriptName FROM creature_template WHERE entry=95409;
SELECT 'custom-targets',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=189134;
SELECT 'target-position',COUNT(*) FROM spell_target_position WHERE ID=189134;
'@
if(!$Live){$sql+=@"
SELECT 'protected',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry IN(243454,243456) AND a.ScriptName=b.ScriptName AND a.flags=b.flags AND a.Data1=b.Data1;
UPDATE gameobject_template SET ScriptName='' WHERE entry=243455;
INSERT INTO spell_script_names VALUES(189134,'custom_gift');
$migration
SELECT 'custom-preserved',ScriptName='' FROM gameobject_template WHERE entry=243455;
SELECT 'custom-bindings',COUNT(*) FROM spell_script_names WHERE spell_id=189134;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Gift World query/replay failed'}}finally{$env:MYSQL_PWD=$old}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
foreach($r in @("quest`t39385`t38928392`t8192`t234881970`t0`t39154`t38872`t39154`t0`t0`t743","quest`t39154`t33554440`t0`t234881970`t0`t38878`t39385`t38878`t0`t0`t0","objective`t39385`t279315`t2`t243455`t3`t0`t0","objective`t39385`t279319`t0`t95412`t1`t1`t2","starter`t93603`t39385","starter`t93603`t39154","ender`t93603`t39385","ender`t94227`t39154","starter`t94227`t38878","ender`t95611`t38878","go-starter`t243454`t39385","spellarea`t189152`t7773`t39385`t1`t8","condition`t41`t39385`t243455`t3","binding`t1","summon-credit`t54`t33`t95412`t23","controller`t95409`tSmartAI`t","custom-targets`t0","target-position`t0")){if($rows -notcontains $r){throw "Gift chain contract changed: $r"}}
foreach($r in @("condition-full`t0`t2`t0`t128287`t1`t0`t1","condition-full`t0`t41`t0`t39385`t243455`t3`t0")){if($rows -notcontains $r){throw 'Gift creator source17 conjunction changed'}}
if((@($rows | Where-Object {$_ -like "incoming`t*"}) -join ',') -ne "incoming`t38873,incoming`t39153,incoming`t39385"){throw '39154 predecessor set changed'}
if(!$Live){foreach($r in @("protected`t2","custom-preserved`t1","custom-bindings`t2")){if($rows -notcontains $r){throw 'Gift migration overwrote custom/protected data'}}}
$sql="SELECT 'effect',COUNT(*) FROM spell_effect WHERE SpellID IN(189134,189152); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(189134,189152); SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=128287; SELECT 'item-effect',COUNT(*) FROM item_effect WHERE ItemID=128287; SELECT 'targets',COUNT(*) FROM spell_target_restrictions WHERE SpellID=189134;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$h=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Gift Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
$h=@($h | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$h | Set-Content "$out/hotfix.tsv"
foreach($kind in @('effect','misc','item','item-effect','targets')){if($h -notcontains "$kind`t0"){throw "Unexpected native $kind override"}}
'PASS:39385/39154/38878 native goals/links, source GO/creator condition/item cast binding, unchanged controller credit, repeated migration and custom protection.'
