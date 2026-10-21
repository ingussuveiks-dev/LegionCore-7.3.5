$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warlock-progression"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$text,[string]$marker){$a=$text.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$end=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++};if($depth){throw "Unbalanced $marker"};$text.Substring($a,$end-$a)}
function Save($name,$value){[IO.File]::WriteAllText("$out/$name.inc",$value)}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
foreach($p in @(@('Level','bool Player::SatisfyQuestLevel('),@('Previous','bool Player::SatisfyQuestPreviousQuest('),@('Class','bool Player::SatisfyQuestClass('),@('Race','bool Player::SatisfyQuestRace('))){Save $p[0] (Block $player $p[1])}
Save 'Status' ((Block (Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw) 'enum QuestStatus')+';')
$cond=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=@('CONDITION_REPUTATION_RANK','CONDITION_QUESTREWARDED','CONDITION_QUESTTAKEN','CONDITION_QUEST_COMPLETE','CONDITION_QUEST_NONE','CONDITION_QUEST_OBJECTIVE_DONE') | ForEach-Object {$a=$cond.IndexOf("        case $_"+':');$b=$cond.IndexOf('        case ',$a+13);$cond.Substring($a,$b-$a)}
Save 'ConditionCases' ($cases -join "`n")
$a=$cond.IndexOf('    if (NegativeCondition)');$b=$cond.IndexOf("`n}",$a)
Save 'ConditionTail' $cond.Substring($a,$b-$a)
Save 'ConditionList' (Block $cond 'bool ConditionMgr::IsObjectMeetToConditionList(')
$ids='40821,40823,40824,44099,42608,41748,41750,42603,41797,42602,42601,42097,41759,39179,39389,39142,40218,41767,41752,41753,41798,42100,42098,39718,39731,39733,39735,39864,39861,41220,39985,43341'
$sql=@"
SELECT 'Q',q.ID,q.MinLevel,COALESCE(a.MaxLevel,0),COALESCE(a.AllowableClasses,0),q.AllowableRaces,COALESCE(a.ExclusiveGroup,0),COALESCE(a.PrevQuestID,0),COALESCE(a.NextQuestID,0) FROM quest_template q LEFT JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN($ids) ORDER BY q.ID;
SELECT 'C',SourceTypeOrReferenceId,IF(SourceTypeOrReferenceId=23,SourceEntry,SourceEntry),ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE (SourceTypeOrReferenceId=19 AND SourceEntry IN($ids)) OR (SourceTypeOrReferenceId=23 AND SourceGroup=7875 AND SourceEntry<=10) ORDER BY SourceTypeOrReferenceId,SourceEntry,ElseGroup;
SELECT 'V','stage-phases',COUNT(*) FROM phase_definitions WHERE zoneId=7875 AND ((entry=4 AND phaseId='6933') OR (entry=5 AND phaseId='6934') OR (entry=6 AND phaseId='6935') OR (entry=7 AND phaseId='6936') OR (entry=9 AND phaseId='6938') OR (entry=10 AND phaseId='6939'));
SELECT 'V','natural-givers',COUNT(DISTINCT t.entry) FROM creature_template t JOIN creature c ON c.id=t.entry WHERE t.entry IN(101097,104795,106199,105816,105018,105922,105923) AND (t.npcflag & 2)<>0 AND c.map=IF(t.entry=105816,1220,1107);
SELECT 'V','disabled',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN($ids);
SELECT 'V','suramar-trigger',COUNT(*) FROM area_queststart WHERE id=7506 AND quest=39985;
SELECT 'V','zone-followups',COUNT(*) FROM creature_queststarter WHERE (id=86563 AND quest=41220) OR (id=91172 AND quest=39861);
SELECT 'V','research-service',COUNT(*) FROM creature_template t WHERE t.entry=108018 AND t.npcflag2=512 AND EXISTS(SELECT 1 FROM gossip_menu_option o WHERE o.MenuID=t.gossip_menu_id AND o.OptionNpc=32);
SELECT 'V','troop-service',COUNT(*) FROM creature_template t JOIN garrison_shipment s ON s.NpcEntry=t.entry WHERE t.entry=106216 AND t.npcflag2=272 AND s.classReq=256 AND s.questReq=41797 AND s.ContainerID=129 AND EXISTS(SELECT 1 FROM gossip_menu_option o WHERE o.MenuID=t.gossip_menu_id AND o.OptionNpc=28);
SELECT 'V','missions',COUNT(*) FROM mission_quest_link WHERE (QuestID=42603 AND MissionStartID=827) OR (QuestID=42602 AND MissionStartID=928) OR (QuestID=42097 AND MissionStartID=839);
"@
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$db[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Progression catalog read failed'}}finally{$env:MYSQL_PWD=$old}
foreach($expected in @("V`tstage-phases`t6","V`tnatural-givers`t7","V`tdisabled`t0","V`tsuramar-trigger`t1","V`tzone-followups`t2","V`tresearch-service`t1","V`ttroop-service`t1","V`tmissions`t3")){if($rows -notcontains $expected){throw "Progression source data changed: $expected"}}
[IO.File]::WriteAllLines("$out/availability.tsv",@($rows | Where-Object {!$_.StartsWith("V`t")}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarlockProgressionTest.cpp`" /Fe`"$out/availability.exe`" /Fo`"$out/availability.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/availability.exe`" `"$out/availability.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run-availability.cmd",$cmd);& "$out/run-availability.cmd"
if($LASTEXITCODE){throw 'Warlock native progression availability failed'}
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarlockProgressionCatalog.cpp -ProbeName WarlockProgressionCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @(
 'TREE 48516 parent 48515 op 0 amount 1 criteria 30438 type 176 asset 616 mod 0',
 'TREE 48518 parent 48517 op 0 amount 1 criteria 30439 type 176 asset 589 mod 0',
 'TREE 53275 parent 53274 op 0 amount 1 criteria 32968 type 174 asset 827 mod 0',
 'TREE 48537 parent 48536 op 0 amount 1 criteria 30449 type 174 asset 928 mod 0',
 'TREE 47551 parent 47550 op 0 amount 1 criteria 29857 type 174 asset 839 mod 0',
 'SHIPMENT 255 container 129 spell 210565 complete 210566 follower 649 flags 1',
 'SHIPMENT 321 container 129 spell 219621 complete 219732 follower 649 flags 0',
 'SPELL 210566 effect 90 misc 106208',
 'DUNGEON 1204 min 110 max 110 difficulty 1')){if($native -notcontains $expected){throw "Native Warlock progression data changed: $expected"}}
'PASS: actual final-link gossip lookup, follower/mission criteria, native training credit and level-110 dungeon boundary.'
