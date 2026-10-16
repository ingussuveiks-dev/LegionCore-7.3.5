$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-light-way"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.QuestPackageID,q.StartItem,q.StartScript,q.CompleteScript,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39849;
SELECT 'objective',ID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=39849;
SELECT 'relations',COUNT(*) FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest JOIN creature_queststarter n ON n.id=s.id AND n.quest=39850 WHERE s.id=97480 AND s.quest=39849;
SELECT 'next',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39850 AND a.PrevQuestID=39849 AND a.ExclusiveGroup=-39850 AND a.NextQuestID=39853 AND q.RewardNextQuest=39853 AND q.StartScript=39850;
SELECT 'source',COUNT(*) FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry JOIN creature_template_scaling s ON s.Entry=t.entry WHERE t.entry=91657 AND t.lootid=91657 AND t.AIName='SmartAI' AND t.ScriptName='' AND t.faction=16 AND t.unit_flags=32768 AND t.flags_extra=0 AND w.QuestItem1=128854 AND w.RequiredExpansion=6 AND w.Displayid1=62703 AND s.LevelScalingMin=98 AND s.LevelScalingMax=110 AND s.LevelScalingDeltaMin=0 AND s.LevelScalingDeltaMax=0;
SELECT 'spawn',COUNT(*) FROM creature WHERE id=91657 AND guid=340975 AND map=1463 AND PhaseId='' AND modelid=0 AND spawnMask=1 AND spawntimesecs=180 AND ABS(position_x-227.785)<0.001 AND ABS(position_y-863.536)<0.001 AND ABS(position_z-20.1206)<0.001;
SELECT 'all-source-spawns',COUNT(*) FROM creature WHERE id=91657;
SELECT 'drop',t.entry,t.lootid,l.Item,l.Currency,l.Reference,l.Chance,l.QuestRequired,l.LootMode,l.GroupId,l.MinCount,l.MaxCount,l.Shared FROM creature_template t JOIN creature_loot_template l ON l.Entry=t.lootid WHERE t.entry=91657 AND l.Item=128854;
SELECT 'all-item-drops',COUNT(*) FROM creature_loot_template WHERE Item=128854;
SELECT 'extra-loot-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceGroup=91657 AND SourceEntry=128854;
SELECT 'difficulty',COUNT(*) FROM smart_scripts WHERE entryorguid=91657 AND source_type=0 AND Difficulties='' AND event_flags=0 AND link=0 AND event_chance=100;
SELECT 'combat',COUNT(*) FROM smart_scripts WHERE entryorguid=91657 AND source_type=0 AND event_type=0 AND action_type=11 AND ((action_param1=186529 AND target_type=1) OR(action_param1=188271 AND target_type=1) OR(action_param1=186409 AND target_type=2));
SELECT 'ashildir',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.id=97480 AND c.guid=11546796 AND c.map=1463 AND c.PhaseId='' AND t.npcflag=2 AND t.AIName='SmartAI' AND t.ScriptName='';
SELECT 'reward-speech',COUNT(*) FROM smart_scripts s JOIN creature_text t ON t.CreatureID=s.entryorguid AND t.GroupID=s.action_param1 WHERE s.entryorguid=97480 AND s.source_type=0 AND s.event_type=20 AND s.event_param1=39849 AND s.action_type=1 AND s.target_type=7 AND t.BroadcastTextID=98490;
SELECT 'model-info',COUNT(*) FROM creature_model_info WHERE DisplayID=62703 AND BoundingRadius>0 AND CombatReach>0;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Light Way World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("quest`t39849`t33554440`t0`t746`t0`t0`t0`t39850`t18446744073709551615`t39857`t39850`t0","objective`t280021`t1`t0`t128854`t1`t0","relations`t1","next`t1","source`t1","spawn`t1","all-source-spawns`t1","drop`t91657`t91657`t128854`t0`t0`t99.155`t1`t0`t0`t1`t1`t0","all-item-drops`t1","extra-loot-conditions`t0","difficulty`t5","combat`t3","ashildir`t1","reward-speech`t1","model-info`t1")){if($rows -notcontains $expected){throw "Missing World evidence $expected"}}
$rows | Set-Content "$out/world.tsv"
($rows | Where-Object {$_ -like "drop`t*"}) -replace '^drop\t','' | Set-Content "$out/drop.tsv"
$sql="SELECT 'item-overrides',COUNT(*) FROM item_sparse WHERE ID IN(128854,129239,129339); SELECT 'effect-overrides',COUNT(*) FROM item_effect WHERE ItemID=128854; SELECT 'display-overrides',COUNT(*) FROM creature_display_info WHERE ID=62703; SELECT 'voice',COUNT(*) FROM broadcast_text WHERE ID=98490 AND SoundEntriesID2=53001; SELECT 'spec-overrides',COUNT(*) FROM item_spec; SELECT 'reward-spec-overrides',COUNT(*) FROM item_spec_override WHERE ItemID IN(129239,129339); SELECT 'specialization-overrides',COUNT(*) FROM chr_specialization; SELECT 'package-overrides',COUNT(*) FROM quest_package_item WHERE PackageID=746;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Light Way Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("item-overrides`t0","effect-overrides`t0","display-overrides`t0","voice`t1","spec-overrides`t0","reward-spec-overrides`t0","specialization-overrides`t0","package-overrides`t0")){if($hotfix -notcontains $expected){throw "Missing Hotfix evidence $expected"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: read-only Waylight objective, Bloodbeard live loot link/drop/spawn/scaling/SmartAI, Ashildir relations, package746 and next39850 boundaries; no extra loot conditions or selected item/model overrides.'
