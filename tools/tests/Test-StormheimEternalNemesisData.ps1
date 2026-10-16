$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-eternal-nemesis"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.StartScript,q.CompleteScript,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39857;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID=39857;
SELECT 'relations',COUNT(*) FROM gameobject_queststarter s JOIN creature_questender e ON e.quest=s.quest JOIN creature_queststarter n ON n.id=e.id AND n.quest=39849 WHERE s.id=244559 AND s.quest=39857 AND e.id=97480;
SELECT 'next',COUNT(*) FROM quest_template_addon a JOIN quest_template q ON q.ID=a.ID WHERE a.ID=39849 AND a.PrevQuestID=39857 AND a.NextQuestID=39850 AND q.RewardNextQuest=39850 AND q.AllowableRaces=18446744073709551615;
SELECT 'next-objective',COUNT(*) FROM quest_objectives WHERE QuestID=39849 AND ID=280021 AND Type=1 AND StorageIndex=0 AND ObjectID=128854 AND Amount=1 AND Flags=0;
SELECT 'altar',COUNT(*) FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.guid=127713 AND g.id=244559 AND g.map=1463 AND g.PhaseId='' AND t.type=2 AND t.flags=0 AND t.Data3=18790 AND t.ScriptName='go_stormheim_helheim_bargain';
SELECT 'ashildir',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.id=97480 AND c.guid=11546796 AND c.map=1463 AND c.PhaseId='' AND c.modelid=0 AND ABS(c.position_x-327.851)<0.001 AND ABS(c.position_y-574.399)<0.001 AND ABS(c.position_z-6.34934)<0.001 AND t.npcflag=2 AND t.AIName='SmartAI' AND t.ScriptName='' AND w.Displayid1=68635;
SELECT 'phase',COUNT(*) FROM conditions c JOIN phase_definitions p ON p.zoneId=c.SourceGroup AND p.entry=c.SourceEntry WHERE c.SourceTypeOrReferenceId=23 AND c.SourceGroup=7588 AND c.SourceEntry=5 AND p.phaseId=5575 AND c.ConditionValue1=39857 AND c.NegativeCondition=0 AND ((c.ElseGroup=0 AND c.ConditionTypeOrReference=28) OR (c.ElseGroup=1 AND c.ConditionTypeOrReference=8));
SELECT 'sai',entryorguid,source_type,id,event_type,event_param1,action_type,action_param1,target_type FROM smart_scripts WHERE (entryorguid=244559 AND source_type=1 AND id=0) OR (entryorguid=97480 AND source_type=0 AND id=0) ORDER BY entryorguid;
SELECT 'actor',entry,id,actorId,creatureId,displayId,unk1,unk2,unk3,duration FROM conversation_actor WHERE entry=2252;
SELECT 'line',entry,id,idx,textId,unk1,unk2 FROM conversation_data WHERE entry=2252;
SELECT 'creature-actors',COUNT(*) FROM conversation_creature WHERE entry=2252;
SELECT 'unrelated-line',COUNT(*) FROM conversation_data WHERE entry=2252 AND id=7170;
SELECT 'ash-text',COUNT(*) FROM creature_text WHERE CreatureID=97480 AND GroupID=0 AND ID=0 AND BroadcastTextID=98488 AND Type=12;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Nemesis World query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("quest`t39857`t33554440`t0`t0`t0`t39849`t18446744073709551615`t39848`t39849`t0","objectives`t0","relations`t1","next`t1","next-objective`t1","altar`t1","ashildir`t1","phase`t2","sai`t244559`t1`t0`t19`t39857`t206`t2252`t7","sai`t97480`t0`t0`t20`t39857`t1`t0`t7","actor`t2252`t0`t50552`t91387`t65043`t0`t1`t0`t3400","line`t2252`t4748`t0`t0`t620`t0","creature-actors`t0","unrelated-line`t0","ash-text`t1")){if($rows -notcontains $expected){throw "Missing World evidence $expected"}}
$rows | Set-Content "$out/world.tsv"
$sql="SELECT 'voice',ID,SoundEntriesID2,ConditionID FROM broadcast_text WHERE ID IN(98488,98489,120831) ORDER BY ID; SELECT 'line-overlay',COUNT(*) FROM conversation_line WHERE ID IN(4748,7170);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$hotfix=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Nemesis Hotfix query failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("voice`t98488`t52999`t0","voice`t98489`t53000`t0","voice`t120831`t70180`t0","line-overlay`t0")){if($hotfix -notcontains $expected){throw "Missing Hotfix evidence $expected"}}
$hotfix | Set-Content "$out/hotfix.tsv"
'PASS: read-only native Nemesis quest/relations, zero objectives, actual altar/Ashildir/phase, acceptance2062252/reward dialogue, conversation actor/line and installed voice records. No migration needed.'
