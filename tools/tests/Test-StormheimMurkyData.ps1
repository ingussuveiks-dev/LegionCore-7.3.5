param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-murky"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Murky([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Murky data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_19_stormheim_murky.sql" -Raw
$setup=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE (entryorguid=98577 AND source_type=0) OR(entryorguid=97973 AND source_type=0);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(98577,97973)) OR(SourceTypeOrReferenceId=13 AND SourceEntry=194067);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM smart_scripts WHERE entryorguid=98577 AND source_type=0 AND id=2;
UPDATE smart_scripts SET event_type=8,event_flags=0,event_param1=194067,event_param2=0,event_param3=0,event_param4=0,event_param5=0,link=0 WHERE entryorguid=98577 AND source_type=0 AND id IN(0,1);
DELETE FROM conditions WHERE(SourceTypeOrReferenceId=22 AND SourceEntry=98577) OR(SourceTypeOrReferenceId=13 AND SourceEntry=194067);
SELECT 'baseline',id,event_type,event_flags,action_type,action_param1 FROM smart_scripts WHERE entryorguid=98577 AND source_type=0 ORDER BY id;
$migration
$migration
SELECT 'unrelated',COUNT(*) FROM smart_scripts s JOIN saved_smart o USING(entryorguid,source_type,id,link) WHERE s.entryorguid=97973 AND s.event_type=o.event_type AND s.action_type=o.action_type AND s.action_param1=o.action_param1;
SELECT 'unrelated-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=97973;
"@
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,a.ProvidedItemCount,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40120;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40120;
SELECT 'torch-drop',ItemDrop1,ItemDropQuantity1 FROM quest_template WHERE ID=40120;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=40120;
SELECT 'npc-ender',id,quest FROM creature_questender WHERE quest=40120;
SELECT 'smart',id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,event_chance,action_type,action_param1,target_type,Difficulties FROM smart_scripts WHERE entryorguid=98577 AND source_type=0 ORDER BY id;
SELECT 'condition',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ElseGroup FROM conditions WHERE(SourceTypeOrReferenceId=22 AND SourceEntry=98577 AND SourceId=0) OR(SourceTypeOrReferenceId=13 AND SourceEntry=194067) ORDER BY SourceTypeOrReferenceId,SourceGroup,ConditionTypeOrReference;
SELECT 'actor',entry,npcflag,faction,unit_flags,unit_flags2,unit_flags3,AIName,ScriptName FROM creature_template WHERE entry IN(98577,98587) ORDER BY entry;
SELECT 'pose',entry,bytes1,auras FROM creature_template_addon WHERE entry IN(98577,98587) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,spawntimesecs,phaseMask,PhaseId,spawnMask,modelid,dynamicflags FROM creature WHERE id IN(98577,98587) ORDER BY id,guid;
SELECT 'model',w.Entry,w.Displayid1,w.Displayid2,w.Displayid3 FROM creature_template_wdb w WHERE w.Entry IN(98577,98587) ORDER BY w.Entry;
SELECT 'model-info',DisplayID FROM creature_model_info WHERE DisplayID IN(25240,25241,25242,63102) ORDER BY DisplayID;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(98577,98587) ORDER BY Entry;
SELECT 'other-conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(17,19,20) AND SourceEntry IN(194067,40120)) OR(SourceTypeOrReferenceId=22 AND SourceEntry=98577 AND SourceId<>0);
SELECT 'spawn-scripts',COUNT(*) FROM smart_scripts WHERE entryorguid IN(SELECT -CAST(guid AS SIGNED) FROM creature WHERE id IN(98577,98587)) AND source_type=0;
SELECT 'spell-script',COUNT(*) FROM spell_script_names WHERE spell_id IN(194067,194069);
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry=129725;
SELECT 'item-addons',COUNT(*) FROM item_template_addon WHERE Id=129725;
'@
$fixture=Read-Murky World ($setup+"`n"+$sql)
$fixture | Set-Content "$out/migration.tsv"
foreach($r in @("baseline`t0`t8`t0`t33`t98577","baseline`t1`t8`t0`t75`t194069","unrelated`t1","unrelated-condition`t1")){if($fixture -notcontains $r){throw "Murky migration isolation failed: $r"}}
$rows=$fixture
if($Live){$rows=Read-Murky World $sql}
$rows | Set-Content "$out/world.tsv"
foreach($r in @("quest`t40120`t302120968`t65536`t18446744073709551615`t129725`t1`t0`t0`t0`t0","objective`t40120`t280336`t0`t98577`t8`t2`t0","starter`t98587`t40120","npc-ender`t98587`t40120","smart`t0`t1`t8`t1`t194067`t0`t0`t0`t100`t33`t98577`t7`t","smart`t1`t2`t61`t0`t0`t0`t0`t0`t100`t75`t194069`t1`t","smart`t2`t0`t61`t0`t0`t0`t0`t0`t100`t41`t10000`t1`t","actor`t98577`t0`t14`t536904448`t2049`t8192`tSmartAI`t","actor`t98587`t2`t190`t33536`t2048`t0`t`t","pose`t98577`t262144`t159474","pose`t98587`t0`t211179","model`t98577`t25240`t25241`t25242","model`t98587`t63102`t0`t0")){if($rows -notcontains $r){throw "Murky contract changed: $r"}}
if(@($rows | Where-Object {$_ -like "smart`t*"}).Count -ne 3 -or @($rows | Where-Object {$_ -like "condition`t*"}).Count -ne 5 -or @($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 1){throw 'Unexpected Murky event/condition/objective set'}
if($rows -notcontains "torch-drop`t129725`t1"){throw 'Torch cleanup contract changed'}
foreach($r in @("condition`t22`t1`t98577`t1`t1`t194069`t0`t0`t1`t0","condition`t22`t1`t98577`t9`t0`t40120`t0`t0`t0`t0","condition`t22`t1`t98577`t31`t0`t4`t0`t0`t0`t0","condition`t13`t1`t194067`t31`t0`t3`t98577`t0`t0`t0","condition`t13`t1`t194067`t1`t0`t194069`t0`t0`t1`t0")){if($rows -notcontains $r){throw "Missing Murky gate: $r"}}
foreach($kind in @('other-conditions','spawn-scripts','spell-script','item-overrides','item-addons')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
foreach($id in @(98577,98587)){if($rows -notcontains "scaling`t$id`t98`t110"){throw 'Invalid Murky actor scaling'}}
foreach($id in @(25240,25241,25242,63102)){if($rows -notcontains "model-info`t$id"){throw 'Missing Murky model metadata'}}
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 17 -or @($spawns | Where-Object {$_.Split("`t")[2] -eq '98577'}).Count -ne 16){throw 'Unexpected corpse/giver spawn count'}
foreach($s in $spawns){$v=$s.Split("`t");if($v[3] -ne '1220' -or $v[7] -ne '180' -or $v[8] -ne '1' -or $v[9] -ne '' -or $v[10] -ne '1' -or $v[11] -ne '0'){throw "Unexpected spawn lifecycle: $s"}}
$rows=Read-Murky Hotfix @'
SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=129725; SELECT 'uses',COUNT(*) FROM item_effect WHERE ItemID=129725; SELECT 'effect',COUNT(*) FROM spell_effect WHERE SpellID IN(194067,194069,159474,211179); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(194067,194069,159474,211179); SELECT 'aura',COUNT(*) FROM spell_aura_restrictions WHERE SpellID=194067; SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(25240,25241,25242,63102); SELECT 'target',COUNT(*) FROM spell_target_restrictions WHERE SpellID=194067; SELECT 'duration',COUNT(*) FROM spell_duration WHERE ID=1; SELECT 'range',COUNT(*) FROM spell_range WHERE ID=291;
'@
$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('item','uses','effect','misc','aura','display','target','duration','range')){if($rows -notcontains "$kind`t0"){throw "Unexpected Murky native $kind override"}}
'PASS: original repeatable hit events reconstructed, migration twice, scoped conditions/LINK lifecycle, unrelated Havi preservation,16 corpse spawns/native poses/models and torch source item.'
