$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-dilemma"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Dilemma([string]$database,[string]$sql,[string]$name){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw "Dilemma $database query failed"}}finally{$env:MYSQL_PWD=$old}
 $rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/$name.tsv";return $rows
}
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38872,38873,39153,39385,39154,38878) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(39153,39154) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39153,39154,38878) ORDER BY quest,id;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39153,39154) ORDER BY quest,id;
SELECT 'incoming',ID FROM quest_template_addon WHERE NextQuestID=39154 ORDER BY ID;
SELECT 'actor',t.entry,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1,w.Displayid2 FROM creature_template t JOIN creature_template_wdb w USING(entry) WHERE t.entry=94614 OR w.KillCredit1=94614 OR w.KillCredit2=94614 ORDER BY t.entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,modelid,phaseMask,PhaseId FROM creature WHERE id IN(93860,94614,94825,109633,109635,93603) ORDER BY id,guid;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(93860,94614,94825,109633,109635) ORDER BY Entry;
SELECT 'models',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID IN(63721,63722,63723,63724,63726,63727,63728,63729,66644) ORDER BY DisplayID;
SELECT 'giver',entry,npcflag,faction,AIName,ScriptName FROM creature_template WHERE entry=93603;
SELECT 'extra-conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(39153,39154)) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(93860,94614,94825,109633,109635));
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE entryorguid IN(93860,94614,94825,109633,109635) AND source_type=0;
SELECT 'spawn-smart',c.guid,c.id,s.event_type,s.event_chance,s.event_flags,s.event_param1,s.event_param2,s.event_param3,s.event_param4,s.action_type,s.action_param1,s.target_type,s.Difficulties FROM smart_scripts s JOIN creature c ON s.entryorguid=-CAST(c.guid AS SIGNED) WHERE s.source_type=0 AND c.id IN(93860,94614,94825,109633,109635) ORDER BY c.guid;
SELECT 'spawn-smart-conditions',COUNT(*) FROM conditions x JOIN creature c ON x.SourceEntry=-CAST(c.guid AS SIGNED) WHERE x.SourceTypeOrReferenceId=22 AND c.id IN(93860,94614,94825,109633,109635);
SELECT 'start-script',COUNT(*) FROM quest_start_scripts WHERE id=39153;
SELECT 'cosmetic-bindings',COUNT(*) FROM spell_script_names WHERE spell_id=168875;
SELECT 'cosmetic-dummy',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=168875;
SELECT 'cosmetic-linked',COUNT(*) FROM spell_linked_spell WHERE spell_trigger IN(168875,-168875);
'@
$rows=Read-Dilemma World $sql 'world'
foreach($r in @("quest`t39153`t33554440`t0`t234881970`t0`t39154`t38872`t39154`t0`t0`t0","objective`t39153`t279015`t0`t94614`t10`t2`t0","starter`t93603`t39153","ender`t93603`t39153","starter`t93603`t39154","ender`t94227`t39154","giver`t93603`t3`t118`t`tnpc_stormheim_fleet_actor","extra-conditions`t0","smart`t0","start-script`t0")){if($rows -notcontains $r){throw "Dilemma contract changed: $r"}}
$incoming=@($rows | Where-Object {$_ -like "incoming`t*"});if(($incoming -join ',') -ne "incoming`t38873,incoming`t39153,incoming`t39385"){throw '39154 incoming set changed: refresh all branch coverage'}
if(@($rows | Where-Object {$_ -like "objective`t39153`t*"}).Count -ne 1 -or @($rows | Where-Object {$_ -like "objective`t39154`t*"}).Count){throw 'Unexpected Dilemma/delivery objective'}
foreach($entry in @(93860,94614,94825,109633,109635)){if($rows -notcontains "scaling`t$entry`t98`t110"){throw "Inactive scaling$entry"}}
$counts=@{93860=2;94614=8;94825=6;109633=8;109635=10;93603=1}
foreach($entry in $counts.Keys){$spawns=@($rows | Where-Object {$_ -like "spawn`t*`t$entry`t*"});if($spawns.Count -ne $counts[$entry]){throw "Spawn count drift$entry"};foreach($s in $spawns){$p=$s.Split("`t");if($p[3] -ne '1220' -or $p[7] -ne '0' -or $p[8] -ne '1' -or $p[9]){throw 'Unexpected model override/map/phase'}}}
if(@($rows | Where-Object {$_ -like "actor`t*"}).Count -ne 5 -or @($rows | Where-Object {$_ -like "models`t*"}).Count -ne 9){throw 'Missing actor/model metadata'}
foreach($r in @("spawn-smart`t293875`t94614`t60`t100`t0`t0`t5000`t2000`t5000`t11`t168875`t1`t","spawn-smart`t293884`t109635`t60`t100`t0`t0`t5000`t2000`t5000`t11`t168875`t1`t","spawn-smart-conditions`t0")){if($rows -notcontains $r){throw 'Individual spawn AI changed'}}
foreach($r in @("cosmetic-bindings`t0","cosmetic-dummy`t0","cosmetic-linked`t0")){if($rows -notcontains $r){throw 'Cosmetic hook changed; recheck its effects'}}
if(@($rows | Where-Object {$_ -like "spawn-smart`t*"}).Count -ne 2){throw 'Unexpected additional spawn AI'}
foreach($r in ($rows | Where-Object {$_ -like "actor`t*"})){$p=$r.Split("`t");$entry=[int]$p[1];$ai=if($entry -in @(94614,109635)){'SmartAI'}else{''};$faction=if($entry -eq 94825){'2618'}else{'2166'};$flags2=if($entry -eq 94825){'33556480'}else{'2048'};if($p[4] -ne $faction -or $p[5] -ne '0' -or $p[6] -ne '32768' -or $p[7] -ne $flags2 -or $p[8] -ne $ai -or $p[9]){throw 'Unexpected actor admission/AI flags'}}
$h=Read-Dilemma Hotfix "SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(63721,63722,63723,63724,63726,63727,63728,63729,66644); SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID IN(3141,3142,10608); SELECT 'faction',COUNT(*) FROM faction_template WHERE ID IN(2166,2618,118,1,2); SELECT 'cosmetic-effect',COUNT(*) FROM spell_effect WHERE SpellID=168875;" 'hotfix'
foreach($r in @("display`t0","model`t0","faction`t0","cosmetic-effect`t0")){if($h -notcontains $r){throw 'Unexpected Dilemma native reference overlay'}}
'PASS:39153 native94614/10/storage2, five credit templates/34 source spawns, Nathanos giver, active scaling, all three39154 incoming alternatives and no extra quest/script condition.'
