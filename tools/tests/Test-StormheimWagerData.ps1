$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-wager"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39789,39787,39786,39792) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags,Flags2 FROM quest_objectives WHERE QuestID IN(39787,39786,39792) ORDER BY QuestID;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39787,39786,39792) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(39787,39786,39792) ORDER BY quest;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM creature_loot_template WHERE Item IN(138020,138022,138023) ORDER BY Entry;
SELECT 'actor',t.entry,t.lootid,t.faction,t.npcflag,t.flags_extra,w.FlagQuest,w.VignetteID,w.Displayid1 FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(97124,107852,107914,107674,107675) ORDER BY t.entry;
SELECT 'spawns',id,COUNT(*),SUM(modelid<>0),SUM(PhaseId<>''),SUM((spawnMask & 1)=0) FROM creature WHERE id IN(97124,107852,107914,107674,107675) GROUP BY id ORDER BY id;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(97124,107852,107914) ORDER BY Entry;
SELECT 'rock',entry,type,displayId,flags,Data0,Data8,Data10,ScriptName FROM gameobject_template WHERE entry=244457;
SELECT 'rock-spawn',guid,map,PhaseId,position_x,position_y,position_z FROM gameobject WHERE id=244457;
SELECT 'phase',zoneId,entry,phaseId,flags FROM phase_definitions WHERE zoneId=7541 AND entry=400;
SELECT 'phase-condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=400;
SELECT 'summon',id,delay,command,datalong,datalong2,dataint,x,y,z,o FROM event_scripts WHERE id=46479;
SELECT 'sleep',entry,auras FROM creature_template_addon WHERE entry=107914;
SELECT 'wake',event_type,action_type,action_param1,target_type FROM smart_scripts WHERE entryorguid=107914 AND source_type=0;
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(39787,39786,39792)) OR(SourceTypeOrReferenceId=1 AND SourceEntry IN(138020,138022,138023));
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry IN(138020,138022,138023);
SELECT 'item-flags',COUNT(*) FROM item_template_addon WHERE Id IN(138020,138022,138023);
SELECT 'model-metadata',COUNT(*) FROM creature_model_info WHERE DisplayID IN(70334,70336,70340,70226,70227) AND BoundingRadius>0 AND CombatReach>0;
SELECT 'admission',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39787,39786,39792) AND q.QuestType=2 AND q.MinLevel=98 AND q.QuestSortID=7541 AND q.StartScript=0 AND q.CompleteScript=0 AND a.AllowableClasses=0;
SELECT 'questgiver-flags',COUNT(*) FROM creature_template WHERE entry IN(107674,107675) AND(npcflag & 2)<>0;
SELECT 'static-spitefeather',COUNT(*) FROM creature WHERE id=97124;
SELECT 'event-count',COUNT(*) FROM event_scripts WHERE id=46479;
SELECT 'phase-condition-count',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=400;
SELECT 'extra-handlers',(SELECT COUNT(*) FROM spell_script_names WHERE spell_id=192045)+(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=244457 AND source_type=1)+(SELECT COUNT(*) FROM gameobject_scripts WHERE id=25167470);
SELECT 'anchors',COUNT(*) FROM creature WHERE (guid=371030 AND id=107674 AND map=1220 AND ABS(position_x-2953.34)<0.001 AND ABS(position_y-2057.69)<0.001 AND ABS(position_z-202.631)<0.001) OR(guid=338664 AND id=107675 AND map=1220 AND ABS(position_x-2969.44)<0.001 AND ABS(position_y-2047.61)<0.001 AND ABS(position_z-201.939)<0.001) OR(guid=290799 AND id=107914 AND map=1220 AND ABS(position_x-2827.81)<0.001 AND ABS(position_y-2341.43)<0.001 AND ABS(position_z-144.987)<0.001) OR(guid=293764 AND id=107852 AND map=1220 AND ABS(position_x-2339.84)<0.001 AND ABS(position_y-2043.03)<0.001 AND ABS(position_z-170.825)<0.001);
'@
$previous=$env:MYSQL_PWD
function Query($database,$text){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';')
 try{$env:MYSQL_PWD=$c[3];$result=@($text | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Wager query failed'};return $result}finally{$env:MYSQL_PWD=$previous}
}
$rows=@(Query 'World' $sql);$rows | Set-Content "$out/world.tsv"
$expected=@(
"quest`t39786`t33554440`t0`t18446744073709551615`t0`t0`t39787`t0`t0`t0`t0",
"quest`t39787`t171966472`t0`t18446744073709551615`t0`t0`t39789`t0`t0`t0`t0",
"quest`t39792`t33554440`t0`t18446744073709551615`t0`t0`t39786`t0`t0`t0`t17908",
"objective`t39786`t279945`t1`t138023`t1`t0`t1`t0","objective`t39787`t279946`t1`t138022`t1`t0`t1`t0","objective`t39792`t279959`t1`t138020`t8`t0`t0`t0",
"loot`t97124`t138022`t69.8967`t1`t0`t0`t1`t1`t0`t0","loot`t107852`t138020`t36.6502`t1`t0`t0`t1`t1`t0`t0","loot`t107914`t138023`t94.1998`t1`t0`t0`t1`t1`t0`t0",
"spawns`t107674`t1`t0`t0`t0","spawns`t107675`t1`t0`t0`t0","spawns`t107852`t35`t0`t0`t0","spawns`t107914`t1`t0`t0`t0",
"rock`t244457`t10`t28738`t0`t2160`t0`t192045`t","rock-spawn`t25167470`t1220`t5600`t3905.85`t2223.91`t322.099",
"phase`t7541`t400`t5600`t16","phase-condition`t9`t0`t39787`t0",
"summon`t46479`t0`t10`t97124`t30000`t0`t3907.56`t2230.56`t324.377`t4.5006",
"sleep`t107914`t214691 145953","wake`t4`t28`t145953`t1","model-metadata`t5","admission`t3","questgiver-flags`t2","static-spitefeather`t0","event-count`t1","phase-condition-count`t1","extra-handlers`t0","anchors`t4")
foreach($id in @(39786,39787,39792)){$giver=if($id -eq 39792){107675}else{107674};$expected+="starter`t$giver`t$id";$expected+="ender`t$giver`t$id"}
foreach($id in @(97124,107852,107914)){$expected+="scaling`t$id`t98`t110"}
foreach($kind in @('conditions','item-overrides','item-flags')){$expected+="$kind`t0"}
foreach($line in $expected){if($rows -notcontains $line){throw "Wager contract changed: $line"}}
if(@($rows | Where-Object {$_ -like "loot`t*"}).Count -ne 3){throw 'Unexpected quest item source'}
foreach($id in @(97124,107852,107914)){$line=$rows | Where-Object {$_ -like "actor`t$id`t*"};$f=$line.Split("`t");if($f[2] -ne "$id" -or $f[5] -ne '0' -or $f[6] -ne '0' -or $f[7] -ne '0'){throw 'Loot selector changed'}}
$sql='SELECT ''overlays'',(SELECT COUNT(*) FROM item_sparse WHERE ID IN(138020,138022,138023))+(SELECT COUNT(*) FROM item WHERE ID IN(138020,138022,138023))+(SELECT COUNT(*) FROM item_effect WHERE ItemID IN(138020,138022,138023))+(SELECT COUNT(*) FROM spell_effect WHERE SpellID IN(192045,145953,214691,214146,123169))+(SELECT COUNT(*) FROM quest_package_item WHERE PackageID=17908)+(SELECT COUNT(*) FROM creature_display_info WHERE ID IN(70334,70336,70340,70226,70227))+(SELECT COUNT(*) FROM gameobject_display_info WHERE ID=28738)+(SELECT COUNT(*) FROM `lock` WHERE ID=2160);'
$rows=@(Query 'Hotfix' $sql);$rows | Set-Content "$out/hotfix.tsv"
if(($rows -join '') -ne "overlays`t0"){throw 'Native catalog has overlays'}
'PASS: three wager quests, mandatory1/1/8 items, rewarded prerequisites, real givers, ordinary loot sources,38 unphased actors, rock phase/event/summon, Stonefang sleep/wake, valid scaling and native overlays.'
