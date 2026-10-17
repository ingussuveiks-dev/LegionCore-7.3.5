param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-greywatch"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-QuestData([string]$database,[string]$sql,[string]$file) {
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw "Greywatch $database query failed"}}finally{$env:MYSQL_PWD=$previous}
 $rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/$file";return $rows
}
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39059,39060,39061,39472,39062) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(39060,39061,39472) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39060,39061,39472,39062) ORDER BY quest;
SELECT 'go-starter',id,quest FROM gameobject_queststarter WHERE quest IN(39060,39061,39472,39062);
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39060,39061,39472,39062) ORDER BY quest;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM creature_loot_template WHERE Item=127285 ORDER BY Entry;
SELECT 'actor',t.entry,t.lootid,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1 FROM creature_template t JOIN creature_template_wdb w ON w.entry=t.entry WHERE t.entry IN(93987,94337,94338,109639,109640,94340,94341,94342) ORDER BY t.entry;
SELECT 'position',guid,id,map,position_x,position_y,position_z,modelid,phaseMask,PhaseId FROM creature WHERE id IN(93987,94318,93779,94340,94341,94342) ORDER BY id,guid;
SELECT 'cache',entry,type,displayId,flags,size,Data0,Data1,Data2,Data10,AIName,ScriptName FROM gameobject_template WHERE entry IN(243339,243343,243344,243700) ORDER BY entry;
SELECT 'go-position',guid,id,map,position_x,position_y,position_z,PhaseId FROM gameobject WHERE id IN(243339,243343,243344,243700) ORDER BY id;
SELECT 'smart',entryorguid,event_type,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,Difficulties FROM smart_scripts WHERE entryorguid IN(94340,94341,94342) AND source_type=0 ORDER BY entryorguid,id;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(39060,39061,39472,39062)) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry=186745) OR (SourceTypeOrReferenceId=1 AND SourceEntry=127285) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(94340,94341,94342));
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry IN(127285,127295);
SELECT 'item-flags',COUNT(*) FROM item_template_addon WHERE Id IN(127285,127295);
SELECT 'linked',COUNT(*) FROM spell_linked_spell WHERE spell_trigger IN(186745,186746,186747);
SELECT 'spell-bindings-unexpected',COUNT(*) FROM spell_script_names WHERE spell_id IN(186745,186746) AND ScriptName NOT IN('spell_stormheim_blight_torch','spell_stormheim_blight_throw');
SELECT 'spell-bindings',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(186745,186746) ORDER BY spell_id,ScriptName;
SELECT 'spell-dummy',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=186745;
SELECT 'incoming',ID FROM quest_template_addon WHERE NextQuestID=39062 ORDER BY ID;
SELECT 'torch-source',ProvidedItemCount FROM quest_template_addon WHERE ID=39060;
'@
$rows=Read-QuestData World $sql 'world.tsv'
foreach($row in @(
 "quest`t39060`t33554440`t0`t824181837`t127295`t39062`t39059`t39062`t0`t0`t0",
 "quest`t39061`t33554440`t0`t824181837`t0`t39062`t39059`t39062`t0`t0`t0",
 "quest`t39472`t33554440`t0`t824181837`t0`t39062`t0`t39062`t0`t0`t752",
 "quest`t39062`t33554440`t0`t824181837`t0`t39063`t39472`t39063`t0`t0`t0",
 "objective`t39060`t278869`t0`t94340`t1`t0`t0","objective`t39060`t278870`t0`t94341`t1`t1`t0","objective`t39060`t278871`t0`t94342`t1`t2`t0",
 "objective`t39061`t278872`t1`t127285`t1`t0`t0","objective`t39472`t279631`t0`t93987`t1`t0`t0",
 "starter`t94318`t39060","starter`t94318`t39061","starter`t94318`t39062","go-starter`t243700`t39472",
 "ender`t94318`t39060","ender`t94318`t39061","ender`t94318`t39472","ender`t93779`t39062",
 "loot`t94337`t127285`t13.0615`t1`t0`t0`t1`t1`t0`t0","loot`t94338`t127285`t7.0059`t1`t0`t0`t1`t1`t0`t0",
 "loot`t109639`t127285`t27.4488`t1`t0`t0`t1`t1`t0`t0","loot`t109640`t127285`t17.8233`t1`t0`t0`t1`t1`t0`t0",
 "cache`t243339`t5`t8618`t0`t1`t0`t1`t0`t0`t`t","cache`t243343`t5`t8618`t0`t1`t0`t1`t0`t0`t`t","cache`t243344`t5`t8618`t0`t1`t0`t1`t0`t0`t`t",
 "cache`t243700`t2`t9676`t0`t1.5`t0`t19431`t0`t0`t`t",
 "actor`t94340`t0`t0`t0`t190`t0`t33555200`t2048`tSmartAI`t`t328",
 "actor`t94341`t0`t0`t0`t190`t0`t33555200`t2048`tSmartAI`t`t328",
 "actor`t94342`t0`t0`t0`t190`t0`t33555200`t2048`tSmartAI`t`t328",
 "actor`t93987`t93987`t94338`t0`t118`t0`t32768`t2048`t`t`t63804",
 "position`t292386`t94340`t1220`t2921.11`t712.945`t85.3764`t0`t1`t",
 "position`t292849`t94341`t1220`t2732.83`t768.535`t104.51`t0`t1`t",
 "position`t292844`t94342`t1220`t2826.72`t818.351`t115.888`t0`t1`t",
 "position`t292797`t93987`t1220`t2778.67`t680.672`t103.628`t0`t1`t",
 "incoming`t39060","incoming`t39061","incoming`t39472"
 ,"torch-source`t1"
)){if($rows -notcontains $row){throw "Greywatch contract changed: $row"}}
foreach($id in @(94340,94341,94342)){if($rows -notcontains "smart`t$id`t8`t100`t0`t186745`t0`t10000`t10000`t33`t$id`t7`t"){throw "Greywatch native SpellHit credit changed: $id"}}
foreach($kind in @('conditions','item-overrides','item-flags','linked','spell-bindings-unexpected','spell-dummy')){if($rows -notcontains "$kind`t0"){throw "Unexpected Greywatch $kind"}}
if($Live){foreach($row in @("spell-bindings`t186745`tspell_stormheim_blight_torch","spell-bindings`t186746`tspell_stormheim_blight_throw")){if($rows -notcontains $row){throw "Missing deployed Greywatch binding: $row"}}}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_07_stormheim_greywatch_torch.sql" -Raw
$fixture=@'
CREATE TEMPORARY TABLE spell_script_names (spell_id INT,ScriptName VARCHAR(128),PRIMARY KEY(spell_id,ScriptName));
INSERT INTO spell_script_names VALUES(186745,'custom_torch'),(999,'unrelated');
'@
$replay=Read-QuestData World ($fixture+$migration+$migration+"SELECT 'migration-count',COUNT(*) FROM spell_script_names; SELECT 'preserved',COUNT(*) FROM spell_script_names WHERE ScriptName IN('custom_torch','unrelated');") 'migration.tsv'
if($replay -notcontains "migration-count`t4" -or $replay -notcontains "preserved`t2"){throw 'Greywatch migration repeat/custom-binding preservation failed'}
$hotfix=Read-QuestData Hotfix @'
SELECT 'item',COUNT(*) FROM item_sparse WHERE ID IN(127285,127295);
SELECT 'uses',COUNT(*) FROM item_effect WHERE ItemID IN(127285,127295);
SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(186745,186746,186747);
SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(186745,186746,186747);
SELECT 'target',COUNT(*) FROM spell_target_restrictions WHERE SpellID IN(186745,186746,186747);
SELECT 'visual-links',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(186745,186746,186747);
SELECT 'duration',COUNT(*) FROM spell_duration WHERE ID=6;
SELECT 'actor-display',COUNT(*) FROM creature_display_info WHERE ID IN(63804,63938,63939,64271,64268,328);
SELECT 'actor-model',COUNT(*) FROM creature_model_data WHERE ID IN(58,57,114);
SELECT 'aura',COUNT(*) FROM spell_aura_options WHERE SpellID IN(186745,186746,186747);
SELECT 'radius',COUNT(*) FROM spell_radius WHERE ID=8;
SELECT 'display',COUNT(*) FROM gameobject_display_info WHERE ID IN(8618,9676);
'@ 'hotfix.tsv'
foreach($kind in @('item','uses','spell','misc','target','radius','display','visual-links','duration','actor-display','actor-model','aura')){if($hotfix -notcontains "$kind`t0"){throw "Unexpected native Greywatch $kind override"}}
'PASS: Greywatch three distinct caches, four quest-only order drops, Curseheart objective, Alliance mask and native incoming-quest alternatives; no production repair inferred from row presence.'
