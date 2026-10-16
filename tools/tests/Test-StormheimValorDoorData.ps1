$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-valor-door"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@"
SELECT 'quest',q.ID,q.QuestType,q.MinLevel,q.Flags,q.FlagsEx,q.StartItem,q.StartScript,q.CompleteScript,q.RewardSpell,q.RewardNextQuest,q.AllowableRaces,a.PrevQuestID,a.NextQuestID,a.SpecialFlags,a.RequiredSkillID,a.RequiredSkillPoints FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40001;
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID=40001;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=40001;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=40001;
SELECT 'prior',COUNT(*) FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40078 AND q.RewardNextQuest=40001 AND a.NextQuestID=40001;
SELECT 'next',COUNT(*) FROM creature_queststarter s JOIN quest_template q ON q.ID=s.quest JOIN quest_template_addon a ON a.ID=q.ID WHERE s.id=97979 AND s.quest=40002 AND a.PrevQuestID=40001 AND q.RewardNextQuest=40003 AND q.AllowableRaces=18446744073709551615;
SELECT 'next-objective',ID,Type,ObjectID,Amount,StorageIndex FROM quest_objectives WHERE QuestID=40002 ORDER BY ID;
SELECT 'actor',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id JOIN creature_template_wdb w ON w.Entry=c.id WHERE c.guid=291392 AND c.id=97979 AND c.map=1220 AND c.zoneId=7541 AND c.areaId=7852 AND c.spawnMask=1 AND c.phaseMask=1 AND c.PhaseId='' AND c.modelid=0 AND c.npcflag=0 AND c.npcflag2=0 AND c.unit_flags=0 AND c.MovementType=0 AND ABS(c.position_x-2875.26)<0.001 AND ABS(c.position_y-1208.98)<0.001 AND ABS(c.position_z-188.763)<0.001 AND ABS(c.orientation-0.930901)<0.00001 AND w.Displayid1=68213 AND w.Displayid2=0 AND w.Displayid3=0 AND w.Displayid4=0 AND t.npcflag=3 AND t.npcflag2=0 AND t.faction=190 AND t.unit_flags=33536 AND t.unit_flags2=2048 AND t.AIName='' AND t.ScriptName='' AND t.gossip_menu_id=12618 AND t.MovementType=0;
SELECT 'addon',entry,path_id,mount,bytes1,bytes2,emote,COALESCE(auras,'') FROM creature_template_addon WHERE entry=97979;
SELECT 'spawn-addon',COUNT(*) FROM creature_addon WHERE guid=291392;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID=68213;
SELECT 'menu',MenuID,TextID FROM gossip_menu WHERE MenuID=12618;
SELECT 'menu-options',COUNT(*) FROM gossip_menu_option WHERE MenuID=12618;
SELECT 'scripts',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(97979,-291392)) OR (event_type IN(19,20) AND event_param1=40001);
SELECT 'conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=40001) OR (SourceTypeOrReferenceId IN(14,15) AND SourceGroup=12618) OR (SourceTypeOrReferenceId=23 AND ConditionValue1=40001 AND ConditionTypeOrReference IN(8,9,28));
SELECT 'havi',COUNT(*) FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid=291280 AND c.id=92539 AND c.map=1220 AND t.npcflag=3 AND t.AIName='SmartAI';
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Valor Door World query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("quest`t40001`t2`t98`t33554440`t0`t0`t0`t0`t0`t40002`t18446744073709551615`t40078`t40002`t0`t0`t0","objectives`t0","starter`t92539`t40001","ender`t97979`t40001","prior`t1","next`t1","next-objective`t280169`t0`t97982`t1`t0","next-objective`t280170`t0`t97981`t1`t1","actor`t1","addon`t97979`t0`t0`t3`t0`t0`t","spawn-addon`t0","model`t68213`t0.75`t6.75","menu`t12618`t17749","menu-options`t0","scripts`t0","conditions`t0","havi`t1")){if($rows -notcontains $expected){throw "Missing World evidence $expected"}}
$rows | Set-Content "$out/world.tsv"
$sql="SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID=68213; SELECT 'model',COUNT(*) FROM creature_model_data WHERE ID=8673; SELECT 'faction-template',COUNT(*) FROM faction_template WHERE ID=190; SELECT 'faction',COUNT(*) FROM faction WHERE ID=148; SELECT 'quest-line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(40001,40002,40078);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Valor Door Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('display','model','faction-template','faction','quest-line')){if($rows -notcontains "$name`t0"){throw "Unexpected Hotfix overlay $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: live40001 prerequisites, no objectives, exact Havi/Vethir relations, native stationary sleeping/selectable questgiver, model metadata and40002 handoff. Read only; no migration needed.'
